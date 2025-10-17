# frozen_string_literal: true

RSpec.describe "Parsing" do
  it "parses nested components" do
    source = <<~HTML
      <header id="foobar" cx-ref="bla">
        <Banner />
        <UserSelector name={{name}} open={{false}} />
      </header>
    HTML

    ast = MiniHTML::Parser.new(source.strip).parse
    expect(ast.length).to eq 1
    container = ast.first
    expect(container).to be_a MiniHTML::AST::Tag
    expect(container).not_to be_self_closing
    expect(container.name).to eq "header"
    expect(container.attributes.length).to eq 2
    expect(container.attributes[0].name).to eq "id"
    expect(container.attributes[0].value).to be_a MiniHTML::AST::String
    expect(container.attributes[0].value.literal).to eq "foobar"

    expect(container.attributes[1].name).to eq "cx-ref"
    expect(container.attributes[1].value).to be_a MiniHTML::AST::String
    expect(container.attributes[1].value.literal).to eq "bla"

    expect(container.children.length).to eq 5

    banner = container.children[1]
    expect(banner).to be_a MiniHTML::AST::Tag
    expect(banner.name).to eq "Banner"
    expect(banner).to be_self_closing

    selector = container.children[3]
    expect(selector).to be_a MiniHTML::AST::Tag
    expect(selector.name).to eq "UserSelector"
    expect(selector).to be_self_closing

    expect(selector.attributes.length).to eq 2
    expect(selector.attributes[0].name).to eq "name"
    expect(selector.attributes[0].value).to be_a MiniHTML::AST::Executable
    expect(selector.attributes[0].value.source).to eq "name"

    expect(selector.attributes[1].name).to eq "open"
    expect(selector.attributes[1].value).to be_a MiniHTML::AST::Executable
    expect(selector.attributes[1].value.source).to eq "false"
  end

  it "parses tags with Ruby path" do
    source = <<~HTML
      <Foo::Bar::Banner />
    HTML

    ast = MiniHTML::Parser.new(source.strip).parse
    expect(ast.length).to eq 1

    tag = ast.first
    expect(tag.name).to eq "Foo::Bar::Banner"
    expect(tag.attributes).to be_empty
    expect(tag.children).to be_empty
    expect(tag).to be_self_closing
  end
end
