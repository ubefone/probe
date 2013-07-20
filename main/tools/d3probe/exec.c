
#include "d3probe.h"

int execute_compiled_file(mrb_state *mrb, const char *path)
{
  int n;
  FILE *script;
  
  script = fopen(path, "r");
  
  if ( script == NULL){
    fprintf(stderr, "File not found.\n");
    return -1;
  }
  
  n = mrb_read_irep_file(mrb, script);
  if (n < 0) {
    fprintf(stderr, "failed to load mrb file.\n");
    return -1;
  }
  
  mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));
  if (mrb->exc) {
    mrb_print_error(mrb);
    return -1;
  }
  
  fclose(script);
  return 0;
}



int execute_file(mrb_state *mrb, const char *path)
{
  mrbc_context *c;
  mrb_value v;
  FILE *script;
  
  script = fopen(path, "r");
  if( !script ){
    fprintf(stderr, "File Not found: %s\n", path);
    return -1;
  }
  
  c = mrbc_context_new(mrb);
  v = mrb_load_file_cxt(mrb, script, c);
  mrbc_context_free(mrb, c);
  
  if (mrb->exc) {
    if (!mrb_undef_p(v)) {
      mrb_print_error(mrb);
    }
    
    return -1;
  }
  
  fclose(script);
  return 0;
}


int execute_string(mrb_state *mrb, const char *code)
{
  mrbc_context *c;
  mrb_value v;
  
  c = mrbc_context_new(mrb);
  v = mrb_load_string_cxt(mrb, code, c);
  
  mrbc_context_free(mrb, c);
  
  if (mrb->exc) {
    if (!mrb_undef_p(v)) {
      mrb_print_error(mrb);
    }
    
    return -1;
  }
  
  return 0;
}
