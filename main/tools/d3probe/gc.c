
#include "d3probe.h"


#define PROTECT_GV_NAME "$_safe_store"

static mrb_value get_store(mrb_state *mrb)
{
  mrb_sym gv_name = mrb_intern(mrb, PROTECT_GV_NAME);
  mrb_value r_arr = mrb_gv_get(mrb, gv_name);
  
  if( mrb_nil_p(r_arr) ){
    r_arr = mrb_ary_new(mrb);
    mrb_gv_set(mrb, gv_name, r_arr);
  }
  
  return r_arr;
}


void protect_register(mrb_state *mrb, mrb_value v)
{
  mrb_value r_arr = get_store(mrb);
  
  mrb_ary_push(mrb, r_arr, v);
}


void protect_unregister(mrb_state *mrb, mrb_value v)
{
  mrb_int i;
  mrb_value r_arr = get_store(mrb);
  
  // mrb_bool mrb_obj_equal(mrb_state*, mrb_value, mrb_value);
  for(i= 0; i< RARRAY_LEN(r_arr); i++){
    mrb_value current = mrb_ary_ref(mrb, r_arr, i);
    
    if( mrb_obj_equal(mrb, current, v) ){
      mrb_ary_set(mrb, r_arr, i, mrb_nil_value());
    }
  }
}



