
config(:output) do |o|
  o.host = '127.0.0.1'
  o.port = 4000
  o.hostname = "local"
  
  o.interval = 5
end

config('ping') do |p|
  p.icmp_count = 10
  p.icmp_delay = 100
  p.icmp_timeout = 2000
  
  p.add_icmp_target('10.11.20.253')
  p.add_icmp_target('192.168.0.83')
  
  p.add_arp_target('10.11.20.253')
end
  
