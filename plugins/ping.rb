class PingPlugin < Plugin
  def initialize
    @arp = ARPPinger.new('en1')
    @icmp = ICMPPinger.new
  end
    
  def cycle
    @targets = [
        # '10.11.20.2',
        '192.168.0.83'#,
        # '212.27.48.10'
      ]
      
    @arp.set_targets(@targets)
    @icmp.set_targets(@targets)
    
    loop do
      pipe.recv(200)
      
      data = {}
      
      ret = @arp.send_pings(500)
      
      @targets.each do |host|
        if ret.has_key?(host)
          data[host] = ret[host]
        else
          data[host] = false
        end
      end
      
      send_metrics('arp-ping' => data)
      
      
      ret = @icmp.send_pings(500, 10)
      p ret

    end
  end
end


register_plugin('ping', PingPlugin.new)
