class StatsdParser
  attr_reader :values
  
  def initialize
    @counters = {}
    @gauges = {}
    @counter_keys = []
    data = IO.read('/tmp/test')
  end
  
  def load_counter_list(path)
    # if a file was provided initialize counters from it
    if File.exist?(path)
      @counter_keys = IO.read(path).split("\n")
    else
      puts "counter file does not exists: #{path}, ignored."
    end
  end
  
  def parse(data)
    # <metric name>:<value>|g
    # <metric name>:<value>|c[|@<sample rate>]
    # <metric name>:<value>|ms
    data.split("\n").each do |line|
      metric_name, rest = line.split(':')
      value, type = rest.split('|')
      
      case type
      when 'c'
        unless @counter_keys.include?(metric_name)
          @counter_keys << metric_name
        end
        
        @counters[metric_name] ||= 0
        @counters[metric_name] += value.to_i
        
      when 'g' then @gauges[metric_name] = value.to_i
      else
        puts "[statsd] unsupported type: #{type}"
      end
    end
  end
  
  def read_and_reset()
    tmp = @gauges
    
    @counters.each do |k, v|
      tmp[k] = v / interval_in_seconds()
    end
    
    @counters = {}
    @gauges = {}
    
    @counter_keys.each do |name|
      @counters[name] = 0
    end
    
    tmp
  end
  
  def interval_in_seconds
    @interval_in_seconds ||= cycle_interval() / 1000.0
  end
end

class StatsdPlugin < Plugin
  attr_accessor :host, :app, :res
  attr_accessor :listen_address, :listen_port
  attr_accessor :counters_list_file
  
  RECV_SIZE = 2000
  
  def initialize
    @parser = StatsdParser.new
  end
  
  # bad name, will only be called once and should not exit
  def cycle
    @initialized = true
    @socket = UDPSocket.new
    @socket.bind(@listen_address, @listen_port)
    
    if @counters_list_file
      @parser.load_counter_list(@counters_list_file)
    end
    
    # ensure we won't be blocked by wait_command
    pipe._setnonblock(true)
    
    loop do
      # wait for data on the socket but no more than for 100ms
      if ret = IO.select([@socket, pipe])
        readables = ret[0]
        
        if readables.include?(@socket)
          msg, _ = @socket.recvfrom(RECV_SIZE, 0)
          @parser.parse(msg)
          
        elsif readables.include?(pipe)
          cmd = wait_command()
          break if cmd == "exit"
          p [:hop]
          data = {}
          
          content = @parser.read_and_reset()
          p [:cc, content]
          unless content.empty?
            if @host
              data['host'] = @host
            end
            
            data[@app] = {}
            data[@app][@res] = content
          end
          
          send_metrics(data)

        end
      end
      
    end
    
  end
  
end


register_plugin('statsd', StatsdPlugin.new)
