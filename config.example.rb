
# general config
config(:output) do |o|
  # where to send the collected data
  o.host = '127.0.0.1'
  o.port = 4000
  
  # advertised hostname included
  # in transmitted data
  o.hostname = "local"
  
  # how frequently data will be sent (in ms)
  o.interval = 5000
end


# config for ping module
plugin('ping') do |p|
  # icmp ping
  p.icmp_count = 10
  p.icmp_delay = 80
  p.icmp_timeout = 2000
  
  p.add_icmp_target('192.168.0.1')
  p.add_icmp_target('192.168.0.12')
  
  # ARP ping
  p.add_arp_target('10.11.20.253')
end
  
plugin('snmp') do |p|
  # p.load_mibs('path/to/mib')
  
  p.query('127.0.0.1', {
    'SNMPv2-MIB::sysName.0' => 'sysname',
    'SNMPv2-MIB::sysServices.0' => 'sysservice'
  })
end

plugin('sigar') do |p|
  # values are collectd in continue and averaged to be
  # then sent on each interval.
  # lower values yield higher precision but also higher
  # cpu usage.
  p.loop_delay = 0.3
  
  # which network interfaces to gather stats for
  p.monitor_interfaces('eth0', 'tun1')
end
