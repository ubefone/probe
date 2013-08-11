
#include "d3probe.h"

#include <fcntl.h>
#include <errno.h>




mrb_value wrap_io(mrb_state *mrb, int fd)
{
  struct RClass *c = mrb_class_get(mrb, "BasicSocket");
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 1, mrb_fixnum_value(fd));
}

void *plugin_thread(void *arg)
{
  mrb_value ret;
  Plugin *plugin = (Plugin *)arg;
  
  ret = mrb_funcall(plugin->mrb, plugin->plugin_obj, "cycle", 0);
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
  int fds[2], flags;
  
  plugin->mrb = mrb_open();
  setup_api(plugin->mrb);
  execute_file(plugin->mrb, path);
  
  C_CHECK("socketpair", socketpair(PF_UNIX, SOCK_DGRAM, 0, fds));
  
  flags = fcntl(fds[0], F_GETFL);
  flags |= O_NONBLOCK;
  if( fcntl(fds[0], F_SETFL, flags) == -1 ){
    perror("fcntl");
    return -1;
  }
  
  
  plugin->host_pipe = fds[0];
  plugin->plugin_pipe = wrap_io(plugin->mrb, fds[1]);
  
  
  // set ivs
  // struct RClass *rprobe = mrb_class_get(plugin->mrb, "D3Probe");
  // mrb_value probe_klass = mrb_obj_value(rprobe);
  mrb_sym plugin_iv_sym = mrb_intern2(plugin->mrb, "@plugin", 7);
  
  plugin->plugin_obj = mrb_iv_get(plugin->mrb, mrb_obj_value(plugin->mrb->top_self), plugin_iv_sym);
  pp(plugin->mrb, plugin->plugin_obj, 0);
  
  // associates the c structure with the ruby objetc
  DATA_PTR(plugin->plugin_obj)  = (void*)plugin;
  
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

#define NOPLUGIN_VALUE 0


int main(int argc, char const *argv[])
{
  fd_set rfds;
  char buffer[BUFFER_SIZE];
  int i, n;
  Plugin plugins[MAX_PLUGINS];
  int plugins_count = 0;
  mrb_state *mrb = mrb_open();
  mrb_value r_output;
  struct RClass *output_class;
  mrb_sym output_gv_sym = mrb_intern2(mrb, "$output", 7);
  
  printf("Initializing core...\n");
  setup_api(mrb);
  execute_file(mrb, "plugins/main.rb");
  
  printf("Loading plugins...\n");
  init_plugin_from_file(&plugins[plugins_count], "plugins/test.rb"); plugins_count++;
  init_plugin_from_file(&plugins[plugins_count], "plugins/ping.rb"); plugins_count++;
  // init_plugin_from_file(&plugins[1], "plugins/test2.rb"); plugins_count++;
  
  
  printf("Instanciating output class...\n");
  // output_class = mrb_class_get(mrb, "Output");
  // r_output = mrb_iv_get(mrb, mrb_obj_value(mrb->top_self), output_iv_sym);
  // r_output = mrb_funcall(mrb, mrb_obj_value(output_class), "new", 0);
  // protect from GC
  // mrb_iv_set(mrb, mrb_obj_value(mrb->top_self), output_iv_sym, r_output);
  r_output = mrb_gv_get(mrb, output_gv_sym);
  
  printf("Sending initial report...\n");
  mrb_funcall(mrb, r_output, "send_report", 0);

  
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
    int maxfd = 0, ai;
    struct timeval tv;
    mrb_value r_buffer;
    
    sleep(1);
    
    bzero(fds, sizeof(int) * MAX_PLUGINS);
    
    // ask every plugin to send their data
    for(i= 0; i< plugins_count; i++){
      strcpy(buffer, "request");
      C_CHECK("send", send(plugins[i].host_pipe, buffer, strlen(buffer), 0) );
      fds[i] = plugins[i].host_pipe;
      // printf("sent request to %d\n", i);
    }
    
    // printf("waiting answers...\n");
    // and now wait for each answer
    while(1){
      int left = 0;
      
      FD_ZERO(&rfds);
      
      for(i = 0; i< MAX_PLUGINS; i++){
        if( fds[i] != NOPLUGIN_VALUE ){
          FD_SET(fds[i], &rfds);
          left++;
          if( fds[i] > maxfd )
            maxfd = fds[i];
        }
      }
      
      // printf("left: %d %d\n", left, left <= 0);
      
      if( 0 == left )
        break;
      
      fill_timeout(&tv, 500000);
      // printf("before select\n");
      n = select(maxfd + 1, &rfds, NULL, NULL, &tv);
      // printf("after select: %d\n", n);
      if( n > 0 ){
        // find out which pipes have data
        for(i = 0; i< MAX_PLUGINS; i++){
          if( (fds[i] != NOPLUGIN_VALUE) && FD_ISSET(fds[i], &rfds) ){
            while (1){
              n = read(fds[i], buffer, sizeof(buffer));
              if( n == -1 ){
                if( errno != EAGAIN )
                  perror("read");
                break;
              }
              
              // printf("[fd:%d] data from plugin: %d bytes\n", fds[i], n);
              
              buffer[n] = 0x00;
              
              ai = mrb_gc_arena_save(mrb);
              r_buffer = mrb_str_buf_new(mrb, n);
              mrb_str_buf_cat(mrb, r_buffer, buffer, n);
              
              // mrb_funcall(mrb, r_output, "tick", 0);
              mrb_funcall(mrb, r_output, "add", 1, r_buffer);
              check_exception(mrb);
              
              // pp(mrb, r_output, 0);
              
              mrb_gc_arena_restore(mrb, ai);
            }
            
            fds[i] = 0;
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
    
    mrb_funcall(mrb, r_output, "flush", 0);
    check_exception(mrb);
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

