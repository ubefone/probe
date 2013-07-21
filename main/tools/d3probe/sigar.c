
#include "d3probe.h"

#include <sigar.h>
#include <sigar_format.h>

#define MAX_CPUS 8

typedef struct {
  sigar_t     *sigar;
  sigar_cpu_t last;
} sigar_state_t;


static void sigar_state_free(mrb_state *mrb, void *ptr)
{
  sigar_state_t *state = (sigar_state_t *)ptr;
  
  sigar_close( state->sigar );
  mrb_free(mrb, ptr);
}

static struct mrb_data_type sigar_state = { "Sigar", sigar_state_free };


static mrb_value _sigar_init(mrb_state *mrb, mrb_value self)
{
  int ret;
  sigar_state_t *state = mrb_malloc(mrb, sizeof(sigar_state_t));
  
  SCHECK( sigar_open(&state->sigar) );
  
  ret = sigar_cpu_get(state->sigar, &state->last);
  
  DATA_PTR(self)  = (void*)state;
  DATA_TYPE(self) = &sigar_state;
  
  return self;
}

static mrb_value _sigar_cpu_get(mrb_state *mrb, mrb_value self)
{
  int ret;
  sigar_state_t *state = DATA_PTR(self);
  mrb_int cpu_index;
  sigar_cpu_t new;
  sigar_cpu_perc_t cpu_perc;
  
  struct RClass *c = mrb_class_get(mrb, "CPUStruct");
  
  ret = sigar_cpu_get(state->sigar, &new);
  
  ret = sigar_cpu_perc_calculate(&state->last, &new, &cpu_perc);
  memcpy(&state->last, &new, sizeof(sigar_cpu_t));
  
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 9,
      mrb_float_value(mrb, cpu_perc.user * 100),
      mrb_float_value(mrb, cpu_perc.sys * 100),
      mrb_float_value(mrb, cpu_perc.nice * 100),
      mrb_float_value(mrb, cpu_perc.idle * 100),
      mrb_float_value(mrb, cpu_perc.wait * 100),
      mrb_float_value(mrb, cpu_perc.irq * 100),
      mrb_float_value(mrb, cpu_perc.soft_irq * 100),
      mrb_float_value(mrb, cpu_perc.stolen * 100),
      mrb_float_value(mrb, cpu_perc.combined * 100)
    );
}



void setup_sigar_api(mrb_state *mrb)
{
  struct RClass *c = mrb_define_class(mrb, "Sigar", NULL);
  
  mrb_define_method(mrb, c, "initialize", _sigar_init,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "cpu_get", _sigar_cpu_get,  ARGS_REQ(0));
  // mrb_define_singleton_method(mrb, c, "inherited", plugin_inherited, ARGS_REQ(1));
}
