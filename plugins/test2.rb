class SnmpPlugin < Plugin
  def initialize
    @snmp1 = SNMP.new()
    @snmp2 = SNMP.new()
    # @snmp.community = 'public'
    # @snmp.version = '1'
    # @snmp.connect()
  end
    
  def cycle
    loop do
      p [:LOOP]
      # puts "I collect cpu"
      # sleep(3)
      pipe.recv(200)
      
      pp = ->(ret){
        p [:PP, ret]
      }

      
      @snmp1.get(['SNMPv2-MIB::sysName.0', 'SNMPv2-MIB::sysServices.0'], &pp)
      # @snmp1.get(['SNMPv2-MIB::sysName.0', 'SNMPv2-MIB::sysServices.0']) do |ret|
      #   p [:CB, ret]
      # end
      
      @snmp2.get(['SNMPv2-MIB::sysServices.0'], &pp)
      # @snmp2.get(['SNMPv2-MIB::sysServices.0']) do |ret|
      #   p [:CB2, ret]
      # end
      
      SNMP.select([@snmp1, @snmp2])
      
      # D3Probe.report(:user => 7, :sstem => 4)
      send_metrics(
          plugin: "cpu",
          value: 1.58
        )
    end
      
  end
end


register_plugin('snmp', SnmpPlugin.new)
