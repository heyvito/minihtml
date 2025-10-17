# frozen_string_literal: true

module MiniHTML
  module AST
    class Comment < Base
      attr_accessor :literal

      def initialize(token)
        super
        @literal = token[:literal]
      end
    end
  end
end
