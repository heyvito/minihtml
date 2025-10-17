# frozen_string_literal: true

module MiniHTML
  module AST
    class Tag < Base
      attr_accessor :name, :self_closing, :attributes, :children, :bad_tag
      alias self_closing? self_closing
      alias bad_tag? bad_tag

      def initialize(token)
        super
        lit = token[:literal]
        if lit.start_with? "</"
          @bad_tag = true
          @name = token[:literal][2...]
        else
          @name = token[:literal][1...]
        end

        @self_closing = false
        @attributes = []
        @children = []
      end
    end
  end
end
