
class TestPlugin < Plugin
  
  SAMPLE_SIZE = 5
  CPU_KEYS = CPUStruct.members.map{|sym| sym.to_s }
  
  def initialize
    super
    @stats = nil
    @sigar = Sigar.new
    @count = 0
    
    
    @stats = {}
    CPU_KEYS.each do |name|
      @stats[name] = []
    end
    
  end
  
  def mean(arr)
    n = 0
    arr.each{|n2| n += n2 }
    
    n / arr.size()
  end
    
  def read_cpu
    cpu = @sigar.cpu_get()
    
    if @count < SAMPLE_SIZE + 1
      @count += 1
    end
    
    ret = CPUStruct.new
    
    CPU_KEYS.each do |name|
      @stats[name] << cpu.send(name)
      if @count == SAMPLE_SIZE + 1
        @stats[name].shift()
      end
      
      ret.send("#{name}=", mean(@stats[name]))

    end
    
    ret
  end
  
  def cycle
    pipe._setnonblock(true)
    stats = {
      type:       'datapoint',
      app_name:   'system',
      res_name:   'cpu',
      data: {}
    }
        
    loop do
      sleep(0.1)
            
      # p @sigar.cpu_get()
      
      cpu = read_cpu()
      # p [:user, ret[:user]]

      
      # p p.query('cpu.0')
      
      begin
        # p [:test, :started]
        data = pipe.recv(200)
        p [:data, data]
        
        stats[:data].clear()
        CPU_KEYS.each do |name|
          stats[:data][name] = cpu.send(name)
        end
        
        send_metrics(stats)
        
        
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
