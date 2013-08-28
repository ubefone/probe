
class Plugin
  def send_metrics(h)
    pipe.write( JSON::stringify(h) )
  end
  
  def wait_command
    pipe.recv(10)
  end
  
  # if the plugin sleep while waiting next cycle just
  # call this method with a block which will be called for each
  # cycle
  def simple_loop
    loop do
      cmd = wait_command()
      if cmd == "exit"
        break
      else
        yield()
      end
    end
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
