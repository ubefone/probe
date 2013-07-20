class CPUPlugin < Plugin
  def cycle
    # puts "I collect cpu"
    sleep(3)
    # D3Probe.report(:user => 7, :sstem => 4)
    pipe.write("TOTO")
    pipe.write("UTUT")
  end
end


D3Probe.register_plugin('cpu', CPUPlugin.new)
