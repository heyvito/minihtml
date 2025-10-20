# frozen_string_literal: true

module MiniHTML
  class Parser
    attr_reader :stream

    def initialize(source)
      scanner = MiniHTML::Scanner.new(source)
      tokens = scanner.tokenize
      raise ParseError.new(*scanner.errors) unless scanner.errors.empty?

      @stream = MiniHTML::TokenStream.new(tokens)
      @tokens = []
    end

    def parse
      @tokens << parse_one until stream.empty?
      @tokens
    end

    def parse_one
      case stream.peek_kind
      when :literal
        AST::PlainText.new(stream.consume)
      when :tag_begin
        if stream.peek[:literal] == "<!--"
          parse_comment
        else
          parse_tag
        end
      when :attr_value_unquoted
        AST::Literal.new(stream.consume)
      when :string
        AST::String.new(stream.consume)
      when :executable
        AST::Executable.new(stream.consume)
      when :string_interpolation
        parse_string_interpolation
      when :tag_closing_start
        tag = AST::Tag.new(stream.consume)
        discard_until_tag_end
        tag
      else
        raise "Unexpected token type #{stream.peek_kind} on #parse_one"
      end
    end

    def parse_comment
      stream.consume
      AST::Comment.new(stream.consume) unless stream.empty?
    end

    def parse_string_interpolation
      interp = AST::Interpolation.new(stream.consume)
      until stream.empty?
        case stream.peek_kind
        when :executable
          interp.values << parse_one
        when :string_interpolation
          interp.values << AST::String.new(stream.consume)
        when :string
          interp.values << AST::String.new(stream.consume)
          return interp
        when :interpolated_executable
          interp.values << AST::Executable.new(stream.consume)
        else
          raise "Unexpected token type #{stream.peek_kind} on #parse_string_interpolation"
        end
      end
      interp
    end

    def discard_until_tag_end
      stream.consume until stream.empty? || %i[tag_end tag_closing_end].include?(stream.peek_kind)
      stream.consume # Consume tag_end or tag_closing_end
    end

    def parse_tag
      tag = AST::Tag.new(stream.consume)
      until stream.empty?
        case stream.peek_kind
        when :right_angled
          stream.consume
          # This tag has children...
          tag.children << parse_one until stream.peek_kind == :tag_closing_start || stream.empty?
        when :tag_closing_start
          if stream.peek[:literal][2...] == tag.name
            # Consume everything until a closing_end
            discard_until_tag_end
            return tag
          end
        when :tag_end
          stream.consume
          tag.self_closing = true
          return tag
        when :attr_key
          tag.attributes << parse_attr
        else
          raise "Unexpected token type #{stream.peek_kind} on #parse_tag"
        end
      end
    end

    def parse_attr
      att = AST::Attr.new(stream.consume)
      return att unless stream.peek_kind == :equal

      stream.consume # equal
      att.value = parse_one
      att
    end
  end
end
