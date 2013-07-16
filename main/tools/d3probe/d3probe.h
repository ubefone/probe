
#include "mruby.h"
#include "mruby/proc.h"
#include "mruby/class.h"
// #include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/compile.h"
#include "mruby/dump.h"
// #include "mruby/variable.h"
#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

#include <pthread.h>


// run aruby code
int execute_compiled_file(mrb_state *mrb, const char *path);
int execute_file(mrb_state *mrb, const char *path);
int execute_string(mrb_state *mrb, const char *code);


// api setup
void setup_api(mrb_state *mrb);

struct RClass *get_rprobe();




struct Plugin {
  mrb_value p
};
