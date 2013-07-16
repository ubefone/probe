
class Plugin
  
end


class D3Probe
  @@plugins = {}
  
  def self.register_plugin(name, klass, *args)
    @@plugins[name] = klass.new(*args)
  end
  
  def self.call_plugins
    @@plugins.each do |name, p|
      p.action
    end
  end
  
  
end

