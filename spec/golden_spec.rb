# frozen_string_literal: true

RSpec.describe "Golden" do
  cases = [
    {
      desc: "Empty input",
      input: "",
      expected_kinds: [],
      errors: []
    },
    {
      desc: "Literal only",
      input: "hello",
      expected_kinds: [:literal],
      errors: []
    },
    {
      desc: "Simple open+close tag",
      input: "<div></div>",
      expected_kinds: %i[tag_begin right_angled tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "Self-closing JSX-style tag",
      input: "<br/>",
      expected_kinds: %i[tag_begin tag_end],
      errors: []
    },
    {
      desc: "Tag with literal attr value",
      input: "<div id=main>",
      expected_kinds: %i[tag_begin attr_key equal attr_value_unquoted right_angled],
      errors: []
    },
    {
      desc: "Tag with quoted string attr",
      input: "<img src=\"foo.png\">",
      expected_kinds: %i[tag_begin attr_key equal string right_angled],
      errors: []
    },
    {
      desc: "Tag with apostrophe string attr",
      input: "<div class='abc'>",
      expected_kinds: %i[tag_begin attr_key equal string right_angled],
      errors: []
    },
    {
      desc: "Attribute with executable value",
      input: "<span value={{foo}}>",
      expected_kinds: %i[tag_begin attr_key equal executable right_angled],
      errors: []
    },
    {
      desc: "Multiple attributes",
      input: "<div a=1 b=\"2\" c={{three}}>",
      expected_kinds: %i[tag_begin attr_key equal attr_value_unquoted attr_key equal string attr_key equal executable right_angled],
      errors: []
    },
    {
      desc: "Executable inside content",
      input: "<div>{{ foo }}</div>",
      expected_kinds: %i[tag_begin right_angled executable tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "Nested executables",
      input: "<div>{{ if(x) {{ y }} }}</div>",
      expected_kinds: %i[tag_begin right_angled executable tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "String with interpolation",
      input: "<div title=\"Hello {{name}}!\">",
      expected_kinds: %i[tag_begin attr_key equal string_interpolation interpolated_executable string right_angled],
      errors: []
    },
    {
      desc: "Escaped quote inside string",
      input: "<div title=\"a \\\"b\\\" c\">",
      expected_kinds: %i[tag_begin attr_key equal string right_angled],
      errors: []
    },
    {
      desc: "Unterminated string attr",
      input: "<div title=\"oops>",
      expected_kinds: %i[tag_begin attr_key equal string],
      errors: ["Unterminated string value"]
    },
    {
      desc: "Unterminated {{ block",
      input: "<div>{{ foo </div>",
      expected_kinds: %i[tag_begin right_angled],
      errors: ["Unmatched {{ block"]
    },
    {
      desc: "Attribute with interpolation in string",
      input: "<input placeholder=\"Name: {{user}}\">",
      expected_kinds: %i[tag_begin attr_key equal string_interpolation interpolated_executable string right_angled],
      errors: []
    },
    {
      desc: "Closing tag with attributes (not valid but should tokenize)",
      input: "</div class=bad>",
      expected_kinds: %i[tag_closing_start attr_key equal attr_value_unquoted tag_closing_end],
      errors: []
    },
    {
      desc: "Comment start",
      input: "<!-- comment -->",
      expected_kinds: %i[tag_begin tag_comment_end],
      errors: []
    },
    {
      desc: "Literal followed by tag",
      input: "foo <b>bar</b>",
      expected_kinds: %i[literal tag_begin right_angled literal tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "Deep nesting",
      input: "<div><span>{{x}}</span></div>",
      expected_kinds: %i[tag_begin right_angled tag_begin right_angled executable tag_closing_start tag_closing_end tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "Unicode text literal",
      input: "<p>Olá ☀️</p>",
      expected_kinds: %i[tag_begin right_angled literal tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "Entity in literal",
      input: "5 &lt; 6",
      expected_kinds: [:literal],
      errors: []
    },
    {
      desc: "Self-closing with space",
      input: "<img />",
      expected_kinds: %i[tag_begin tag_end],
      errors: []
    },
    {
      desc: "Attr with numeric literal",
      input: "<div count=123>",
      expected_kinds: %i[tag_begin attr_key equal attr_value_unquoted right_angled],
      errors: []
    },
    {
      desc: "Attr without value",
      input: "<input disabled>",
      expected_kinds: %i[tag_begin attr_key right_angled],
      errors: []
    },
    {
      desc: "Multiple spaces and newlines",
      input: "<div   \n   id =   \"foo\"   >",
      expected_kinds: %i[tag_begin attr_key equal string right_angled],
      errors: []
    },
    {
      desc: "Literal with {{ interpolation }} inside text",
      input: "Hello {{name}}!",
      expected_kinds: %i[literal executable literal],
      errors: []
    },
    {
      desc: "Nested tags with attrs",
      input: "<ul><li id=one>1</li><li id=two>2</li></ul>",
      expected_kinds: %i[tag_begin right_angled tag_begin attr_key equal attr_value_unquoted right_angled literal tag_closing_start tag_closing_end tag_begin attr_key equal attr_value_unquoted right_angled literal tag_closing_start tag_closing_end tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "Malformed tag missing >",
      input: "<div",
      expected_kinds: [:tag_begin],
      errors: []
    },
    {
      desc: "Literal with curly but not {{",
      input: "cost {100}",
      expected_kinds: [:literal],
      errors: []
    },
    {
      desc: "String with nested executable",
      input: "<div title=\"Hello {{name}}\">",
      expected_kinds: %i[tag_begin attr_key equal string_interpolation interpolated_executable string right_angled],
      errors: []
    },
    {
      desc: "Executable containing nested braces",
      input: "<div>{{ arr[ {x:1} ] }}</div>",
      expected_kinds: %i[tag_begin right_angled executable tag_closing_start tag_closing_end],
      errors: []
    }
  ].freeze

  cases.each do |test_case|
    it test_case[:desc] do
      inst = MiniHTML::Scanner.new(test_case[:input])
      tokens = inst.tokenize
      expect(tokens.map { it[:kind] }).to eq(test_case[:expected_kinds])

      next if test_case[:errors].empty?

      test_case[:errors].each.with_index do |err, idx|
        expect(inst.errors[idx]).to start_with(err)
      end
    end

    next unless test_case[:errors].empty?

    it "Parses #{test_case[:desc]}" do
      inst = MiniHTML::Parser.new(test_case[:input])
      inst.parse
    end
  end
end
