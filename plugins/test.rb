
class TestPlugin < Plugin
  def cycle
    pipe._setnonblock(true)
    
    loop do
      # Sleep::usleep(2)
      # D3Probe.report(:free => 42, :used => 5)
      
      begin
        p [:test, :started]
        data = pipe.recv(200)
        p [:data, data]
        pipe.write("JAZJAZKAZKJAZKAZ\n")
        p [:test, :done]
      rescue
        p [:sleep]
        sleep(0.2)
      end
      
    end
  
  end
end

D3Probe.register_plugin('test', TestPlugin.new)
