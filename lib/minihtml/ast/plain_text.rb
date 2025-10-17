# frozen_string_literal: true

module MiniHTML
  module AST
    class PlainText < Base
      attr_accessor :literal

      def initialize(token)
        super
        @literal = token[:literal]
      end
    end
  end
end
