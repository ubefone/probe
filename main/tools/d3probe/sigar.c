
#include "d3probe.h"

#include <sigar.h>
#include <sigar_format.h>

#include <arpa/inet.h>

// #define MAX_CPUS 8
#define CHK(WHAT) if(WHAT == -1){ mrb_raise(mrb, E_RUNTIME_ERROR, "error: " #WHAT); return mrb_nil_value(); }

#define CONVERT_KB(VALUE) mrb_fixnum_value(VALUE / 1024.0)
#define CONVERT_MB(VALUE) mrb_fixnum_value(VALUE / (1024.0*1024))
#define CONVERT_GB(VALUE) mrb_fixnum_value(VALUE / (1024.0*1024*1024))


typedef struct {
  sigar_t     *sigar;
  sigar_cpu_t last;
} sigar_state_t;


static void sigar_state_free(mrb_state *mrb, void *ptr)
{
  sigar_state_t *state = (sigar_state_t *)ptr;
  
  sigar_close( state->sigar );
  mrb_free(mrb, ptr);
}

static struct mrb_data_type sigar_state = { "Sigar", sigar_state_free };


static mrb_value _sigar_init(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = mrb_malloc(mrb, sizeof(sigar_state_t));
  
  CHK( sigar_open(&state->sigar) );
  
  CHK( sigar_cpu_get(state->sigar, &state->last) );
  
  DATA_PTR(self)  = (void*)state;
  DATA_TYPE(self) = &sigar_state;
  
  return self;
}


/////////////////////////
// Static informations
//////////////////////


static mrb_value _sigar_cpus_info_get(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_value r_ret;
  sigar_cpu_info_list_t cpu_infos;
  sigar_state_t *state = DATA_PTR(self);
  struct RClass *c = mrb_class_get(mrb, "CPUInfoStruct");
  
  CHK( sigar_cpu_info_list_get(state->sigar, &cpu_infos) );
  
  r_ret = mrb_ary_new_capa(mrb, cpu_infos.number);
  
  for(i= 0; i< cpu_infos.number; i++){
    mrb_value r_cpu = mrb_funcall(mrb, mrb_obj_value(c), "new", 5,
        mrb_str_new_cstr(mrb, cpu_infos.data[i].vendor),
        mrb_str_new_cstr(mrb, cpu_infos.data[i].model),
        mrb_fixnum_value(cpu_infos.data[i].mhz),
        // mrb_fixnum_value(cpu_infos.data[i].mhz_min),
        // mrb_fixnum_value(cpu_infos.data[i].mhz_max),
        mrb_fixnum_value(cpu_infos.data[i].cache_size),
        // mrb_fixnum_value(cpu_infos.data[i].total_sockets),
        mrb_fixnum_value(cpu_infos.data[i].total_cores)
        // mrb_fixnum_value(cpu_infos.data[i].cores_per_socket)
      );
    
    mrb_ary_push(mrb, r_ret, r_cpu);
  }
  
  sigar_cpu_info_list_destroy(state->sigar, &cpu_infos);
  
  return r_ret;
}


static mrb_value _sigar_filesystems(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_value r_ret;
  sigar_state_t *state = DATA_PTR(self);
  sigar_file_system_list_t fs_list;
  
  struct RClass *c = mrb_class_get(mrb, "FSStruct");

  CHK( sigar_file_system_list_get(state->sigar, &fs_list) );
  
  r_ret = mrb_ary_new_capa(mrb, fs_list.number);
  
  for(i= 0; i< fs_list.number; i++){
    mrb_value r_fs = mrb_funcall(mrb, mrb_obj_value(c), "new", 6,
        mrb_str_new_cstr(mrb, fs_list.data[i].dir_name),
        mrb_str_new_cstr(mrb, fs_list.data[i].dev_name),
        mrb_str_new_cstr(mrb, fs_list.data[i].type_name),
        mrb_str_new_cstr(mrb, fs_list.data[i].sys_type_name),
        mrb_str_new_cstr(mrb, fs_list.data[i].options),
        mrb_fixnum_value(fs_list.data[i].flags)
      );
    
    mrb_ary_push(mrb, r_ret, r_fs);
  }
  
  sigar_file_system_list_destroy(state->sigar, &fs_list);
  
  return r_ret;
}


static mrb_value _ip_address(mrb_state *mrb, sigar_net_address_t address)
{
  char str_addr[INET6_ADDRSTRLEN];
  mrb_value r_family, r_addr;
  struct RClass *c = mrb_class_get(mrb, "NetworkAddressStruct");
  
  if( address.family ==  SIGAR_AF_INET){
    r_family = mrb_str_new_cstr(mrb, "inet");
    
    inet_ntop(AF_INET, address.addr.in6, str_addr, sizeof(str_addr));
    r_addr = mrb_str_new_cstr(mrb, str_addr);
    
  }
  else if( address.family == SIGAR_AF_INET6 ){
    r_family = mrb_str_new_cstr(mrb, "inet6");
    
    inet_ntop(AF_INET6, address.addr.in6, str_addr, sizeof(str_addr));
    r_addr = mrb_str_new_cstr(mrb, str_addr);
  }
  else if( address.family == SIGAR_AF_LINK ){
    r_family = mrb_str_new_cstr(mrb, "link");
    r_addr = mrb_nil_value();
  }
  else {
    printf("family: %d\n", address.family);
    r_family = mrb_str_new_cstr(mrb, "unknown");
    r_addr = mrb_nil_value();
  }
  
  
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 2,
      r_family,
      r_addr
    );
}

static mrb_value _sigar_net_routes(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_value r_ret;
  sigar_state_t *state = DATA_PTR(self);
  sigar_net_route_list_t routes;
  int ai;
  
  struct RClass *c = mrb_class_get(mrb, "NetworkRouteStruct");

  CHK( sigar_net_route_list_get(state->sigar, &routes) );
  
  r_ret = mrb_ary_new_capa(mrb, routes.number);
  
  ai = mrb_gc_arena_save(mrb);
  
  for(i= 0; i< routes.number; i++){
    mrb_value r_fs = mrb_funcall(mrb, mrb_obj_value(c), "new", 6,
        _ip_address(mrb, routes.data[i].destination),
        _ip_address(mrb, routes.data[i].gateway),
        _ip_address(mrb, routes.data[i].mask),
        mrb_fixnum_value(routes.data[i].flags),
        mrb_fixnum_value(routes.data[i].refcnt),
        mrb_fixnum_value(routes.data[i].use),
        mrb_fixnum_value(routes.data[i].metric),
        mrb_fixnum_value(routes.data[i].mtu),
        mrb_fixnum_value(routes.data[i].window),
        mrb_fixnum_value(routes.data[i].irtt)
      );
    
    mrb_ary_push(mrb, r_ret, r_fs);
    mrb_gc_arena_restore(mrb, ai);
  }
  
  sigar_net_route_list_destroy(state->sigar, &routes);
  
  return r_ret;
}

static mrb_value _sigar_net_infos(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = DATA_PTR(self);
  sigar_net_info_t net_infos;
  
  struct RClass *c = mrb_class_get(mrb, "NetworkInfosStruct");

  CHK( sigar_net_info_get(state->sigar, &net_infos) );
  
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 5,
      mrb_str_new_cstr(mrb, net_infos.default_gateway),
      mrb_str_new_cstr(mrb, net_infos.default_gateway_interface),
      mrb_str_new_cstr(mrb, net_infos.host_name),
      mrb_str_new_cstr(mrb, net_infos.primary_dns),
      mrb_str_new_cstr(mrb, net_infos.secondary_dns)
    );
}

static mrb_value _sigar_sysinfo(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = DATA_PTR(self);
  sigar_sys_info_t infos;
  
  struct RClass *c = mrb_class_get(mrb, "SysInfoStruct");

  CHK( sigar_sys_info_get(state->sigar, &infos) );
  
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 10,
      mrb_str_new_cstr(mrb, infos.name),
      mrb_str_new_cstr(mrb, infos.version),
      mrb_str_new_cstr(mrb, infos.arch),
      mrb_str_new_cstr(mrb, infos.machine),
      mrb_str_new_cstr(mrb, infos.description),
      mrb_str_new_cstr(mrb, infos.patch_level),
      mrb_str_new_cstr(mrb, infos.vendor),
      mrb_str_new_cstr(mrb, infos.vendor_version),
      mrb_str_new_cstr(mrb, infos.vendor_name),
      mrb_str_new_cstr(mrb, infos.vendor_code_name)
    );
}

static mrb_value _sigar_net_interfaces(mrb_state *mrb, mrb_value self)
{
  int i, ai;
  mrb_value r_ret;
  sigar_state_t *state = DATA_PTR(self);
  sigar_net_interface_list_t iflist;
  
  CHK( sigar_net_interface_list_get(state->sigar, &iflist) );

  r_ret = mrb_ary_new_capa(mrb, iflist.number);
  
  ai = mrb_gc_arena_save(mrb);
  for(i = 0; i< iflist.number; i++){
    mrb_value r_str = mrb_str_new_cstr(mrb, iflist.data[i]);
    mrb_ary_push(mrb, r_ret, r_str);
    mrb_gc_arena_restore(mrb, ai);
  }
  
  sigar_net_interface_list_destroy(state->sigar, &iflist);
  
  return r_ret;
}


/////////////////////////
// Dynamic informations
//////////////////////

static mrb_value _sigar_cpu_get(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = DATA_PTR(self);
  sigar_cpu_t new;
  sigar_cpu_perc_t cpu_perc;
  
  struct RClass *c = mrb_class_get(mrb, "CPUStruct");
  
  CHK( sigar_cpu_get(state->sigar, &new) );
  CHK( sigar_cpu_perc_calculate(&state->last, &new, &cpu_perc) );
  memcpy(&state->last, &new, sizeof(sigar_cpu_t));
  
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 9,
      mrb_float_value(mrb, cpu_perc.user * 100),
      mrb_float_value(mrb, cpu_perc.sys * 100),
      mrb_float_value(mrb, cpu_perc.nice * 100),
      mrb_float_value(mrb, cpu_perc.idle * 100),
      mrb_float_value(mrb, cpu_perc.wait * 100),
      mrb_float_value(mrb, cpu_perc.irq * 100),
      mrb_float_value(mrb, cpu_perc.soft_irq * 100),
      mrb_float_value(mrb, cpu_perc.stolen * 100),
      mrb_float_value(mrb, cpu_perc.combined * 100)
    );
}

static mrb_value _sigar_mem_get(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = DATA_PTR(self);
  sigar_mem_t mem;
  
  struct RClass *c = mrb_class_get(mrb, "MemStruct");
  
  CHK( sigar_mem_get(state->sigar, &mem) );
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 8,
      mrb_fixnum_value(mem.ram),
      CONVERT_KB(mem.total),
      CONVERT_KB(mem.used),
      CONVERT_KB(mem.free),
      CONVERT_KB(mem.actual_used),
      CONVERT_KB(mem.actual_free),
      mrb_float_value(mrb, mem.used_percent),
      mrb_float_value(mrb, mem.free_percent)
    );
}


static mrb_value _sigar_swap_get(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = DATA_PTR(self);
  sigar_swap_t swap;
  
  struct RClass *c = mrb_class_get(mrb, "SwapStruct");
  
  CHK( sigar_swap_get(state->sigar, &swap) );
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 3,
      CONVERT_MB(swap.used),
      CONVERT_MB(swap.page_in),
      CONVERT_MB(swap.page_out)
    );
}

static mrb_value _sigar_loadavg(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = DATA_PTR(self);
  sigar_loadavg_t load;
  
  struct RClass *c = mrb_class_get(mrb, "LoadAvgStruct");
  
  CHK( sigar_loadavg_get(state->sigar, &load) );

  return mrb_funcall(mrb, mrb_obj_value(c), "new", 3,
      mrb_float_value(mrb, load.loadavg[0]),
      mrb_float_value(mrb, load.loadavg[1]),
      mrb_float_value(mrb, load.loadavg[2])
    );
}

// TODO: find units on linux
static mrb_value _sigar_proc_mem(mrb_state *mrb, mrb_value self)
{
  mrb_int pid;
  sigar_state_t *state = DATA_PTR(self);
  sigar_proc_mem_t mem;
  
  struct RClass *c = mrb_class_get(mrb, "ProcMemStruct");
  
  mrb_get_args(mrb, "i", &pid);
  
  // printf("mem: %ld\n", (mem.size / (1024*1024*1024)));
  
  CHK( sigar_proc_mem_get(state->sigar, pid, &mem) );
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 6,
      CONVERT_KB(mem.size),
      CONVERT_KB(mem.resident),
      CONVERT_KB(mem.share),
      CONVERT_KB(mem.minor_faults),
      CONVERT_KB(mem.major_faults),
      CONVERT_KB(mem.page_faults)
    );
}


static mrb_value _sigar_proc_time(mrb_state *mrb, mrb_value self)
{
  mrb_int pid;
  sigar_state_t *state = DATA_PTR(self);
  sigar_proc_time_t times;
  
  struct RClass *c = mrb_class_get(mrb, "ProcTimeStruct");
  
  mrb_get_args(mrb, "i", &pid);
  
  CHK( sigar_proc_time_get(state->sigar, pid, &times) );
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 4,
      mrb_fixnum_value(times.start_time / 1000), // ms => s
      mrb_fixnum_value(times.user),
      mrb_fixnum_value(times.sys),
      mrb_fixnum_value(times.total)
    );
}

static mrb_value _sigar_proc_state(mrb_state *mrb, mrb_value self)
{
  mrb_int pid;
  sigar_state_t *state = DATA_PTR(self);
  sigar_proc_state_t proc_state;
  
  struct RClass *c = mrb_class_get(mrb, "ProcStateStruct");
  
  mrb_get_args(mrb, "i", &pid);
  
  CHK( sigar_proc_state_get(state->sigar, pid, &proc_state) );
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 8,
      mrb_str_new_cstr(mrb, proc_state.name),
      mrb_str_new(mrb, &proc_state.state, 1),
      mrb_fixnum_value(proc_state.ppid),
      mrb_fixnum_value(proc_state.tty),
      mrb_fixnum_value(proc_state.priority),
      mrb_fixnum_value(proc_state.nice),
      mrb_fixnum_value(proc_state.processor),
      mrb_fixnum_value(proc_state.threads)
    );
}


static mrb_value _sigar_if_stats(mrb_state *mrb, mrb_value self)
{
  char *ifname;
  sigar_state_t *state = DATA_PTR(self);
  sigar_net_interface_stat_t ifstats;
  
  struct RClass *c = mrb_class_get(mrb, "NetInterfaceState");
  
  mrb_get_args(mrb, "z", &ifname);
  
  CHK( sigar_net_interface_stat_get(state->sigar, ifname, &ifstats) );
  
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 14,
      mrb_fixnum_value(ifstats.rx_packets),
      mrb_fixnum_value(ifstats.rx_bytes),
      mrb_fixnum_value(ifstats.rx_errors),
      mrb_fixnum_value(ifstats.rx_dropped),
      mrb_fixnum_value(ifstats.rx_overruns),
      mrb_fixnum_value(ifstats.rx_frame),
      
      mrb_fixnum_value(ifstats.tx_packets),
      mrb_fixnum_value(ifstats.tx_bytes),
      mrb_fixnum_value(ifstats.tx_errors),
      mrb_fixnum_value(ifstats.tx_dropped),
      mrb_fixnum_value(ifstats.tx_overruns),
      mrb_fixnum_value(ifstats.tx_collisions),
      mrb_fixnum_value(ifstats.tx_carrier),
      
      mrb_fixnum_value(ifstats.speed)
    );
}

static mrb_value _sigar_fs_usage(mrb_state *mrb, mrb_value self)
{
  char *dirname;
  sigar_state_t *state = DATA_PTR(self);
  sigar_file_system_usage_t usage;
  
  struct RClass *c = mrb_class_get(mrb, "FSUsageStruct");
  struct RClass *c2 = mrb_class_get(mrb, "DiskUsageStruct");
  mrb_value r_disk;
  
  mrb_get_args(mrb, "z", &dirname);
  
  CHK( sigar_file_system_usage_get(state->sigar, dirname, &usage) );
  
  
  r_disk = mrb_funcall(mrb, mrb_obj_value(c2), "new", 11,
      mrb_fixnum_value(usage.disk.reads),
      mrb_fixnum_value(usage.disk.writes),
      mrb_fixnum_value(usage.disk.write_bytes / 1024),
      mrb_fixnum_value(usage.disk.read_bytes / 1024),
      mrb_fixnum_value(usage.disk.rtime),
      mrb_fixnum_value(usage.disk.wtime),
      mrb_fixnum_value(usage.disk.qtime),
      mrb_fixnum_value(usage.disk.time),
      mrb_fixnum_value(usage.disk.snaptime),
      mrb_float_value(mrb, usage.disk.service_time),
      mrb_float_value(mrb, usage.disk.queue)
    );
  
  return mrb_funcall(mrb, mrb_obj_value(c), "new", 8,
      r_disk,
      mrb_float_value(mrb, usage.use_percent),
      mrb_fixnum_value(usage.total),
      mrb_fixnum_value(usage.free),
      mrb_fixnum_value(usage.used),
      mrb_fixnum_value(usage.avail),
      mrb_fixnum_value(usage.files),
      mrb_fixnum_value(usage.free_files)
    );
}

static mrb_value _sigar_uptime(mrb_state *mrb, mrb_value self)
{
  sigar_state_t *state = DATA_PTR(self);
  sigar_uptime_t uptime;
  
  CHK( sigar_uptime_get(state->sigar, &uptime) );
  
  return mrb_float_value(mrb, uptime.uptime);
}


void setup_sigar_api(mrb_state *mrb)
{
  struct RClass *c = mrb_define_class(mrb, "Sigar", NULL);
  
  mrb_define_method(mrb, c, "initialize", _sigar_init,  ARGS_REQ(0));
  
  // mostly static informations
  mrb_define_method(mrb, c, "cpus_info_get", _sigar_cpus_info_get,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "filesystems", _sigar_filesystems,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "sysinfo", _sigar_sysinfo,  ARGS_REQ(0));  
  mrb_define_method(mrb, c, "network_interfaces", _sigar_net_interfaces,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "network_infos", _sigar_net_infos,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "network_routes", _sigar_net_routes,  ARGS_REQ(0));

  
  
  // live system measure
  mrb_define_method(mrb, c, "cpu_get", _sigar_cpu_get,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "mem_get", _sigar_mem_get,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "swap_get", _sigar_swap_get,  ARGS_REQ(0));
  mrb_define_method(mrb, c, "loadavg", _sigar_loadavg,  ARGS_REQ(0));
  
  mrb_define_method(mrb, c, "fs_usage", _sigar_fs_usage,  ARGS_REQ(1));
  
  mrb_define_method(mrb, c, "if_stats", _sigar_if_stats,  ARGS_REQ(1));
  
  mrb_define_method(mrb, c, "proc_mem", _sigar_proc_mem,  ARGS_REQ(1));
  mrb_define_method(mrb, c, "proc_time", _sigar_proc_time,  ARGS_REQ(1));
  mrb_define_method(mrb, c, "proc_state", _sigar_proc_state,  ARGS_REQ(1));
  
  
  // others 
  mrb_define_method(mrb, c, "uptime", _sigar_uptime,  ARGS_REQ(0));
}



#undef CHK
