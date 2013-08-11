class PingPlugin < Plugin
  attr_accessor :icmp_timeout, :icmp_delay, :icmp_count
  
  def initialize
    @arp = ARPPinger.new('en1')
    @icmp = ICMPPinger.new
    
    @icmp_targets = []
    @arp_targets = []
    
    @icmp_timeout = 500
    @icmp_count = 10
    @icmp_delay = 10
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
      
      unless @arp_targets.empty?
        ret = @arp.send_pings(500)
        
        data['arp-ping'] = {}
        
        @arp_targets.each do |host|
          data['arp-ping'][host] = {'reply' => ret.has_key?(host) ? 1 : 0 }
        end
        
      end
      
      unless @icmp_targets.empty?
        ret = @icmp.send_pings(@icmp_timeout, @icmp_count, @icmp_delay)
        p ret
        
        data['icmp-ping'] = {}
        
        @icmp_targets.each do |host|
          data['icmp-ping'][host] = {
            'latency' => ret[host][0],
            'loss' => ret[host][1]
          }
        end
        
      end

      send_metrics(data)
    end
  end
end


register_plugin('ping', PingPlugin.new)
