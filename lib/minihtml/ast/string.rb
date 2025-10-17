# frozen_string_literal: true

module MiniHTML
  module AST
    class String < Base
      attr_accessor :quote, :literal

      def initialize(token)
        super
        @literal = token[:literal].gsub(/\\#{token[:quote_char]}/, token[:quote_char])
        @quote = token[:quote_char]
      end
    end
  end
end
