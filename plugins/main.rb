

class HardwareReport
  def initialize
    @sigar = Sigar.new
  end
  
  
  def build_report
    report = {
      type: 'specs',
      host: 'local1',
      filesystems: []
    }
    
    mem = @sigar.mem_get()
    report[:memory] = {
      :total => mem.total
    }
    
    
    cpus = @sigar.cpus_info_get()
    cpu_info = cpus[0]
    
    report[:cpu] = {
      cores:      cpu_info.total_cores,
      frequency:  cpu_info.mhz,
      cache_size: cpu_info.cache_size,
      vendor:     cpu_info.vendor,
      model:      cpu_info.model
    }
    
    net_infos = @sigar.network_infos()
    report[:network] = {
      :hostname           => net_infos.hostname,
      :nameservers        => [net_infos.primary_dns, net_infos.secondary_dns],
      :gateway            => net_infos.default_gateway,
      :gateway_interface  => net_infos.default_gateway_interface,
      :interfaces         => []
    }
    
    # interfaces
    @sigar.network_interfaces().each do |device_name|
      report[:network][:interfaces] << {
        device: device_name
      }
    end
    
    # filesystems
    @sigar.filesystems().each do |fs|
      report[:filesystems] << {
        sys_type:   fs.sys_type,
        dev_name:   fs.dev_name,
        mountpoint: fs.dir_name,
        options:    fs.options,
        type:       fs.type,
        
        # flags: [...]
      }
    end
    
    
    
    # ident
    # SNMPv2-MIB::sysDescr.0 = STRING: Darwin pomme.home 12.4.0 Darwin Kernel Version 12.4.0: Wed May  1 17:57:12 PDT 2013; root:xnu-2050.24.15~1/RELEASE_X86_64 x86_64
    
    # hostname
    
    # network interfaces: (sigar_net_info_get, sigar_net_route_list_get, sigar_net_interface_config_get)
    # - name (vr3, eth0)
    # - type
    # - chipset
    # - hwaddress
    # - addresses (ipv4 or ipv6) (?)
    
    # hard drives: (sigar_file_system_list_get)
    # - type
    # - mount point (/usr)
    # - size
    
    # cpus: (sigar_cpu_info_list_get)
    # - count
    # - type
    # - frequency
    
    # memory:
    # - total size
    # - slots free / full (?)
    
    
    report
  end
  
  
  def send
    @output.add()
  end
end



class Output
  attr_accessor :host, :port, :hostname, :interval
  
  def initialize
    @buffer = []
    @socket = UDPSocket.new
    
    @host = "127.0.0.1"
    @port = 4000
    @hostname = 'local1'
    @just_started = true
  end
  
  def add(json)
    data = Zlib.inflate(json)
    h = JSON::parse( data )
    @buffer << h
  rescue ArgumentError => err
    raise "Invalid json received: #{data}\n"
  end
  
  def build_msg(host = nil)
    ret = {
      'type' => 'datapoints',
      'host' => host || @hostname
    }
    
    if @just_started
      @just_started = false
      ret['first'] = true
    end
    
    ret
  end
  
  ##
  # send data
  def flush
    built_msgs = {}
    
    while  msg = @buffer.shift
      # use global hostname as default
      host = msg.delete('host') || @hostname
      # and initialize packet if it does not already exists
      built_msgs[host] ||= build_msg(host)
      target = built_msgs[host]
      
      msg.each do |k, v|
        if target[k]
          target[k] = target[k].merge(v)
        else
          target[k] = v
        end
      end
    end
    
    # if the probe is launched by daemontools, check our memory usage
    # if it exceeds 100MB print an error and exit
    if built_msgs[@hostname] && built_msgs[@hostname]["processes"] && (probe_res = built_msgs[@hostname]["processes"]["probe"])
      mem = probe_res['memory'] / 1024
      puts "memory used: #{mem} MB"
      if mem >= 100
        puts "using too much memory, exiting now !"
        exit
      end
    end
    
    # send each message
    built_msgs.each do |host, msg|
      send_msg(msg)
    end
    
  end
  
  def send_msg(msg)
    # p msg
    json = JSON::dump(msg)
    @socket.send(Zlib.deflate(json), 0, @host, @port)
  rescue
    # if @socket.send fail, just ignores it
  end
  
  def send_report
    # report = HardwareReport.new
    # ret = report.build_report()
    # p [:report, ret]
    # send_msg(ret)
  end
  
end

# $output = nil

def register_output(obj)
  $output = obj
end

register_output(Output.new)
