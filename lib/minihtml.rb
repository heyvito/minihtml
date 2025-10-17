# frozen_string_literal: true

require_relative "minihtml/version"
require_relative "minihtml/minihtml_scanner"
require_relative "minihtml/minihtml_token_stream"

require_relative "minihtml/ast"
require_relative "minihtml/parser"

module MiniHTML
  class Error < StandardError; end

  class ParseError < Error
    attr_reader :errors

    def initialize(*errors)
      @errors = errors
      super("Parsing failed with #{errors.length} error#{"s" if errors.length > 1}: #{errors.join(", ")}")
    end
  end
end
