
class Plugin
  
end


class D3Probe
  @@plugin = nil
  
  def self.register_plugin(name, klass, *args)
    @@plugin = klass.new(*args)
  end
  
  def self.start_cycle
    @@plugin.cycle()
  end
  
  def self.pipe
    @pipe
  end
  
end

