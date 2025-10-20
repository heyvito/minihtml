# frozen_string_literal: true

RSpec.describe "Minimal" do
  cases = [
    {
      desc: "Plain literal text",
      input: "hello world",
      expected_kinds: [:literal],
      errors: []
    },
    {
      desc: "Simple tag",
      input: "<div>",
      expected_kinds: %i[tag_begin right_angled],
      errors: []
    },
    {
      desc: "Self-closing tag (JSX style)",
      input: "<br/>",
      expected_kinds: %i[tag_begin tag_end],
      errors: []
    },
    {
      desc: "Attribute with string value",
      input: "<img src=\"foo.png\">",
      expected_kinds: %i[tag_begin attr_key equal string right_angled],
      errors: []
    },
    {
      desc: "Attribute with literal value",
      input: "<div class=main>",
      expected_kinds: %i[tag_begin attr_key equal attr_value_unquoted right_angled],
      errors: []
    },
    {
      desc: "Attribute with executable value",
      input: "<span value={{foo}}>",
      expected_kinds: %i[tag_begin attr_key equal executable right_angled],
      errors: []
    },
    {
      desc: "Executable inside body",
      input: "<div>{{ user.name }}</div>",
      expected_kinds: %i[tag_begin right_angled executable tag_closing_start tag_closing_end],
      errors: []
    },
    {
      desc: "String with interpolation",
      input: "<div title=\"Hello {{name}}!\">",
      expected_kinds: %i[
        tag_begin attr_key equal
        string_interpolation interpolated_executable string
        right_angled
      ],
      errors: []
    },
    {
      desc: "Unterminated string error",
      input: "<div title=\"oops>",
      expected_kinds: %i[tag_begin attr_key equal string],
      errors: ["Unterminated string value"]
    },
    {
      desc: "Unterminated {{ block error",
      input: "<div>{{ foo </div>",
      expected_kinds: %i[tag_begin right_angled],
      errors: ["Unmatched {{ block"]
    }
  ].freeze

  cases.each do |test_case|
    it test_case[:desc] do
      inst = MiniHTML::Scanner.new(test_case[:input])
      tokens = inst.tokenize
      expect(tokens.map { it[:kind] }).to eq(test_case[:expected_kinds])
    end
  end
end
