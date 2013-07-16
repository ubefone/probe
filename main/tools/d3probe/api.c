
#include "d3probe.h"

static struct RClass *class = NULL;

struct RClass *get_rprobe()
{
  return class;
}

void setup_api(mrb_state *mrb)
{
  // class = mrb_define_class(mrb, "D3Probe", NULL);
  class = mrb_class_get(mrb, "D3Probe");
  
  int ai = mrb_gc_arena_save(mrb);
  
  // mrb_define_method(mrb, class, "initialize", ping_initialize,  ARGS_REQ(1));
  // mrb_define_method(mrb, class, "set_targets", ping_set_targets,  ARGS_REQ(1));
  // mrb_define_method(mrb, class, "add_values", ping_send_pings,  ARGS_REQ(1));
    
  mrb_gc_arena_restore(mrb, ai);
}
