class CPUPlugin < Plugin
  def cycle
    loop do
      # puts "I collect cpu"
      # sleep(3)
      pipe.recv(200)
      
      # D3Probe.report(:user => 7, :sstem => 4)
      send_metrics(
          plugin: "cpu",
          value: 1.58
        )
    end
  end
end


D3Probe.register_plugin('cpu', CPUPlugin.new)
