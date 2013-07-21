
class Output
  def initialize
    @buffer = []
    @socket = UDPSocket.new
    
    @host = "127.0.0.1"
    @port = 4000
    @hostname = 'local1'
  end
  
  def add(json)
    toto = $output
    h = JSON::parse(json)
    @buffer << h
  end
  
  ##
  # send data
  def flush
    @buffer.each do |msg|
      msg[:type] = 'datapoint'
      msg[:host] = @hostname
      
      @socket.send(JSON::stringify(msg), 0, @host, @port)
    end
    
    @buffer.clear
  end
  
  
  
end

# $output = nil

def register_output(obj)
  $output = obj
end

register_output(Output.new)
