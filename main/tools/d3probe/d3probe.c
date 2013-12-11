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
  plugin->mrb = mrb_open_allocf(profiler_allocf, (void *)path);
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
  mrb_sym plugin_iv_sym = mrb_intern(plugin->mrb, "@plugin", 7);
  
  plugin->plugin_obj = mrb_iv_get(plugin->mrb, mrb_obj_value(plugin->mrb->top_self), plugin_iv_sym);
  // pp(plugin->mrb, plugin->plugin_obj, 0);
  
  // associates the c structure with the ruby objetc
  DATA_PTR(plugin->plugin_obj)  = (void*)plugin;
  
  return 0;
}

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

static void fill_timeout(struct timeval *tv, struct timeval started_at, uint64_t duration)
{
  struct timeval tmp1, tmp2;
  
  tv->tv_sec = 0;
  while( duration >= 1000 ){
    duration -= 1000;
    tv->tv_sec += 1;
  }
  
  tv->tv_usec = duration * 1000;
  
  // substract the time we alreay waited
  gettimeofday(&tmp1, NULL);
  // first: <current time> - <cycle start time>
  timeval_subtract(&tmp2, &tmp1, &started_at);
  // then: <interval time> - (<current time> - <cycle start time>)
  timeval_subtract(&tmp1, tv, &tmp2);
  memcpy(tv, &tmp1, sizeof(tmp1));
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
    printf("warning: loop execution exceeded interval ! (%u ms)\n", (uint32_t)(elapsed_useconds / 1000));
  }
}

static void really_sleep(uint32_t duration_ms){
  int rc;
  struct timespec rem, req;
  
  req.tv_sec = 0;
  while( duration_ms >= 1000 ){
    duration_ms -= 1000;
    req.tv_sec += 1;
  }
  
  req.tv_nsec = duration_ms * 1000000;
  
  // pause until time elapsed (restart if necessary)
  // req=duration_ms;
  do {
    rc= nanosleep(&req,&rem);
    req=rem;
  }
  while (rc==-1 && errno==EINTR);
  // here, rc should be 0
  // if not, parameters to nanosleep() were invalid (check errno)

}

static uint8_t running = 1;

void clean_exit(int sig)
{
  puts("Exiting...");
  running = 0;
}

int main(int argc, char const *argv[])
{
#ifdef _MEM_PROFILER
  uint8_t checkpoint_set = 0;
#endif
  fd_set rfds;
  char buffer[BUFFER_SIZE];
  int i, n;
  Plugin plugins[MAX_PLUGINS];
  int plugins_count = 0;
  mrb_state *mrb;
  mrb_value r_output, r_plugins_list;
  mrb_sym output_gv_sym, plugins_to_load_gv_sym;
  mrb_int interval;
  
  if( argc != 2 ){
    printf("Usage: %s <config_path>\n", argv[0]);
    exit(1);
  }
  
#ifdef _MEM_PROFILER
  init_profiler();
#endif
  
  config_path = argv[1];
  
  printf("Initializing core...\n");
  mrb = mrb_open_allocf(profiler_allocf, "main");
  output_gv_sym = mrb_intern(mrb, "$output", 7);
  plugins_to_load_gv_sym = mrb_intern(mrb, "$plugins_to_load", 16);
  setup_api(mrb);
  execute_file(mrb, "plugins/main.rb");
  execute_file(mrb, config_path);
  
  printf("Loading plugins...\n");
  r_plugins_list = mrb_gv_get(mrb, plugins_to_load_gv_sym);
  for(i = 0; i< mrb_ary_len(mrb, r_plugins_list); i++){
    char path[30];
    mrb_value r_plugin_name = mrb_ary_ref(mrb, r_plugins_list, i);
    const char *plugin_name = mrb_string_value_cstr(mrb, &r_plugin_name);
    
    snprintf(path, sizeof(path) - 1, "plugins/%s.rb", plugin_name);
    
    if( access(path, F_OK) == -1 ){
      printf("cannot open plugin file \"%s\": %s\n", path, strerror(errno));
      exit(1);
    }
    
    init_plugin_from_file(&plugins[plugins_count], path); plugins_count++;
  }
  
  printf("Instanciating output class...\n");
  r_output = mrb_gv_get(mrb, output_gv_sym);
  interval = mrb_fixnum(mrb_funcall(mrb, r_output, "interval", 0));
  
  printf("Interval set to %dms\n", (int)interval);
  
  printf("Sending initial report...\n");
  mrb_funcall(mrb, r_output, "send_report", 0);
  
  if (mrb->exc) {
    mrb_print_error(mrb);
    
    exit(1);
  }

  
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
      
      if( !running || (0 == left) )
        break;
      
      // substract 20ms to stay below the loop delay
      fill_timeout(&tv, cycle_started_at, interval - 20);
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
              printf("received answer from %s in %u ms\n", (const char *) plugins[i].mrb->ud,
                  (uint32_t)((answered_at.tv_sec - cycle_started_at.tv_sec) * 1000 +
                  (answered_at.tv_usec - cycle_started_at.tv_usec) / 1000)
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
        printf("no responses received from %d plugins.\n", left);
        break;
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
  
  printf("Sending exit signal to all plugins...\n");
  strcpy(buffer, "exit");
  for(i= 0; i< plugins_count; i++){
    C_CHECK("send", send(plugins[i].host_pipe, buffer, strlen(buffer), 0) );
  }
  
  printf("Giving some time for threads to exit...\n\n");
  really_sleep(2000);
  
  
  for(i= 0; i< plugins_count; i++){
    int ret = pthread_kill(plugins[i].thread, 0);
    
    // if a success is returned then the thread is still alive
    // which means the thread did not acknoledged the exit message
    // kill it.
    if( ret == 0 ){
      printf("    - plugin \"%s\" failed to exit properly, killing it...\n", (const char *) plugins[i].mrb->ud);
      pthread_cancel(plugins[i].thread);
    }
    else {
      printf("    - plugin \"%s\" exited properly.\n", (const char *) plugins[i].mrb->ud);
    }
    
    if( pthread_join(plugins[i].thread, NULL) < 0){
      fprintf(stderr, "join failed\n");
    }
    
    mrb_close(plugins[i].mrb);
  }
  
  mrb_close(mrb);
  
  printf("Exited !\n");
  return 0;
}

