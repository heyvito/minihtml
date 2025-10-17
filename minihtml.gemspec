# frozen_string_literal: true

require_relative "lib/minihtml/version"

Gem::Specification.new do |spec|
  spec.name = "minihtml"
  spec.version = MiniHTML::VERSION
  spec.authors = ["Vito Sartori"]
  spec.email = ["hey@vito.io"]

  spec.summary = "MiniHTML is a small HTML parser intended for React-like HTML-JSX syntax"
  spec.description = spec.summary
  spec.homepage = "https://github.com/heyvito/minihtml"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 3.4"

  spec.metadata["allowed_push_host"] = "https://rubygems.org"

  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = spec.homepage
  spec.metadata["changelog_uri"] = spec.homepage
  spec.metadata["rubygems_mfa_required"] = "true"

  gemspec = File.basename(__FILE__)
  spec.files = IO.popen(%w[git ls-files -z], chdir: __dir__, err: IO::NULL) do |ls|
    ls.readlines("\x0", chomp: true).reject do |f|
      (f == gemspec) ||
        f.start_with?(*%w[bin/ test/ spec/ features/ .git .github appveyor Gemfile])
    end
  end

  spec.bindir = "exe"
  spec.executables = spec.files.grep(%r{\Aexe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]
  spec.extensions = ["ext/minihtml_scanner/extconf.rb", "ext/minihtml_token_stream/extconf.rb"]
end
