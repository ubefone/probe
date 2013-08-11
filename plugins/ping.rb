class PingPlugin < Plugin
  def initialize
    @arp = ARPPinger.new('en1')
    @icmp = ICMPPinger.new
    
    @icmp_targets = []
    @arp_targets = []
  end
  
  def add_icmp_target(*addr)
    addr.each{|a| @icmp_targets << a }
  end
  
  def add_arp_target(*addr)
    addr.each{|a| @arp_targets << a }
  end

    
  def cycle
    @arp.set_targets(@arp_targets)
    @icmp.set_targets(@icmp_targets)
    
    loop do
      pipe.recv(200)
      
      data = {}
      
      ret = @arp.send_pings(500)
      
      @arp_targets.each do |host|
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
