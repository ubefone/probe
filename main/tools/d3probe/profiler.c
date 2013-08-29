#include "d3probe.h"

uint8_t show_allocs = 0;


void* profiler_allocf(mrb_state *mrb, void *p, size_t size, void *ud)
{
  if (size == 0) {
    // printf("[%s] free()\n", (const char *)ud);
    free(p);
    return NULL;
  }
  else {
    // if( show_allocs && (size > 1000) ){
      // printf("[%s] malloc(%zd)\n", (const char *)ud, size);
      // print_backtrace();
    // }
    return realloc(p, size);
  }
}


#ifdef MEMORY_PROFILE

#include <execinfo.h>

void print_backtrace()
{
  void* callstack[20];
  int i, frames = backtrace(callstack, 20);
  char** strs = backtrace_symbols(callstack, frames);
  
  for(i = 0; i < frames; ++i) {
    // printf("%s\n", strs[i]);
  }
  
  free(strs);
}

void dump_state(mrb_state *mrb)
{
  mrb_full_gc(mrb);
  printf("\nVM: %s\n", (const char *)mrb->ud);
  mrb_funcall(mrb, mrb_obj_value(mrb->top_self), "print_memory_state", 0);
}

#endif
