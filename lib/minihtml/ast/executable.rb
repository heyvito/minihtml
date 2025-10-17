# frozen_string_literal: true

module MiniHTML
  module AST
    class Executable < Base
      attr_accessor :source

      def initialize(token)
        super
        @source = token[:literal]
      end
    end
  end
end
