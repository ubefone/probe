class SnmpPlugin < Plugin
  SnmpHost = Struct.new(:obj, :mibs, :routing_table)
  
  def initialize
    @snmps = {}
  end
  
  def load_mibs(path)
    SNMP.load_mibs(path)
  end
  
  def query(host, opts = {})
    raise "mibs required" unless opts.has_key?(:mibs)
    
    snmp = SNMP.new(host)
    
    @snmps[host] = SnmpHost.new(
        snmp,
        opts.delete(:mibs),
        opts.delete(:routing_table)
      )
    
    rt = opts.delete(:routing_table)
    if rt.is_a?(Integer) && Socket::Constants.const_defined?(:SO_RTABLE)
      snmp.sock.setsockopt(Socket::Constants::SOL_SOCKET, Socket::Constants::SO_RTABLE, rt)
    end
  end
    
  def cycle
    simple_loop do
      ret = {}
      
      unless @snmps.empty?
        @snmps.each do |host, snmp|
          ret[host] = {}
          snmp.obj.get(snmp.mibs.keys) do |oid, tag, val|
            ret[host][snmp.mibs[oid] || oid] = val
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
