# frozen_string_literal: true

require "bundler/gem_tasks"
require "rspec/core/rake_task"

RSpec::Core::RakeTask.new(:spec)

require "rubocop/rake_task"

RuboCop::RakeTask.new

require "rake/extensiontask"

task build: :compile

GEMSPEC = Gem::Specification.load("minihtml.gemspec")

Rake::ExtensionTask.new("minihtml_scanner", GEMSPEC) do |ext|
  ext.lib_dir = "lib/minihtml"
end

Rake::ExtensionTask.new("minihtml_token_stream", GEMSPEC) do |ext|
  ext.lib_dir = "lib/minihtml"
end

task default: %i[clobber compile spec rubocop]
