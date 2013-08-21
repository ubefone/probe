#include "d3probe.h"

#if __FreeBSD__ >= 8

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/pfvar.h>


static int dev = -1;

#define ERROUT(MSG) { puts(MSG); goto error; }
#define ERROUTF(FORMAT, ARGS...) { printf(FORMAT "\n", ## ARGS); goto error; }

static mrb_value _pf_init(mrb_state *mrb, mrb_value self)
{
  
  if( dev == -1 ){
    dev = open("/dev/pf", O_RDONLY);
    if( dev == -1 )
      ERROUT("Unable to open device");
  }
  
error:
  return mrb_nil_value();
}

static mrb_value _pf_stats(mrb_state *mrb, mrb_value self)
{
  int ai, rules_count;
  mrb_int i;
  mrb_value r_ret;
  struct pfioc_rule pr;
  struct RClass *c = mrb_class_get(mrb, "PacketFilterLabelStats");
  
  if( ioctl(dev, DIOCGETRULES, &pr) )
    ERROUT("Unable to list rules");
  
  rules_count = pr.nr;
  r_ret = mrb_ary_new_capa(mrb, rules_count);
  
  ai = mrb_gc_arena_save(mrb);
  
  for( i =0; i< rules; i++){
    mrb_value r_tmp;
    pr.nr = i;
    if( ioctl(dev, DIOCGETRULE, &pr) )
      ERROUTF("Unable to query rule %d", i);
    
    r_tmp = mrb_funcall(mrb, mrb_obj_value(c), "new", 6,
        mrb_str_new_cstr(mrb, pr.rule.label),
        mrb_fixnum_value(pr.rule.evaluations),
        mrb_fixnum_value(pr.rule.packets[0]),
        mrb_fixnum_value(pr.rule.bytes[0]),
        mrb_fixnum_value(pr.rule.packets[1]),
        mrb_fixnum_value(pr.rule.bytes[1])
      );
    
    mrb_ary_set(mrb, r_ret, i, r_tmp);
    
    
    mrb_gc_arena_restore(mrb, ai);
  }
  
  return r_ret;
  
error:
  return mrb_nil_value();
}

void setup_pf_api(mrb_state *mrb)
{
  struct RClass *c = mrb_class_get(mrb, "PacketFilter");
  
  mrb_define_method(mrb, c, "initialize", _pf_init,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "_stats", _pf_stats,  ARGS_REQ(0));
}

#else

void setup_pf_api(mrb_state *mrb){
  
}

#endif
