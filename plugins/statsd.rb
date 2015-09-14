
class StatsdPlugin < Plugin
  attr_accessor :host, :app, :res
  attr_accessor :listen_address, :listen_port
  attr_accessor :counters_list_file
  
  include StatsdCPlugin
    
  # bad name, will only be called once and should not exit
  def cycle
    @initialized = true
    @socket = UDPSocket.new
    @socket.bind(@listen_address, @listen_port)
    
    initial_counters = {}
    
    if @counters_list_file && File.exist?(@counters_list_file)
      counter_keys = IO.read(@counters_list_file).split("\n")
      counter_keys.each do |key|
        initial_counters[key] = 0
      end
    end
    
    # block will be called when a request comes from the core
    # if exit is received the method will return
    network_loop(pipe, @socket, initial_counters) do |msg, counters, gauges|
      data = {}
      
      values = gauges
      
      counters.each do |k, v|
        values[k] = v / interval_in_seconds()
      end
      
      unless values.empty?
        if @host
          data['host'] = @host
        end
        
        data[@app] = {}
        data[@app][@res] = values
      end
      
      send_metrics(data)
    end
    
  end
  
private
  def interval_in_seconds
    @interval_in_seconds ||= cycle_interval() / 1000.0
  end
end


register_plugin('statsd', StatsdPlugin.new)
