
#include "d3probe.h"

static void pp(mrb_state *mrb, mrb_value obj, int prompt)
{
  obj = mrb_funcall(mrb, obj, "inspect", 0);
  if (prompt) {
    if (!mrb->exc) {
      fputs(" => ", stdout);
    }
    else {
      obj = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
    }
  }
  fwrite(RSTRING_PTR(obj), RSTRING_LEN(obj), 1, stdout);
  putc('\n', stdout);
}


mrb_value wrap_io(mrb_state *mrb, int fd)
{
  struct RClass *c = mrb_class_get(mrb, "BasicSocket");
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 1, mrb_fixnum_value(fd));
}

void *plugin_thread(void *arg)
{
  Plugin *plugin = (Plugin *)arg;
  
  struct RClass *rprobe = mrb_class_get(plugin->mrb, "D3Probe");
  mrb_value probe_klass = mrb_obj_value(rprobe);
  mrb_sym pipe_iv_sym = mrb_intern2(plugin->mrb, "@pipe", 5);
  
  
  mrb_iv_set(plugin->mrb, probe_klass, pipe_iv_sym, plugin->plugin_pipe);
  mrb_value ret = mrb_funcall(plugin->mrb, probe_klass, "start_cycle", 0);
  
  if (plugin->mrb->exc) {
    if (!mrb_undef_p(ret)) {
      mrb_print_error(plugin->mrb);
    }
  }
  else {
    // pp(plugin->mrb, ret, 0);
  }
  
  return NULL;
}



int init_plugin_from_file(Plugin *plugin, const char *path)
{
  int fds[2];
  
  plugin->mrb = mrb_open();
  setup_api(plugin->mrb);
  execute_file(plugin->mrb, path);
  
  C_CHECK("socketpair", socketpair(PF_UNIX, SOCK_DGRAM, 0, fds));
  
  plugin->host_pipe = fds[0];
  plugin->plugin_pipe = wrap_io(plugin->mrb, fds[1]);
  
  return 0;
}

static void fill_timeout(struct timeval *tv, uint64_t duration)
{
  tv->tv_sec = 0;
  while( duration >= 1000000 ){
    duration -= 1000000;
    tv->tv_sec += 1;
  }
  
  tv->tv_usec = duration;
}

#define BUFFER_SIZE 500
#define MAX_PLUGINS 10

int main(int argc, char const *argv[])
{
  fd_set rfds;
  char buffer[BUFFER_SIZE];
  int i, n;
  Plugin plugins[MAX_PLUGINS];
  int plugins_count = 2;
  // mrb_state *mrb = mrb_open();
  
  init_plugin_from_file(&plugins[0], "test.rb");
  init_plugin_from_file(&plugins[1], "test2.rb");
  
  
  // puts("setup");
  // setup_api(mrb);
  // puts("load");
  // execute_file(mrb, "test.rb");
  // // execute_compiled_file(mrb, "test.mrb");
  // // execute_string(mrb, "fuck(1)");
  
  // puts("after load");
  // mrb_value p;
  
  // start all the threads
  for(i= 0; i< plugins_count; i++){
    // printf("== plugin %d\n", i);
    n = pthread_create(&plugins[i].thread, NULL, plugin_thread, (void *)&plugins[i]);
    if( n < 0 ){
      fprintf(stderr, "create failed\n");
    }
  }
  
  while(1){
    int fds[MAX_PLUGINS];
    int maxfd = 0;
    struct timeval tv;
    sleep(2);
    
    bzero(fds, sizeof(int) * MAX_PLUGINS);
    
    // ask every plugin to send their data
    for(i= 0; i< plugins_count; i++){
      strcpy(buffer, "request");
      C_CHECK("send", send(plugins[i].host_pipe, buffer, strlen(buffer), 0) );
      fds[i] = plugins[i].host_pipe;
      printf("sent request to %d\n", i);
    }
    
    printf("waiting answers...\n");
    // and now wait for each answer
    while(1){
      FD_ZERO(&rfds);
      
      for(i = 0; i< MAX_PLUGINS; i++){
        if( fds[i] != 0 ){
          FD_SET(fds[i], &rfds);
          if( fds[i] > maxfd )
            maxfd = fds[i];
        }
      }
      
      fill_timeout(&tv, 5000);
      // printf("before select\n");
      n = select(maxfd + 1, &rfds, NULL, NULL, &tv);
      // printf("after select: %d\n", n);
      if( n > 0 ){
        // find out which pipes have data
        for(i = 0; i< MAX_PLUGINS; i++){
          if( (fds[i] != 0) && FD_ISSET(fds[i], &rfds) ){
            n = read(fds[i], buffer, sizeof(buffer));
            buffer[n] = 0x00;
            printf("data from plugin: %s\n", buffer);
          }
        }
      }
      else if( n == 0 )  {
        // timeout
      }
      else {
        perror("select");
      }

    }
  }
  
  for(i= 0; i< plugins_count; i++){
    printf("== joining thread %d\n", i);
    if( pthread_join(plugins[i].thread, NULL) < 0){
      fprintf(stderr, "join ailed\n");
    }
  }
  
  puts("Sleeping...\n");
  sleep(10);
  
  printf("completed !\n");
  
  return 0;
}

