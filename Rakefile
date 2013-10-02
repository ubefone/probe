task :default => :build

task :build do
  config_path = File.expand_path('../build_config.rb', __FILE__)
  Dir.chdir( ENV['MRUBY_PATH'] ) do
    sh "MRUBY_CONFIG=#{config_path} rake --trace"
  end
end

task :test => :build do
  puts ""
  exec "sudo ./build/bin/d3probe config.rb"
end
