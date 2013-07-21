
class Plugin
  def send_metrics(h)
    pipe.write( JSON::stringify(h) )
  end
end

class CPUStruct < Struct.new(:user, :sys, :nice, :idle, :wait, :irq, :soft_irq, :stolen, :combined)
  
end


class D3Probe
  @@plugin = nil
  
  def self.register_plugin(name, obj)
    @@plugin = obj
  end
  
  def self.start_cycle
    @@plugin.cycle()
  end
    
end



def register_plugin(name, obj)
  D3Probe.register_plugin(name, obj)
end
