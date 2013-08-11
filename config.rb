


config(:output) do |o|
  o.host = '127.0.0.1'
  o.port = 4000
  o.hostname = "testhost"
end



config('ping') do |p|
  p.add_icmp_target('1.2.3.4')
  p.add_icmp_target('192.168.0.83')
  
  p.add_arp_target('192.168.0.83')
end
  
