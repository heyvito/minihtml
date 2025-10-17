# frozen_string_literal: true

module MiniHTML
  module AST
    class Base
      attr_reader :position_start, :position_end, :original_token

      def initialize(token)
        @original_token = token
        @position_start = Position.new(line: token[:start_line], column: token[:start_column], offset: token[:start_offset])
        @position_end = Position.new(line: token[:end_line], column: token[:end_column], offset: token[:end_offset])
      end
    end
  end
end
