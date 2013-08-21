
#include "d3probe.h"
#include <errno.h>

// static mrb_value plugin_report(mrb_state *mrb, mrb_value self)
// {
//   printf("got a report\n");
  
//   return mrb_nil_value();
// }


static mrb_value plugin_sleep(mrb_state *mrb, mrb_value self)
{
  mrb_float delay;
  
  mrb_get_args(mrb, "f", &delay);
  
  usleep((int)(delay * 1000000));
  
  return mrb_nil_value();
}

static mrb_value plugin_errno(mrb_state *mrb, mrb_value self)
{
  mrb_value r_ret;
  
  r_ret = mrb_str_new_cstr(mrb, strerror(errno));
  
  return r_ret;
}


// static mrb_value register_plugin(mrb_state *mrb, mrb_value self)
// {
//   mrb_value m_name, m_obj;
  
// }


// static mrb_value plugin_pipe(mrb_state *mrb, mrb_value self)
// {
  
// }


void setup_api(mrb_state *mrb)
{
  // class = mrb_define_class(mrb, "D3Probe", NULL);
  // struct RClass *class = mrb_class_get(mrb, "D3Probe");
  struct RClass *kernel = mrb->kernel_module;
  
  
  
  int ai = mrb_gc_arena_save(mrb);
  
  // Kernel
  mrb_define_method(mrb, kernel, "sleep", plugin_sleep, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kernel, "errno_str", plugin_errno, MRB_ARGS_REQ(0));
  
  // D3Probe
  // mrb_define_singleton_method(mrb, class, "report", plugin_report, ARGS_REQ(1));
  // mrb_define_singleton_method(mrb, class, "register_plugin", register_plugin, ARGS_REQ(2));
  
  setup_plugin_api(mrb);
  setup_sigar_api(mrb);
  setup_snmp_api(mrb);
  setup_pf_api(mrb);
  
  mrb_gc_arena_restore(mrb, ai);
}
