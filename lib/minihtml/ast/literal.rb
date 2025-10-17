# frozen_string_literal: true

module MiniHTML
  module AST
    class Literal < Base
      attr_accessor :value

      def initialize(token)
        super
        @value = token[:literal]
      end
    end
  end
end
