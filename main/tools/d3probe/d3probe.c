#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>


#include "d3probe.h"

static const char *config_path;


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
  check_exception("cycle", plugin->mrb);
  return NULL;
}



int init_plugin_from_file(Plugin *plugin, const char *path)
{
  int fds[2], flags;
  
  printf("Loading plugin %s...\n", path);
  plugin->mrb = mrb_open_allocf(profiler_allocf, path);
  setup_api(plugin->mrb);
  execute_file(plugin->mrb, path);
  execute_file(plugin->mrb, config_path);
  
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

#define BUFFER_SIZE 2048
#define MAX_PLUGINS 10

#define NOPLUGIN_VALUE 0

static void sleep_delay(struct timeval *start, struct timeval *end, mrb_int interval)
{
  uint64_t expected = interval * 1000;
  uint64_t elapsed_useconds;
  
  elapsed_useconds = (end->tv_sec * 1000000 + end->tv_usec) - (start->tv_sec * 1000000 + start->tv_usec);
  
  if(expected > elapsed_useconds){
    usleep(expected - elapsed_useconds);
  }
  else {
    printf("warning: loop execution exceeded interval ! (%d ms)\n", elapsed_useconds / 1000);
  }
}


static uint8_t running = 1;

void clean_exit(int sig)
{
  puts("Exiting...");
  running = 0;
}

int main(int argc, char const *argv[])
{
  uint8_t checkpoint_set = 0;
  fd_set rfds;
  char buffer[BUFFER_SIZE];
  int i, n;
  Plugin plugins[MAX_PLUGINS];
  int plugins_count = 0;
  mrb_state *mrb = mrb_open_allocf(profiler_allocf, "main");
  mrb_value r_output;
  mrb_sym output_gv_sym = mrb_intern2(mrb, "$output", 7);
  mrb_int interval;
  
  if( argc != 2 ){
    printf("Usage: %s <config_path>\n", argv[0]);
    exit(1);
  }
  
  config_path = argv[1];
  
  
  printf("Initializing core...\n");
  setup_api(mrb);
  execute_file(mrb, "plugins/main.rb");
  execute_file(mrb, config_path);
  
  printf("Loading plugins...\n");
  // init_plugin_from_file(&plugins[plugins_count], "plugins/dummy.rb"); plugins_count++;
  init_plugin_from_file(&plugins[plugins_count], "plugins/sigar.rb"); plugins_count++;
  init_plugin_from_file(&plugins[plugins_count], "plugins/ping.rb"); plugins_count++;
  init_plugin_from_file(&plugins[plugins_count], "plugins/snmp.rb"); plugins_count++;
  
#if __FreeBSD__ >= 8
  init_plugin_from_file(&plugins[plugins_count], "plugins/pf.rb"); plugins_count++;
#endif
  
  
  printf("Instanciating output class...\n");
  r_output = mrb_gv_get(mrb, output_gv_sym);
  interval = mrb_fixnum(mrb_funcall(mrb, r_output, "interval", 0));
  
  printf("Interval set to %dms\n", (int)interval);
  
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
  
  if( signal(SIGINT, clean_exit) == SIG_ERR){
    perror("signal");
    exit(1);
  }
  
  while(running){
    int fds[MAX_PLUGINS];
    int maxfd = 0, ai;
    struct timeval tv;
    mrb_value r_buffer;
    struct timeval cycle_started_at, cycle_completed_at;
    
    gettimeofday(&cycle_started_at, NULL);
    
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
              struct timeval answered_at;
              n = read(fds[i], buffer, sizeof(buffer));
              if( n == -1 ){
                if( errno != EAGAIN )
                  perror("read");
                break;
              }
              
              if( n == BUFFER_SIZE ){
                printf("BUFFER_SIZE is too small, increase it ! (value: %d)\n", BUFFER_SIZE);
                continue;
              }
              
              gettimeofday(&answered_at, NULL);
              printf("received answer from %s in %d ms\n", (const char *) plugins[i].mrb->ud,
                  (answered_at.tv_sec - cycle_started_at.tv_sec) * 1000 +
                  (answered_at.tv_usec - cycle_started_at.tv_usec) / 1000
                );
              
              buffer[n] = 0x00;
              
              ai = mrb_gc_arena_save(mrb);
              r_buffer = mrb_str_buf_new(mrb, n);
              mrb_str_buf_cat(mrb, r_buffer, buffer, n);
              
              // mrb_funcall(mrb, r_output, "tick", 0);
              mrb_funcall(mrb, r_output, "add", 1, r_buffer);
              check_exception("add", mrb);
              
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
    check_exception("flush", mrb);
    
    // and now sleep until the next cycle
    gettimeofday(&cycle_completed_at, NULL);
    
  #ifdef _MEM_PROFILER
    if( checkpoint_set ){
      print_allocations();
    }
  #endif
    
    // force a gc run at the end of each cycle
    mrb_full_gc(mrb);
  
  #ifdef _MEM_PROFILER
    checkpoint_set = 1;
    // and set starting point
    profiler_set_checkpoint();
  #endif

    
  #ifdef _MEM_PROFILER_RUBY
    // dump VMS state
    dump_state(mrb);
    for(i= 0; i< plugins_count; i++){
      dump_state(plugins[i].mrb);
    }
  #endif
    
    sleep_delay(&cycle_started_at, &cycle_completed_at, interval);
  }
  
  printf("Canceling all threads...\n");
  strcpy(buffer, "exit");
  for(i= 0; i< plugins_count; i++){
    C_CHECK("send", send(plugins[i].host_pipe, buffer, strlen(buffer), 0) );
  }
  
  for(i= 0; i< plugins_count; i++){
    printf("  - joining thread %d\n", i);
    if( pthread_join(plugins[i].thread, NULL) < 0){
      fprintf(stderr, "join failed\n");
    }
    
    mrb_close(plugins[i].mrb);
  }
  
  mrb_close(mrb);
  
  printf("Exited !\n");
  return 0;
}

