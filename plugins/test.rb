

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
  
  def send_stat(plugin, stats)
    stats[:res_name] = @name
    stats[:data].clear()
    @keys.each do |name|
      stats[:data][name] = @last_avg.send(name)
    end
    
    plugin.send_metrics(stats)
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


class TestPlugin < Plugin
  
  def initialize
    super
    @sigar = Sigar.new
  end
  
      
  def cycle
    @cpu = AveragedCPUStat.new(@sigar)
    @mem = AveragedMemoryStats.new(@sigar)
    
    pipe._setnonblock(true)
    stats = {
      type:       'datapoint',
      app_name:   'system',
      data: {}
    }
        
    loop do
      sleep(0.1)
      
      @cpu.read()
      @mem.read()
      
      begin
        data = pipe.recv(20)
        
        @cpu.send_stat(self, stats)
        @mem.send_stat(self, stats)
        
        
        p [:test, :done]
      rescue => err
        if err.message != "recv"
          p [:err, err]
        end
        # p [:sleep]
      end
      
    end
  
  end
end

register_plugin('test', TestPlugin.new)
