
#include "d3probe.h"



static void plugin_state_free(mrb_state *mrb, void *ptr)
{
  mrb_free(mrb, ptr);
}

static struct mrb_data_type plugin_state_type = { "Plugin", plugin_state_free };


static mrb_value plugin_initialize(mrb_state *mrb, mrb_value self)
{
  Plugin *plugin = mrb_malloc(mrb, sizeof(Plugin));
  
  DATA_PTR(self)  = (void*)plugin;
  DATA_TYPE(self) = &plugin_state_type;
  
  return self;
}

static mrb_value plugin_pipe(mrb_state *mrb, mrb_value self)
{
  Plugin *plugin = DATA_PTR(self);
  
  return plugin->plugin_pipe;
}

static mrb_value plugin_operating_system(mrb_state *mrb, mrb_value self)
{
  mrb_value os = mrb_nil_value();
  
#ifdef __OpenBSD__
  os = mrb_str_new_cstr(mrb, "OpenBSD");
#endif
  
  return os;
}

// static mrb_value plugin_inherited(mrb_state *mrb, mrb_value self)
// {
//   mrb_value m_obj;
  
//   mrb_get_args(mrb, "o", &m_obj);
  
//   printf("inherited !\n");
  
//   return self;
// }



void setup_plugin_api(mrb_state *mrb)
{
  struct RClass *plugin_class = mrb_class_get(mrb, "Plugin");
  
  mrb_define_method(mrb, plugin_class, "initialize", plugin_initialize,  MRB_ARGS_REQ(1));
  mrb_define_method(mrb, plugin_class, "pipe", plugin_pipe,  MRB_ARGS_REQ(0));
  mrb_define_method(mrb, plugin_class, "operating_system", plugin_operating_system,  MRB_ARGS_REQ(0));
  // mrb_define_singleton_method(mrb, plugin_class, "inherited", plugin_inherited, MRB_ARGS_REQ(1));
}
