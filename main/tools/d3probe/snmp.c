#include "d3probe.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>


static void snmp_state_free(mrb_state *mrb, void *ptr)
{
  snmp_close((struct snmp_session*) ptr);
}

static struct mrb_data_type snmp_state = { "Snmp", snmp_state_free };



static mrb_value _snmp_init(mrb_state *mrb, mrb_value self)
{
  struct snmp_session temp, *session ;
  
  snmp_sess_init(&temp);
  temp.version = SNMP_VERSION_1;
  temp.community = (u_char*) "public";
  temp.community_len = strlen((char*) temp.community);
  temp.peername = "127.0.0.1";
  session = snmp_open(&temp);
  
  DATA_PTR(self)  = (void*)session;
  DATA_TYPE(self) = &snmp_state;
  
  return self;
}

static mrb_value _snmp_select(mrb_state *mrb, mrb_value self)
{
  // mrb_value r_snmp;
  
  // mrb_get_args(mrb, "A", &r_snmp);
  
  while (1) {
    int fds = 0, block = 0;
    fd_set fdset;
    struct timeval timeout;

    FD_ZERO(&fdset);
    snmp_select_info(&fds, &fdset, &timeout, &block);
    fds = select(fds, &fdset, NULL, NULL, block ? NULL : &timeout);
    if (fds < 0) {
      perror("select failed");
      break;
    } else if (fds == 0) {
      break;
    } else {
      snmp_read(&fdset);
    }
  }
  
  return mrb_nil_value();
}



struct snmp_cb_args {
  mrb_state *mrb;
  mrb_value cb_block;
};

static mrb_value _snmp_sync_response(mrb_state *mrb, struct snmp_pdu *response, int vars_count)
{
  char *val_buf, *name_buf;
  struct variable_list *vars;
  mrb_value r_ret, r_key, r_value;
  
  if( vars_count > 0 ){
    r_ret = mrb_hash_new_capa(mrb, vars_count);
  }
  else {
    r_ret = mrb_hash_new(mrb);
  }
  
  
  for (vars = response->variables; vars != NULL; vars = vars->next_variable) {
    val_buf = (char *)malloc(sizeof(char) * MAX_OID_LEN);
    name_buf = (char *)malloc(sizeof(char) * MAX_OID_LEN);
    snprint_objid(name_buf, MAX_OID_LEN, vars->name, vars->name_length);
    snprint_value(val_buf, MAX_OID_LEN, vars->name, vars->name_length, vars);
    
    r_key = mrb_str_buf_new(mrb, strlen(name_buf));
    mrb_str_buf_cat(mrb, r_key, name_buf, strlen(name_buf));
    
    r_value = mrb_str_buf_new(mrb, strlen(val_buf));
    mrb_str_buf_cat(mrb, r_value, val_buf, strlen(val_buf));
    
    mrb_hash_set(mrb, r_ret, r_key, r_value);
    
    // printf("%s  ::  %s\n", name_buf, val_buf);
  }

  return r_ret;
}



static int _snmp_response_cb(int operation, struct snmp_session *sp, int reqid, struct snmp_pdu *pdu, void *data)
{
  struct snmp_cb_args *cb_args = (struct snmp_cb_args*) data;
  mrb_value r_ret;
  mrb_state *mrb = cb_args->mrb;  
  r_ret = _snmp_sync_response(mrb, pdu, 0);
  
  mrb_funcall(mrb, cb_args->cb_block, "call", 1, r_ret);
  mrb_free(mrb, cb_args);
  
  return 0;
}


static mrb_value _snmp_get(mrb_state *mrb, mrb_value self)
{
  struct snmp_session *session = DATA_PTR(self);
  struct snmp_pdu *pdu;
  struct snmp_pdu *response;

  oid oid_name[MAX_OID_LEN];
  size_t oid_len = MAX_OID_LEN;
  
  int status;
  mrb_int i;
  
  
  mrb_value r_oids, r_tmp, r_block;
  
  mrb_get_args(mrb, "A|&", &r_oids, &r_block);
    
  pdu = snmp_pdu_create(SNMP_MSG_GET);
  
  for(i = 0; i< RARRAY_LEN(r_oids); i++){
    printf("i: %d, arr: %d\n", i, RARRAY_LEN(r_oids));
    r_tmp = mrb_ary_ref(mrb, r_oids, i);
    mrb_check_type(mrb, r_tmp, MRB_TT_STRING);
    
    read_objid(RSTRING_PTR(r_tmp), oid_name, &oid_len);
    snmp_add_null_var(pdu, oid_name, oid_len);
  }
  
  
  if( mrb_nil_p(r_block) ){
    // synchronous
    status = snmp_synch_response(session, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR){
      mrb_value r_ret = _snmp_sync_response(mrb, response, RARRAY_LEN(r_oids));
      snmp_free_pdu(response);
      return r_ret;
    }
  }
  else {
    struct snmp_cb_args *cb_args = mrb_malloc(mrb, sizeof(struct snmp_cb_args));
    
    cb_args->mrb      = mrb;
    cb_args->cb_block = r_block;
    
    // asynchronous
    status = snmp_async_send(session, pdu, _snmp_response_cb, (void*)cb_args);
  }

    
  
  return mrb_nil_value();
}



void setup_snmp_api(mrb_state *mrb)
{
  struct RClass *c = mrb_define_class(mrb, "SNMP", NULL);
  
  init_snmp("Probe");
  read_mib("/usr/local/Cellar/net-snmp/5.7.2/share/snmp/mibs/SNMPv2-MIB.txt");
  
  mrb_define_method(mrb, c, "initialize", _snmp_init,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "get", _snmp_get,  ARGS_REQ(1));
  
  mrb_define_singleton_method(mrb, c, "select", _snmp_select, ARGS_REQ(0));
}
