

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
    toto = $output
    h = JSON::parse(json)
    @buffer << h
  rescue ArgumentError => err
    raise "Invalid json received: #{json}"
  end
  
  ##
  # send data
  def flush
    # puts "\n\nFLUSH\n"
    
    cycle_msg = {
      'type' => 'datapoints',
      'host' => @hostname
    }
    
    if @just_started
      cycle_msg['first'] = true
      @just_started = false
    end
    
    while  msg = @buffer.shift
      # send_msg(msg)
      msg.each do |k, v|
        if cycle_msg[k]
          cycle_msg[k] = cycle_msg[k].merge(v)
        else
          cycle_msg[k] = v
        end
      end
    end
    
    send_msg(cycle_msg)
    
  end
  
  def send_msg(msg)
    # p msg
    json = JSON::stringify(msg)
    # p [:send_msg, json.bytesize]
    @socket.send(json, 0, @host, @port)
  end
  
  def send_report
    report = HardwareReport.new
    ret = report.build_report()
    p [:report, ret]
    send_msg(ret)
  end
  
end

# $output = nil

def register_output(obj)
  $output = obj
end

register_output(Output.new)
