# -*- encoding: utf-8 -*-
require File.expand_path("../lib/clipper/version", __FILE__)

Gem::Specification.new do |s|

  s.name        = "clipper"
  s.version     = Clipper::VERSION
  s.platform    = Gem::Platform::RUBY
  s.authors     = ['Angus Johnson','Mike Owens']
  s.email       = []
  s.homepage    = "http://github.com/mieko/rbclipper"
  s.summary     = "Ruby wrapper for Clipper, Angus Johnson's excellent polygon clipping library"
  s.description = "Builds a native ruby extension for Clipper"

  s.has_rdoc          = true
  s.extra_rdoc_files  = %w(README.md Changelog ext/clipper/rbclipper.cpp)
  s.rdoc_options.concat %w(--main=README.md)

  s.required_rubygems_version = ">= 1.3.6"
  s.rubyforge_project         = "clipper"

  s.add_development_dependency "bundler", ">= 1.0.0"
  s.files        = Dir["**/clipper/*", "LICENSE*", "Rakefile", "Gemfile", "Changelog", "*.md"]
  s.executables  = Dir["**/clipper/*.so"]
  s.require_path = 'lib'
  
  s.extensions = "ext/clipper/extconf.rb"

end
