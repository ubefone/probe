
class PacketFilter
  def stats
    ret = {}
    
    _stats().each do |st|
      existing_st = ret[st.label]
      
      if existing_st
        existing_st.members.each do |field|
          existing_st.send("#{field}=", existing_st.send(field) + st.send(field))
        end
      else
        ret[st.label] = st
      end
    end
    
    ret
  end
  
  # def _stats
  #   [
  #     PacketFilterLabelStats.new("data", *(%w(10 0 0 10 0).map{|s| s.to_i})),
  #     PacketFilterLabelStats.new("voip", *(%w(10 0 0 20 0).map{|s| s.to_i})),
  #     PacketFilterLabelStats.new("data", *(%w(10 0 0 2 0).map{|s| s.to_i}))
  #   ]
  # end
end



class PacketFilterPlugin < Plugin
  def initialize
    @pf = PacketFilter.new
  end
  
  def cycle
    loop do
      ret = {}
      pipe.recv(200)
      
      p @pf.stats
      
      send_metrics('pf_labels' => ret)
    end
  end
end

register_plugin('pf', PacketFilterPlugin.new)
