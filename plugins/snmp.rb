class SnmpPlugin < Plugin
  SnmpHost = Struct.new(:obj, :mibs, :metric_name)
  
  def initialize
    @snmps = {}
  end
  
  def query(host, opts = {})
    raise "mibs required" unless opts.has_key?(:mibs)
    
    snmp = SNMP.new(host)
    snmp.timeout = 1
    
    @snmps[host] = SnmpHost.new(
        snmp,
        opts.delete(:mibs),
        opts.delete(:metric_name) || host
      )
    
    rt = opts.delete(:routing_table)
    if rt.is_a?(Integer) && Socket::Constants.const_defined?(:SO_RTABLE)
      snmp.sock.setsockopt(Socket::Constants::SOL_SOCKET, Socket::Constants::SO_RTABLE, [rt].pack('L'))
      snmp.sock.setsockopt(Socket::Constants::IPPROTO_IP, Socket::Constants::SO_RTABLE, [rt].pack('L'))
    end
  end
    
  def cycle
    simple_loop do
      ret = {}
      
      unless @snmps.empty?
        @snmps.each do |host, snmp|
          ret[snmp.metric_name] = {}
          snmp.obj.get(snmp.mibs.keys) do |oid, tag, val|
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
