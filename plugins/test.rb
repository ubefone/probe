

class Struct
  def to_h
    ret = {}
    members.map{|s| s.to_s }.each do |key|
      val = send(key)
      if val.is_a?(Struct)
        ret[key] = val.to_h()
      else
        ret[key] = val
      end
    end
    ret
  end
end

class AveragedStat
  SAMPLE_SIZE = 5
  
  def initialize(sigar, name, method_name, struct_class)
    @name = name
    @sigar = sigar
    @sigar_method = method_name
    @struct_class = struct_class
    @buffer = {}
    @keys = struct_class.members.map{|s| s.to_s }
    @keys.each do |name|
      @buffer[name] = []
    end
    
    @last_avg = nil
  end
  
  def read
    res = @sigar.send(@sigar_method)
        
    ret = @struct_class.new
    
    @keys.each do |name|
      @buffer[name] << res.send(name)
      if @buffer[name].size > SAMPLE_SIZE
        @buffer[name].shift()
      end
      
      ret.send("#{name}=", mean(@buffer[name]))

    end
    
    @last_avg = ret
    
    ret
  end
  
  def send_stat(plugin)
    data = {}
    @keys.each do |name|
      data[name] = @last_avg.send(name)
    end
    
    plugin.send_metrics(@name => data)
  end

private
  def mean(arr)
    n = 0
    arr.each{|n2| n += n2 }
    
    n / arr.size()
  end
  
end

class AveragedCPUStat < AveragedStat
  def initialize(sigar)
    super(sigar, 'cpu', :cpu_get, CPUStruct)
  end
end


class AveragedMemoryStats < AveragedStat
  def initialize(sigar)
    super(sigar, 'mem', :mem_get, MemStruct)
  end
end


class AveragedSwapStats < AveragedStat
  def initialize(sigar)
    super(sigar, 'swap', :swap_get, SwapStruct)
  end
end


class TestPlugin < Plugin
  
  def initialize
    super
    @sigar = Sigar.new
    
    p [:cpus]
    @sigar.cpus_info_get().map{|s| s.to_h }.each do |h|
      p h
    end
    
    p [:filesystems]
    @sigar.filesystems().map{|s| s.to_h }.each do |h|
      p h
    end
    
    # p [:network_routes]
    # @sigar.network_routes().map{|s| s.to_h }.each do |h|
    #   p h
    # end
    
    p [:sysinfo]
    p @sigar.sysinfo().to_h()
    
    p [:network_interfaces]
    p @sigar.network_interfaces()
    
    p [:mem_get]
    p @sigar.mem_get()
    
    
    # p @sigar.fs_usage('/').to_h()

    
    # p @sigar.loadavg().to_h()
    
    # p @sigar.uptime()
    
    # p @sigar.proc_mem(33).to_h()
    # p @sigar.proc_time(33).to_h()
    # p @sigar.proc_state(33).to_h()
    
    p [:network_infos]
    p @sigar.network_infos().to_h()
  end
  
      
  def cycle
    @cpu = AveragedCPUStat.new(@sigar)
    @mem = AveragedMemoryStats.new(@sigar)
    @swap = AveragedSwapStats.new(@sigar)
    
    pipe._setnonblock(true)
            
    loop do
      sleep(0.1)
      
      @cpu.read()
      @mem.read()
      @swap.read()
      
      begin
        data = pipe.recv(20)
        
        @cpu.send_stat(self)
        @mem.send_stat(self)
        @swap.send_stat(self)
        
        loadavg = @sigar.loadavg()
        send_metrics('system' => {
          'load' => {
            'min1' => loadavg.min1,
            'min5' => loadavg.min5,
            'min15' => loadavg.min15
          }})
        
      rescue => err
        if err.message != "recv"
          p [:err, err, err.message]
        end
        # p [:sleep]
      end
      
    end
  
  end
end

register_plugin('test', TestPlugin.new)
