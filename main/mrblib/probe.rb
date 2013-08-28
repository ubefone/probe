
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

@states = [{}, {}]
@last_state = 0

if Object.const_defined?(:ObjectSpace)
  def print_memory_state
    next_state = (@last_state + 1) % 2
    ObjectSpace.count_objects(@states[next_state])
    unless @states[@last_state].empty?
      tmp = @states[next_state].each do |k, v|
        change = v - @states[@last_state][k]
        if change != 0
          p [k, change]
        end
      end
    end
    
    @last_state = next_state
  end
end

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
