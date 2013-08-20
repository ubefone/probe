class SnmpPlugin < Plugin
  SnmpHost = Struct.new(:obj, :mibs)
  
  def initialize
    @snmps = {}
  end
  
  def load_mibs(path)
    SNMP.load_mibs(path)
  end
  
  def query(host, mibs = {})
    @snmps[host] = SnmpHost.new(
        SNMP.new('127.0.0.1'),
        mibs
      )
  end
    
  def cycle
    loop do
      ret = {}
      
      pipe.recv(200)
      
      @snmps.each do |host, snmp|
        ret[host] = {}
        snmp.obj.get(*snmp.mibs.keys) do |rr|
          h = {}
          rr.each do |k, v|
            h[snmp.mibs[k]] = parse_value(v)
          end
          ret[host] = h
        end
      end
      
      # wait for the responses
      SNMP.select()
      
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
