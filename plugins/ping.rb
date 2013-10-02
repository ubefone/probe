class PingPlugin < Plugin
  attr_accessor :icmp_timeout, :icmp_delay, :icmp_count
  
  def initialize
    # TODO: param should allow nil
    # @arp = ARPPinger.new('en1')
    @icmp = ICMPPinger.new
    
    @arp_targets = []
    
    @icmp_timeout = 500
    @icmp_count = 10
    @icmp_delay = 10
    @icmp_host_mapping = {}
  end
  
  def add_icmp_target(addr, opts = {})
    if metric_name = opts[:metric_name]
      @icmp_host_mapping[addr] = metric_name
    end
    
    @icmp.add_target(addr, opts)
  end
  
  def add_arp_target(*addr)
    addr.each{|a| @arp_targets << a }
  end

    
  def cycle
    # @arp.set_targets(@arp_targets)
    
    simple_loop do
      data = {}
      
      # unless @arp_targets.empty?
      #   ret = @arp.send_pings(500)
        
      #   data['arp_ping'] = {}
        
      #   @arp_targets.each do |host|
      #     data['arp_ping'][host] = {'reply' => ret.has_key?(host) ? 1 : 0 }
      #   end
        
      # end
      
      if @icmp.has_targets?
        percentiles = [0.05, 0.25, 0.5, 0.75, 0.95, 0.98]
        ret = @icmp.send_pings(@icmp_timeout, @icmp_count, @icmp_delay, percentiles)
        # p [:ret, ret]
        
        data['icmp_ping'] = {}
        
        ret.each do |host, dd|
          key = @icmp_host_mapping[host] || host
          
          data['icmp_ping'][key] = {
            'latency' => ret[host][0] / 1000,
            'loss' => ret[host][1]
          }
          
          percentiles.each do |p|
            v = ret[host][2][p]
            if v
              data['icmp_ping'][key]["p#{(p * 100).to_i}"] = v / 1000
            end
          end
        end
        
      end

      send_metrics(data)
    end
  end
end


register_plugin('ping', PingPlugin.new)
