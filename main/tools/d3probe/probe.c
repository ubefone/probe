
#include "d3probe.h"

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



static mrb_value plugin_ms_sleep(mrb_state *mrb, mrb_value self)
{
  mrb_float delay;
  
  mrb_get_args(mrb, "f", &delay);
  
  usleep((int)(delay * 1000));
  
  return mrb_nil_value();
}



static mrb_value plugin_getpid(mrb_state *mrb, mrb_value self)
{
  mrb_int pid = (mrb_int) getpid();
  return mrb_fixnum_value(pid);
}


extern mrb_int interval;
static mrb_value _plugin_cycle_interval(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(interval);
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
  // class = mrb_define_class(mrb, "D3Probe", mrb->object_class);
  // struct RClass *class = mrb_class_get(mrb, "D3Probe");
  struct RClass *kernel = mrb->kernel_module;
  
  
  
  int ai = mrb_gc_arena_save(mrb);
  
  // Kernel
  mrb_define_method(mrb, kernel, "sleep", plugin_sleep, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kernel, "ms_sleep", plugin_ms_sleep, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, kernel, "getpid", plugin_getpid, MRB_ARGS_REQ(0));
  mrb_define_method(mrb, kernel, "cycle_interval", _plugin_cycle_interval, MRB_ARGS_REQ(0));
  
  
  // D3Probe
  // mrb_define_singleton_method(mrb, class, "report", plugin_report, ARGS_REQ(1));
  // mrb_define_singleton_method(mrb, class, "register_plugin", register_plugin, ARGS_REQ(2));
  
  setup_plugin_api(mrb);
  setup_sigar_api(mrb);
  setup_snmp_api(mrb);
  setup_pf_api(mrb);
  
  mrb_gc_arena_restore(mrb, ai);
}
