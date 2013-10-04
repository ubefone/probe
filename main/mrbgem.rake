MRuby::Gem::Specification.new('d3-probe') do |spec|
  spec.license = 'MIT'
  spec.authors = 'Julien Ammous'
  
  spec.bins = %w(d3probe)
  spec.linker.libraries << %w(pthread sigar)
end
