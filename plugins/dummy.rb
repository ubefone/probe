
class DummyPlugin < Plugin
  def cycle
    simple_loop do
      puts "sleeping..."
      send_metrics({toto: 42})
    end
  end
end

register_plugin('dummy', DummyPlugin.new)

