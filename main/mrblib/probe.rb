
class Plugin
  def send_metrics(h)
    pipe.write( JSON::stringify(h) )
  end
end

@plugin = nil
@plugin_name = nil

def register_plugin(name, obj)
  @plugin = obj
  @plugin_name = name
end

def config(name, &block)
  if name == :output
    block.call($output) if $output
  elsif name == @plugin_name
    puts "Plugin found, configuring..."
    block.call(@plugin)
  end
end

def plugin(*args, &block)
  config(*args, &block)
end
