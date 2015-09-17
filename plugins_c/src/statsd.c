#include <sys/select.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mruby.h"
#include "mruby/array.h"
#include "mruby/hash.h"

// statsd example:
// <metric name>:<value>|g
// <metric name>:<value>|c[|@<sample rate>]
// <metric name>:<value>|ms
typedef struct {
  mrb_value counters_base;
  mrb_value counters;
  mrb_value gauges;
} statsd_parser;

typedef enum {
  STATE_KEY,
  STATE_VALUE,
  STATE_TYPE
} parser_state;


static void _statsd_reset(mrb_state *mrb, statsd_parser *state)
{
  state->counters = mrb_funcall(mrb, state->counters_base, "clone", 0);
  state->gauges = mrb_hash_new(mrb);
}


static void _statsd_parser_init(mrb_state *mrb, statsd_parser *state, mrb_value initial_counters)
{
  state->counters_base = initial_counters;
  _statsd_reset(mrb, state);
}

static int _statsd_parser_parse(mrb_state *mrb, statsd_parser *state, const char *buffer, ssize_t len)
{
  int pos = 0;
  char key[20], value[5], type;
  int key_pos = 0, value_pos = 0;
  
  parser_state st = STATE_KEY;
  
  do {
    char c = buffer[pos];
    
    if( (c == ':') && (st == STATE_KEY) ){
      // terminate key string and jump to value
      key[key_pos] = 0x00;
      st = STATE_VALUE;
    }
    else if( (c == '|') && (st == STATE_VALUE) ){
      // terminate value string and jump to type
      value[value_pos] = 0x00;
      st = STATE_TYPE;
    }
    else if( c == '\n' ){
      mrb_int val = atoi(value);
      
      key_pos = 0;
      value_pos = 0;
      st = STATE_KEY;
      
      // set current key in hash and reset everything for next line
      mrb_value r_key = mrb_str_new_cstr(mrb, key);
      
      if( type == 'c' ){
        // check if key exists and add to existing value it it does
        mrb_value r_existing_value = mrb_hash_get(mrb, state->counters, r_key);
        
        if( mrb_nil_p(r_existing_value) ){
          mrb_hash_set(mrb, state->counters, r_key, mrb_fixnum_value(val));
          mrb_hash_set(mrb, state->counters_base, r_key, mrb_fixnum_value(0));
        }
        else {
          mrb_hash_set(mrb, state->counters, r_key, mrb_fixnum_value( mrb_fixnum(r_existing_value) + val) );
        }
      }
      else {
        // for gauge just set the value, discard existing
        mrb_hash_set(mrb, state->gauges, r_key, mrb_fixnum_value(val));
      }
      
    }
    else {
      if( st == STATE_KEY )    key[key_pos++]      = buffer[pos];
      if( st == STATE_VALUE )  value[value_pos++]  = buffer[pos];
      if( st == STATE_TYPE )   type = buffer[pos];
    }
    
  } while( ++pos < len );
  
  return 0;
}

static mrb_value _parser_get_counters(statsd_parser *state){ return state->counters; }
static mrb_value _parser_get_gauges(statsd_parser *state){ return state->gauges; }

static mrb_value _network_loop(mrb_state *mrb, mrb_value self)
{
  mrb_value r_pipe, r_socket, r_block, r_initial_counters;
  int pipe, socket, maxfd, idx;
  statsd_parser parser;
  fd_set rd_set;
  
  mrb_get_args(mrb, "ooH&", &r_pipe, &r_socket, &r_initial_counters, &r_block);
  
  // extract the file descriptors
  pipe = mrb_fixnum( mrb_funcall(mrb, r_pipe, "fileno", 0) );
  socket = mrb_fixnum( mrb_funcall(mrb, r_socket, "fileno", 0) );
  
  if( pipe > socket ){
    maxfd = pipe + 1;
  }
  else {
    maxfd = socket + 1;
  }
  
  _statsd_parser_init(mrb, &parser, r_initial_counters);
  idx = mrb_gc_arena_save(mrb);
  
  // and now the main loop
  while(1){
    ssize_t rcv_size;
    char core_msg[10] = "";
    
    FD_ZERO(&rd_set);
    
    FD_SET(pipe, &rd_set);
    FD_SET(socket, &rd_set);
    
    if( select(maxfd, &rd_set, NULL, NULL, NULL) == -1){
      mrb_raisef(mrb, E_RUNTIME_ERROR, "select failed: %S", mrb_str_new_cstr(mrb, strerror(errno)));
      break;
    }
    
    // select returned successfully, find out why
    
    
    // data received
    if( FD_ISSET(socket, &rd_set) ){
      char buffer[2000];
      
      rcv_size = recv(socket, buffer, sizeof(buffer), 0);
      if( rcv_size <= 0 ){
        mrb_raisef(mrb, E_RUNTIME_ERROR, "[socket] recv failed: %d", rcv_size);
        break;
      }
      
      _statsd_parser_parse(mrb, &parser, buffer, rcv_size);
    }
    
    // message from core
    if( FD_ISSET(pipe, &rd_set) ){
      
      rcv_size = recv(pipe, core_msg, sizeof(core_msg), 0);
      if( rcv_size <= 0 ){
        mrb_raisef(mrb, E_RUNTIME_ERROR, "[pipe] recv failed: %d", rcv_size);
        break;
      }
      
      if( strncmp(core_msg, "exit", 4) == 0 ){
        break;
      }
      else {
        mrb_value r_args = mrb_ary_new_capa(mrb, 3);
        
        mrb_ary_set(mrb, r_args, 0, mrb_str_new_cstr(mrb, core_msg));
        mrb_ary_set(mrb, r_args, 1, _parser_get_counters(&parser));
        mrb_ary_set(mrb, r_args, 2, _parser_get_gauges(&parser));
        
        // core asked for data, run the block with the data we got so far
        mrb_yield(mrb, r_block, r_args);
        
        mrb_gc_arena_restore(mrb, idx);
        
        // reset parser state
        _statsd_reset(mrb, &parser);
      }
    }
    
  }
  
  return mrb_nil_value();
}


void mrb_plugins_c_gem_init(mrb_state* mrb)
{
  struct RClass *mod = mrb_define_module(mrb, "StatsdCPlugin");
  
  mrb_define_module_function(mrb, mod, "network_loop", _network_loop, MRB_ARGS_REQ(3));
}


void mrb_plugins_c_gem_final(mrb_state* mrb)
{
  
}
