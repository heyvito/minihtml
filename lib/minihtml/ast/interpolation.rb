# frozen_string_literal: true

module MiniHTML
  module AST
    class Interpolation < Base
      attr_accessor :values

      def initialize(token)
        super
        @values = [AST::String.new(token)]
      end
    end
  end
end
