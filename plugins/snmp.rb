class SnmpPlugin < Plugin
  SnmpHost = Struct.new(:obj, :mibs, :metric_name, :cached_get_query)
  
  def initialize
    @snmps = {}
  end
  
  def query(host, opts = {})
    raise "mibs required" unless opts.has_key?(:mibs)
    
    snmp = SNMP.new(host, opts.merge({:timeout => 1, :version => "2c"}))
    
    key = opts.delete(:metric_name) || host
    
    @snmps[key] = SnmpHost.new(
        snmp,
        opts.delete(:mibs),
        key,
        nil
      )
    
    rt = opts.delete(:routing_table)
    if rt.is_a?(Integer) && Socket::Constants.const_defined?(:SO_RTABLE)
      snmp.sock.setsockopt(Socket::Constants::SOL_SOCKET, Socket::Constants::SO_RTABLE, [rt].pack('L'))
      snmp.sock.setsockopt(Socket::Constants::IPPROTO_IP, Socket::Constants::SO_RTABLE, [rt].pack('L'))
    end
  end
    
  def cycle
    # build get requests and cache them
    @snmps.each do |_, snmp|
      query = snmp.obj.build_get_query(snmp.mibs.keys)
      snmp.cached_get_query = query
    end
    
    simple_loop do
      ret = {}
      
      unless @snmps.empty?
        @snmps.each do |_, snmp|
          ret[snmp.metric_name] = {}
          snmp.obj.get(snmp.cached_get_query) do |oid, tag, val|
            ret[snmp.metric_name][snmp.mibs[oid] || oid] = val
          end
        end
        
      end
      
      send_metrics('snmp' => ret)
    end
      
  end
  
private
  def parse_value(str)
    type, value = str.split(': ')
    case type
      when 'INTEGER' then value.to_i
      when 'STRING'  then value
      else
        raise "unsupported snmp return type: #{type}"
    end
  end
end


register_plugin('snmp', SnmpPlugin.new)
