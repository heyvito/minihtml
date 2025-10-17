# frozen_string_literal: true

module MiniHTML
  module AST
    class Attr < Base
      attr_accessor :name
      attr_reader :value

      def initialize(token)
        super
        @name = token[:literal]
        @value = nil
      end

      def value=(token)
        @position_end = token.position_end
        @value = token
      end
    end
  end
end
