#if __FreeBSD__ >= 8 || defined(__OpenBSD__)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/pfvar.h>

#include "d3probe.h"

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


static int _pf_stats_recursive(mrb_state *mrb, const char *anchor, mrb_value klass, int ai, mrb_value r_ret)
{
  int rules_count;
  mrb_int i;
  struct pfioc_rule pr;
  
  bzero(&pr, sizeof(pr));
  
  if( anchor ){
    strlcpy(pr.anchor, anchor, MAXPATHLEN);
  }
  
  if( ioctl(dev, DIOCGETRULES, &pr) )
    ERROUT("Unable to list rules");
  
  rules_count = pr.nr;
  
  for( i = 0; i< rules_count; i++){
    mrb_value r_tmp;
    pr.nr = i;
    if( ioctl(dev, DIOCGETRULE, &pr) )
      ERROUTF("Unable to query rule %d", i);
    
    if( pr.rule.label[0] ){
      r_tmp = mrb_funcall(mrb, klass, "new", 6,
          mrb_str_new_cstr(mrb, pr.rule.label),
          mrb_fixnum_value(pr.rule.evaluations),
          mrb_fixnum_value(pr.rule.packets[0]),
          mrb_fixnum_value(pr.rule.bytes[0]),
          mrb_fixnum_value(pr.rule.packets[1]),
          mrb_fixnum_value(pr.rule.bytes[1])
        );
      
      mrb_ary_push(mrb, r_ret, r_tmp);
      mrb_gc_arena_restore(mrb, ai);
    }
    else if( pr.anchor_call[0] ) {
      _pf_stats_recursive(mrb, pr.anchor_call, klass, ai, r_ret);
    }
  }
  
  return 0;
  
error:
  return -1;
}


static mrb_value _pf_stats(mrb_state *mrb, mrb_value self)
{
  int ai;
  mrb_value r_ret;
  struct RClass *c = mrb_class_get(mrb, "PacketFilterLabelStats");
  
  r_ret = mrb_ary_new(mrb);
  
  ai = mrb_gc_arena_save(mrb);
  _pf_stats_recursive(mrb, NULL, mrb_obj_value(c), ai, r_ret);
  return r_ret;
}

void setup_pf_api(mrb_state *mrb)
{
  struct RClass *c = mrb_define_class(mrb, "PacketFilter", NULL);
  
  mrb_define_method(mrb, c, "initialize", _pf_init,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "_stats", _pf_stats,  ARGS_REQ(0));
}

#else

#include "d3probe.h"

void setup_pf_api(mrb_state *mrb){
  
}

#endif
