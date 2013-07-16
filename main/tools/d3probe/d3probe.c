
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

void *plugin_thread(void *arg)
{
  
}

int main(int argc, char const *argv[])
{
  mrb_state *mrb = mrb_open();
  
  puts("setup");
  setup_api(mrb);
  puts("load");
  execute_file(mrb, "test.rb");
  // execute_compiled_file(mrb, "test.mrb");
  // execute_string(mrb, "fuck(1)");
  
  puts("after load");
  mrb_value p;
  
  struct RClass *rprobe = get_rprobe();
  mrb_value ret = mrb_funcall(mrb, mrb_obj_value(rprobe), "call_plugins", 0);
  
  if (mrb->exc) {
    if (!mrb_undef_p(ret)) {
      mrb_print_error(mrb);
    }
    
  }
  else {
    pp(mrb, ret, 0);
  }
  
  
  printf("started !\n");
  return 0;
}

