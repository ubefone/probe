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
        
        data['arp_ping'] = {}
        
        @arp_targets.each do |host|
          data['arp_ping'][host] = {'reply' => ret.has_key?(host) ? 1 : 0 }
        end
        
      end
      
      unless @icmp_targets.empty?
        percentiles = [0.05, 0.25, 0.5, 0.75, 0.95, 0.98]
        ret = @icmp.send_pings(@icmp_timeout, @icmp_count, @icmp_delay, percentiles)
        # p [:ret, ret]
        
        data['icmp_ping'] = {}
        
        @icmp_targets.each do |host|
          data['icmp_ping'][host] = {
            'latency' => ret[host][0] / 1000,
            'loss' => ret[host][1]
          }
          
          
          percentiles.each do |p|
            v = ret[host][2][p]
            if v
              data['icmp_ping'][host]["p#{(p * 100).to_i}"] = v / 1000
            end
          end
        end
        
      end

      send_metrics(data)
    end
  end
end


register_plugin('ping', PingPlugin.new)
