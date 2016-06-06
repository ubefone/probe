class TcpPlugin < Plugin
  attr_accessor :res, :app
  attr_accessor :max_retries, :retry_delay
  
  def initialize
    @targets = {}
    @requests = []
  end
  
  def after_config
    @retry_delay ||= 0.2
    @max_retries ||= 10
    
    unless @res
      raise "res is mandatory"
    end
  end
  
  def add_request(name, opts = {})
    send_data = opts[:send] || raise("missing send")
    expects   = opts[:expects] || raise("missing expects")
    
    @requests << [send_data, expects, name]
  end
  
  def set_targets(h)
    @targets = {}
    
    h.each do |key, opts|
      addr, port = *opts
      @targets[key] = Socket.sockaddr_in(port, addr)
    end
  end
  
  def ping(so_addr, request)
    s = Socket.new(Socket::Constants::AF_INET, Socket::Constants::SOCK_STREAM, 0)
    
    retries = @max_retries
    begin
      s.connect_nonblock(so_addr)
    rescue Errno::EINPROGRESS, Errno::EALREADY
      IO.select(nil, [s], nil, @retry_delay)
      if (retries -= 1) <= 0
        return false
      else
        retry
      end
    
    rescue Errno::ECONNREFUSED
      return false
    
    rescue Errno::EISCONN
      # we are connected
    end
    
    s.write( request[0] )
    
    # wait for the answer
    retries = @max_retries
    begin
      str = s.recv_nonblock(100)
      return (str == request[1])
    rescue Errno::EWOULDBLOCK
      IO.select([s], nil, nil, @retry_delay)
      if (retries -= 1) <= 0
        return false
      else
        retry
      end
    end
    
  end
  
  def cycle()
    simple_loop do
      ret= { @app => {} }
      
      @targets.each do |name, so_addr|
        ret[@app][name] = {}
        @requests.each do |request|
          
          started_at = Kernel.time()
          if ping(so_addr, request)
            ret[@app][name][ request[2] ] = (Kernel.time() - started_at)
          end
          
        end
      end
      
      send_metrics(ret)
    end
  end
end

register_plugin('tcp', TcpPlugin.new)
