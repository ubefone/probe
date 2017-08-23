#include "d3probe.h"

// #include <net-snmp/net-snmp-config.h>
// #include <net-snmp/net-snmp-includes.h>


// typedef struct {
//   // session cannot be closed inside callback so we
//   // have to keep track of it and free it on the next cycle...
//   struct snmp_session master_session, *session;
// } snmp_state_t;


// static void snmp_state_free(mrb_state *mrb, void *ptr)
// {
//   snmp_state_t *state = (snmp_state_t *) ptr;
//   free(state->master_session.peername);
  
//   if( state->session ){
//     snmp_close(state->session);
//     state->session = NULL;
//   }
// }

// static struct mrb_data_type snmp_state = { "Snmp", snmp_state_free };



// static mrb_value _snmp_init(mrb_state *mrb, mrb_value self)
// {
//   snmp_state_t *state = mrb_malloc(mrb, sizeof(snmp_state_t));
//   char *address;
  
//   mrb_get_args(mrb, "z", &address);
  
//   state->session = NULL;
  
//   snmp_sess_init(&state->master_session);
//   state->master_session.version = SNMP_VERSION_2c;
//   state->master_session.community = (u_char*) "public";
//   state->master_session.community_len = strlen((char*) state->master_session.community);
//   state->master_session.peername = strdup(address);
  
//   DATA_PTR(self)  = (void*)state;
//   DATA_TYPE(self) = &snmp_state;
  
//   return self;
// }

// static mrb_value _snmp_select(mrb_state *mrb, mrb_value self)
// {
//   while (1) {
//     int fds = 0, block = 0;
//     fd_set fdset;
//     struct timeval timeout;

//     FD_ZERO(&fdset);
//     snmp_select_info(&fds, &fdset, &timeout, &block);
    
//     // printf("block: %d, timeout[%d . %d]\n", block, timeout.tv_sec, timeout.tv_usec);
//     // ignore returned timeout, we only needs the fds
//     timeout.tv_sec = 0;
//     timeout.tv_usec = 200000;
    
//     puts("++ SELECT");
    
//     fds = select(fds, &fdset, NULL, NULL, &timeout);
//     if (fds < 0) {
//       perror("select failed");
//       break;
//     }
//     else if (fds == 0) {
//       snmp_timeout();
//       break;
//     }
//     else {
//       puts("++ before read");
//       snmp_read(&fdset);
//       puts("++ after read");
//     }
//   }
  
//   return mrb_nil_value();
// }



// struct snmp_cb_args {
//   mrb_state *mrb;
//   mrb_value cb_block;
// };

// static mrb_value _snmp_sync_response(mrb_state *mrb, struct snmp_pdu *response, int vars_count)
// {
//   struct variable_list *vars;
//   mrb_value r_ret;
  
//   char *val_buf   = (char *)mrb_malloc(mrb, sizeof(char) * MAX_OID_LEN);
//   char *name_buf  = (char *)mrb_malloc(mrb, sizeof(char) * MAX_OID_LEN);
  
//   if( vars_count > 0 ){
//     r_ret = mrb_hash_new_capa(mrb, vars_count);
//   }
//   else {
//     r_ret = mrb_hash_new(mrb);
//   }
  
//   for (vars = response->variables; vars != NULL; vars = vars->next_variable) {
//     mrb_value r_key, r_value;
//     snprint_objid(name_buf, MAX_OID_LEN, vars->name, vars->name_length);
//     snprint_value(val_buf, MAX_OID_LEN, vars->name, vars->name_length, vars);
    
//     r_key = mrb_str_buf_new(mrb, strlen(name_buf));
//     mrb_str_buf_cat(mrb, r_key, name_buf, strlen(name_buf));
    
//     r_value = mrb_str_buf_new(mrb, strlen(val_buf));
//     mrb_str_buf_cat(mrb, r_value, val_buf, strlen(val_buf));
    
//     mrb_hash_set(mrb, r_ret, r_key, r_value);
    
//     // printf("%s  ::  %s\n", name_buf, val_buf);
//   }
  
//   mrb_free(mrb, name_buf);
//   mrb_free(mrb, val_buf);

//   return r_ret;
// }



// static int _snmp_response_cb(int operation, struct snmp_session *sp, int reqid, struct snmp_pdu *pdu, void *data)
// {
//   if( operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE ){
//     struct snmp_cb_args *cb_args = (struct snmp_cb_args*) data;
//     mrb_value r_ret;
//     mrb_state *mrb = cb_args->mrb;
//     int ai = mrb_gc_arena_save(mrb);
//     r_ret = _snmp_sync_response(mrb, pdu, 0);
    
//     printf("[reqid: %d] snmp_callback, response received\n", reqid);
//     mrb_funcall(mrb, cb_args->cb_block, "call", 1, r_ret);
    
//     protect_unregister(mrb, cb_args->cb_block);
//     mrb_free(mrb, cb_args);
//     mrb_gc_arena_restore(mrb, ai);
    
//     // snmp_close(sp);
//   }
//   else {
//     // just ignore this crap
//     printf("[reqid: %d] snmp_callback, operation: %d\n", reqid, operation);
//   }
  
//   return 0;
// }

// static mrb_value _snmp_load_mib(mrb_state *mrb, mrb_value self)
// {
//   char *path;
  
//   mrb_get_args(mrb, "z", &path);
  
//   printf("Loading MIB definitions %s...\n", path);
  
//   if( read_mib(path) == NULL ){
//     mrb_raisef(mrb, E_ARGUMENT_ERROR, "Unable to load MIB definitions from \"%S\"", mrb_str_new_cstr(mrb, path));
//   }
  
//   return mrb_nil_value();
// }

// static mrb_value _snmp_cleanup(mrb_state *mrb, mrb_value self)
// {
//   snmp_state_t *state = DATA_PTR(self);
  
//   if( state->session ){
//     snmp_close(state->session);
//   }
  
//   return self;
// }

// static mrb_value _snmp_get(mrb_state *mrb, mrb_value self)
// {
//   snmp_state_t *state = DATA_PTR(self);
//   struct snmp_pdu *pdu;
//   int status;
//   mrb_int i;
  
  
//   mrb_value *r_oids, r_block;
//   int r_count;
  
//   puts("snmp_get");
  
//   // mrb_get_args(mrb, "A|&", &r_oids, &r_block);
//   mrb_get_args(mrb, "*|&", &r_oids, &r_count, &r_block);
  
//   if( state->session ){
//     snmp_close(state->session);
//   }
  
//   state->session = snmp_open(&state->master_session);
//   if( state->session == NULL ){
//     snmp_perror("snmp_open");
//     goto error;
//   }
  
//   pdu = snmp_pdu_create(SNMP_MSG_GET);
  
//   for(i = 0; i< r_count; i++){
//     oid oid_name[MAX_OID_LEN];
//     size_t oid_len = MAX_OID_LEN;

//     // printf("i: %d, arr: %d\n", i, r_count);
//     mrb_check_type(mrb, r_oids[i], MRB_TT_STRING);
    
//     read_objid(RSTRING_PTR(r_oids[i]), oid_name, &oid_len);
//     snmp_add_null_var(pdu, oid_name, oid_len);
//   }
  
  
//   if( mrb_nil_p(r_block) ){
//     struct snmp_pdu *response;
    
//     // synchronous
//     status = snmp_synch_response(state->session, pdu, &response);
//     if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR){
//       mrb_value r_ret = _snmp_sync_response(mrb, response, r_count);
//       snmp_free_pdu(response);
//       return r_ret;
//     }
    
//     snmp_close(state->session);
//   }
//   else {
//     struct snmp_cb_args *cb_args = mrb_malloc(mrb, sizeof(struct snmp_cb_args));
    
//     cb_args->mrb      = mrb;
//     cb_args->cb_block = r_block;
    
//     protect_register(mrb, r_block);
    
//     // asynchronous
//     status = snmp_async_send(state->session, pdu, _snmp_response_cb, (void*)cb_args);
//     if( status == 0 ){
//       char *error;
//       int err1, err2;
//       snmp_error(state->session, &err1, &err2, &error);
//       printf("snmp_async_send error: %s\n", error);
//       snmp_free_pdu(pdu);
//     }
//   }

    
// error:
//   return mrb_nil_value();
// }



void setup_snmp_api(mrb_state *mrb)
{
//   struct RClass *c = mrb_define_class(mrb, "SNMP", mrb->object_class);
  
//   init_snmp("Probe");
//   // read_mib("/usr/local/share/snmp/mibs/SNMPv2-MIB.txt");
  
//   mrb_define_method(mrb, c, "initialize", _snmp_init,  MRB_ARGS_REQ(1));
//   mrb_define_method(mrb, c, "get", _snmp_get,  MRB_ARGS_REQ(1));
//   mrb_define_method(mrb, c, "cleanup", _snmp_cleanup,  MRB_ARGS_REQ(0));
  
//   mrb_define_singleton_method(mrb, (struct RObject *)c, "load_mibs", _snmp_load_mib,  MRB_ARGS_REQ(1));
//   mrb_define_singleton_method(mrb, (struct RObject *)c, "select", _snmp_select, MRB_ARGS_REQ(0));
}
