
class Output
  def initialize
    @buffer = []
    @socket = UDPSocket.new
    
    @host = "127.0.0.1"
    @port = 4000
  end
  
  def add(json)
    h = JSON::parse(json)
    p [:add, h]
    @buffer << h
  end
  
  ##
  # send data
  def flush
    @socket.send("hello", 0, @host, @port)
    @buffer.clear()
  rescue => err
    p [:err, err]
  end
  
  
  
end
