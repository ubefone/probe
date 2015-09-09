class StatsdParser
  attr_reader :values
  
  def initialize
    @values = {}
  end
  
  def parse(data)
    # <metric name>:<value>|g
    # <metric name>:<value>|c[|@<sample rate>]
    # <metric name>:<value>|ms
    data.split("\n").each do |line|
      metric_name, rest = line.split(':')
      value, _ = rest.split('|')
      
      @values[metric_name] = value.to_i
    end
  end
  
  def read_and_reset()
    tmp = @values
    @values = {}
    tmp
  end
end

class StatsdPlugin < Plugin
  attr_accessor :host, :app, :res
  attr_accessor :listen_address, :listen_port
  
  RECV_SIZE = 2000
  
  def initialize
    @parser = StatsdParser.new
  end
  
  # bad name, will only be called once and should not exit
  def cycle
    @initialized = true
    @socket = UDPSocket.new
    @socket.bind(@listen_address, @listen_port)
    
    # ensure we won't be blocked by wait_command
    pipe._setnonblock(true)
    
    loop do
      # wait for data on the socket but no more than for 100ms
      if IO.select([@socket], nil, nil, 0.1)
        msg, _ = @socket.recvfrom(RECV_SIZE, 0)
        @parser.parse(msg)
      end
      
      
      begin
        cmd = wait_command()
        break if cmd == "exit"
        
        data = {}
        
        content = @parser.read_and_reset()
        unless content.empty?
          if @host
            data['host'] = @host
          end
          
          data[@app] = {}
          data[@app][@res] = content
        end
        
        send_metrics(data)
        GC.start()
      rescue => err
        
      end
    end
    
  end
  
end


register_plugin('statsd', StatsdPlugin.new)
