#include "d3probe.h"

uint8_t show_allocs = 0;

static const char *none= "(none)";

#define PROFILER_PATH_SIZE 60

typedef struct {
  const char      *vm;
  char      fname[PROFILER_PATH_SIZE];
  char      ruby_fname[PROFILER_PATH_SIZE];
  uint32_t  line;
  void      *address;
  size_t    size;
  
  void      *next;
} allocation_t;

// /mruby/src/

#ifdef _MEM_PROFILER
static allocation_t *allocations_list = NULL;

static allocation_t *checkpoint = NULL;

static char pwd[MAXPATHLEN] = "";

void init_profiler()
{
  if( getcwd(pwd, sizeof(pwd)) == NULL ){
    perror("getcwd");
  }
  
  printf("pwd: %s\n", pwd);
}

static allocation_t *create_allocation(mrb_state *mrb, const char *path, uint32_t line, void *address, size_t size)
{
  char *tmp = strstr(path, pwd);
  allocation_t *alloc = malloc(sizeof(allocation_t));
  
  if( mrb ){
    if( mrb->c && mrb->c->ci ){
      // from error.c, function: exc_debug_info
      mrb_callinfo *ci = mrb->c->ci;
      mrb_code *pc = ci->pc;
      const char *file = "";
      uint32_t line = 0;
      
      ci--;
      while (ci >= mrb->c->cibase) {
        if (ci->proc && !MRB_PROC_CFUNC_P(ci->proc)) {
          mrb_irep *irep = ci->proc->body.irep;
          
          if (irep->filename && irep->lines && irep->iseq <= pc && pc < irep->iseq + irep->ilen) {
            file = irep->filename;
            line = irep->lines[pc - irep->iseq - 1];
            break;
          }
        }
        pc = ci->pc;
        ci--;
      }
      
      if( file[0] !=  '\0' ){
        char *fname = strstr(file, pwd);
        if( fname ) file = fname;
        snprintf(alloc->ruby_fname, sizeof(alloc->ruby_fname) - 1, "%s:%d", file, line);
      }
      else {
        alloc->ruby_fname[0] = '\0';
      }
    }

    alloc->vm = (const char *)mrb->ud;
  }
  else {
    alloc->vm = none;
  }
  alloc->line = line;
  alloc->address = address;
  alloc->size = size;
  alloc->next = NULL;
  
  if( tmp == NULL ){
    // printf("create_allocations strstr: %s\n", path);
    tmp = path;
  }
  
  strncpy(alloc->fname, tmp, sizeof(alloc->fname) - 1);
  
  return alloc;
}

static void destroy_allocation(allocation_t *item)
{
  free(item);
}

static allocation_t *allocations_list_tip()
{
  allocation_t *tip = allocations_list;
  while( tip->next != NULL ){
    tip = tip->next;
  }
  
  return tip;
}

void profiler_set_checkpoint()
{
  checkpoint = allocations_list_tip();
}

void print_allocations()
{
  allocation_t *curr = checkpoint;
  while( curr != NULL ){
    
    if( curr->ruby_fname[0] != '\0' ){
      printf("[%s][ruby] %s : %d bytes\n", curr->vm, curr->ruby_fname, curr->size);
    }
    else {
      printf("[%s][C] %s:%d : %d bytes\n", curr->vm, curr->fname, curr->line, curr->size);
    }
    
    curr = curr->next;
  }
}


void* profiler_allocf(mrb_state *mrb, void *p, size_t size, void *ud, const char *file, uint32_t line)
{ 
  if (size == 0) {
    allocation_t *prev = NULL;
    allocation_t *curr = allocations_list;
    // find associated entry
    while( (curr != NULL) && (curr->address != p) ){
      prev = curr;
      curr = curr->next;
    }
    
    if( curr != NULL ){
      // printf("[%s][%s:%d] free()\n", (const char *)ud, file, line);
      if( prev ){
        prev->next = curr->next;
      }
      else {
        allocations_list = NULL;
      }
      
      destroy_allocation(curr);
    }
    else {
      // printf("allocation not found: %s:%d !!! \n", file, line);
    }
    
    free(p);
    return NULL;
  }
  else {
    void *addr = realloc(p, size);
    allocation_t *curr = create_allocation(mrb, file, line, addr, size);
    
    if( allocations_list == NULL ){
      allocations_list = curr;
    }
    else {
      // find tip
      allocation_t *tip = allocations_list_tip();
      // and add it at the end
      tip->next = curr;
    }
    
    // printf("[%s][%s:%d] malloc(%zd)\n", (const char *)ud, file, line, size);
    // print_backtrace();
    return addr;
  }
}

#else

void* profiler_allocf(mrb_state *mrb, void *p, size_t size, void *ud)
{
  if (size == 0) {
    free(p);
    return NULL;
  }
  else {
    return realloc(p, size);
  }
}

#endif // _MEM_PROFILER


#ifdef _MEM_PROFILER_RUBY

// #include <execinfo.h>

// void print_backtrace()
// {
//   void* callstack[20];
//   int i, frames = backtrace(callstack, 20);
//   char** strs = backtrace_symbols(callstack, frames);
  
//   for(i = 0; i < frames; ++i) {
//     // printf("%s\n", strs[i]);
//   }
  
//   free(strs);
// }

void dump_state(mrb_state *mrb)
{
  mrb_full_gc(mrb);
  printf("\nVM: %s\n", (const char *)mrb->ud);
  mrb_funcall(mrb, mrb_obj_value(mrb->top_self), "print_memory_state", 0);
}

#endif
