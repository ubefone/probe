
class Plugin
  def send_metrics(h)
    pipe.write( JSON::stringify(h) )
  end
end


# unit: percents
CPUStruct = Struct.new(
    :user,
    :sys,
    :nice,
    :idle,
    :wait,
    :irq,
    :soft_irq,
    :stolen,
    :combined
  )

# unit: KB
MemStruct = Struct.new(
    :ram,             # Mb
    :total,           # Kb
    :used,
    :free,
    :actual_used,
    :actual_free,
    :used_percent,
    :free_percent
  )



@plugin = nil

def register_plugin(name, obj)
  @plugin = obj
end

