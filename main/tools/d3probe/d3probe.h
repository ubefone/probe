
#include "mruby.h"
#include "mruby/proc.h"
#include "mruby/class.h"
#include "mruby/array.h"
#include "mruby/hash.h"
#include "mruby/data.h"
#include "mruby/string.h"
#include "mruby/compile.h"
#include "mruby/dump.h"
#include "mruby/variable.h"
#include "mruby/numeric.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <pthread.h>
#include <sys/socket.h>
#include <sys/param.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define PROBE_VERSION "1.3.1"

#define C_CHECK(MSG, what) if(what == -1){ perror(MSG);  return -1; }


#define RUBY_ERR(MSG) { mrb_raise(mrb, E_RUNTIME_ERROR, MSG); return self; }
#define RUBY_ERRF(MSG, FORMAT, ARGS...) { mrb_raisef(mrb, E_RUNTIME_ERROR, FORMAT, ## ARGS); return self; }

// debug
void pp(mrb_state *mrb, mrb_value obj, int prompt);


// run aruby code (exec.c)
int execute_compiled_file(mrb_state *mrb, const char *path);
int execute_file(mrb_state *mrb, const char *path);
int execute_string(mrb_state *mrb, const char *code);
int check_exception(const char *msg, mrb_state *mrb);


// api setup
void setup_api(mrb_state *mrb);
void setup_sigar_api(mrb_state *mrb);
void setup_plugin_api(mrb_state *mrb);
void setup_snmp_api(mrb_state *mrb);
void setup_pf_api(mrb_state *mrb);


// gc
void protect_register(mrb_state *mrb, mrb_value v);
void protect_unregister(mrb_state *mrb, mrb_value v);

// profiler
void init_profiler();

#ifdef _MEM_PROFILER
  void* profiler_allocf(mrb_state *mrb, void *p, size_t size, void *ud, const char *file, uint32_t line);
  void profiler_set_checkpoint();
  void print_allocations();
#else
  void* profiler_allocf(mrb_state *mrb, void *p, size_t size, void *ud);
#endif

#ifdef _MEM_PROFILER_RUBY
  void dump_state(mrb_state *mrb);
#endif


typedef struct {
  mrb_value p;
  mrb_state *mrb;
  mrb_value plugin_pipe;
  mrb_value plugin_obj;
  pthread_t thread;
  int       host_pipe;
  
} Plugin;
