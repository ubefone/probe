MRuby::Build.new do |conf|
  # load specific toolchain settings
  toolchain :gcc

  # Use mrbgems
  # conf.gem 'examples/mrbgems/ruby_extension_example'
  # conf.gem 'examples/mrbgems/c_extension_example' do |g|
  #   g.cc.flags << '-g' # append cflags in this gem
  # end
  # conf.gem 'examples/mrbgems/c_and_ruby_extension_example'
  # conf.gem :github => 'masuidrive/mrbgems-example', :branch => 'master'
  # conf.gem :git => 'git@github.com:masuidrive/mrbgems-example.git', :branch => 'master', :options => '-v'

  # include the default GEMs
  # conf.gembox 'default'
  
  def add_lib_folders(conf, *paths)
    paths.each do |path|
      conf.linker.library_paths << path if Dir.exists?(path)
    end
  end
  
  def add_include_folders(conf, *paths)
    paths.each do |path|
      conf.cc.include_paths << path if Dir.exists?(path)
    end
  end
  
  
  
  # options
  with_dmalloc = false
  with_gmalloc = false

  with_memory_profiler_c = false
  with_memory_profiler_ruby = false
  
  enable_64bits_mode = false
  gc_stress = false
  debug_mode = true



  ## ------------

  conf.gem core: "mruby-print"
  conf.gem core: "mruby-struct"
  conf.gem core: "mruby-numeric-ext"
  # conf.gem core: "mruby-enum-ext"
  # conf.gem core: "mruby-string-ext"

  # conf.gem github: 'iij/mruby-io'
  # conf.gem github: 'iij/mruby-socket'
  # conf.gem github: 'mattn/mruby-json'
  # conf.gem github: 'viking/mruby-zlib'
  # conf.gem '/Users/schmurfy/Dev/personal/mrbgems/mruby-ping'
  # conf.gem github: 'schmurfy/mruby-ping'
  
  conf.gem "#{root}/../mrbgems/mruby-io"
  conf.gem "#{root}/../mrbgems/mruby-socket"
  conf.gem "#{root}/../mrbgems/mruby-json"
  conf.gem "#{root}/../mrbgems/mruby-ping"
  
  
  add_include_folders(conf,
      "#{root}/include",
      "../../deps/libnet/src/include",
      "../../deps/sigar/src/include",
      "../../deps/net-snmp/src/include",
      "/tmp/sigar/include",
      "/usr/local/include",
      "/usr/local/include/libnet-1.1"
    )
  
  add_lib_folders(conf,
      "../../deps/libnet/src/src/.libs",
      "../../deps/sigar/src/src/.libs",
      "../../deps/net-snmp/src/snmplib/.libs",
      "/tmp/sigar/src/.libs",
      "/usr/local/lib"
      # "/usr/local/lib/libnet11",
    )
  
  
  if debug_mode
    conf.cc.flags = %w(-g -Wall -Werror-implicit-function-declaration)
  end
  
  if gc_stress
    conf.cc.defines << "MRB_GC_STRESS"
  end
  
  if enable_64bits_mode
    conf.cc.defines << "MRB_INT64"
  end

  if with_memory_profiler_c
    conf.cc.defines << '_MEM_PROFILER'
  end

  if with_memory_profiler_ruby
    conf.cc.defines << '_MEM_PROFILER_RUBY'
    conf.gem core: "mruby-objectspace"
    # keep filenames and line numbers
    conf.mrbc.compile_options = "-g -B%{funcname} -o-" # The -g option is required for line numbers
  end
  

  if with_dmalloc
    conf.cc.defines << 'DMALLOC'
    conf.cc.defines << 'DMALLOC_FUNC_CHECK'
    conf.linker.libraries << 'dmallocth.a'
  end

  if with_gmalloc
    conf.linker.libraries << 'gmalloc'
  end
    
  
  
  conf.gem 'main'
  
  
  
  # C compiler settings
  # conf.cc do |cc|
  #   cc.command = ENV['CC'] || 'gcc'
  #   cc.flags = [ENV['CFLAGS'] || %w()]
  #   cc.include_paths = ["#{root}/include"]
  #   cc.defines = %w(DISABLE_GEMS)
  #   cc.option_include_path = '-I%s'
  #   cc.option_define = '-D%s'
  #   cc.compile_options = "%{flags} -MMD -o %{outfile} -c %{infile}"
  # end

  # mrbc settings
  # conf.mrbc do |mrbc|
  #   mrbc.compile_options = "-g -B%{funcname} -o-" # The -g option is required for line numbers
  # end

  # Linker settings
  # conf.linker do |linker|
  #   linker.command = ENV['LD'] || 'gcc'
  #   linker.flags = [ENV['LDFLAGS'] || []]
  #   linker.flags_before_libraries = []
  #   linker.libraries = %w()
  #   linker.flags_after_libraries = []
  #   linker.library_paths = []
  #   linker.option_library = '-l%s'
  #   linker.option_library_path = '-L%s'
  #   linker.link_options = "%{flags} -o %{outfile} %{objs} %{libs}"
  # end
 
  # Archiver settings
  # conf.archiver do |archiver|
  #   archiver.command = ENV['AR'] || 'ar'
  #   archiver.archive_options = 'rs %{outfile} %{objs}'
  # end
 
  # Parser generator settings
  # conf.yacc do |yacc|
  #   yacc.command = ENV['YACC'] || 'bison'
  #   yacc.compile_options = '-o %{outfile} %{infile}'
  # end
 
  # gperf settings
  # conf.gperf do |gperf|
  #   gperf.command = 'gperf'
  #   gperf.compile_options = '-L ANSI-C -C -p -j1 -i 1 -g -o -t -N mrb_reserved_word -k"1,3,$" %{infile} > %{outfile}'
  # end
  
  # file extensions
  # conf.exts do |exts|
  #   exts.object = '.o'
  #   exts.executable = '' # '.exe' if Windows
  #   exts.library = '.a'
  # end

  # file separetor
  # conf.file_separator = '/'
end

# Define cross build settings
# MRuby::CrossBuild.new('32bit') do |conf|
#   toolchain :gcc
#   
#   conf.cc.flags << "-m32"
#   conf.linker.flags << "-m32"
#
#   conf.build_mrbtest_lib_only
#   
#   conf.gem 'examples/mrbgems/c_and_ruby_extension_example'
#
#   conf.test_runner.command = 'env'
#
# end
