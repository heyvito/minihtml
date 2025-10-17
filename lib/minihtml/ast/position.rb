# frozen_string_literal: true

module MiniHTML
  module AST
    class Position
      attr_reader :line, :column, :offset

      def initialize(line:, column:, offset:)
        @line = line
        @column = column
        @offset = offset
      end
    end
  end
end
