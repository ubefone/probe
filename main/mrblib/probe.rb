
class Plugin
  
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

