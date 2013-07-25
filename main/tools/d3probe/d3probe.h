
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
#include <stdio.h>
#include <unistd.h>
// #include <stdlib.h>
// #include <string.h>
#include <math.h>

#include <pthread.h>
#include <sys/socket.h>

#define SCHECK(what) if(what == -1){ fprintf(stderr, "error in " #what "\n"); }
#define C_CHECK(MSG, what) if(what == -1){ perror(MSG);  return -1; }


#define RUBY_ERR(MSG) { mrb_raise(mrb, E_RUNTIME_ERROR, MSG); return self; }
#define RUBY_ERRF(MSG, FORMAT, ARGS...) { mrb_raisef(mrb, E_RUNTIME_ERROR, FORMAT, ## ARGS); return self; }

// debug
void pp(mrb_state *mrb, mrb_value obj, int prompt);


// run aruby code (exec.c)
int execute_compiled_file(mrb_state *mrb, const char *path);
int execute_file(mrb_state *mrb, const char *path);
int execute_string(mrb_state *mrb, const char *code);
int check_exception(mrb_state *mrb);


// api setup
void setup_api(mrb_state *mrb);
void setup_sigar_api(mrb_state *mrb);
void setup_plugin_api(mrb_state *mrb);
void setup_snmp_api(mrb_state *mrb);




typedef struct {
  mrb_value p;
  mrb_state *mrb;
  mrb_value plugin_pipe;
  mrb_value plugin_obj;
  pthread_t thread;
  int       host_pipe;
  
} Plugin;
