#include "d3probe.h"

void pp(mrb_state *mrb, mrb_value obj, int prompt)
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


