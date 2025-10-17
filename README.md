# MiniHTML

MiniHTML is a tiny HTML tokenizer and parser with a JSX-inspired syntax. It is designed to power template-like inputs that mix static markup with executable expressions, returning a small, Ruby-friendly abstract syntax tree (AST).

## Features

- Understands standard HTML tags plus component-style names such as `Foo::Bar`.
- Supports self-closing tags (`<Tag />`) and nested child nodes.
- Handles attributes without values as well as string, literal, or executable (`{{ expr }}`) attribute values.
- Preserves plain text nodes, comment nodes (`<!-- ... -->`), and boolean attributes.
- Provides string interpolation inside quoted values (`title="Hello {{name}}"`).
- Produces rich AST nodes (`MiniHTML::AST::Tag`, `PlainText`, `String`, `Literal`, `Executable`, `Interpolation`, `Comment`, `Attr`) that include source locations.
- Raises descriptive errors for unterminated strings, comments, or interpolation blocks during tokenization.

## Installation

This project ships as a Ruby gem with native extensions and requires Ruby 3.4 or newer.

Add it to your `Gemfile` directly from GitHub until it is published to RubyGems:

```ruby
gem "minihtml"
```

Then install the dependencies:

```bash
bundle install
```

The extensions use the standard Ruby build toolchain (`extconf.rb`); make sure a compiler and headers are available on your system.

## Usage

### Parsing markup into an AST

```ruby
require "minihtml"

source = <<~HTML
  <header id="greeting" data-count={{items.size}}>
    Hello {{user.name}}!
    <Button disabled />
    <Link href="/users/{{user.id}}">View profile</Link>
  </header>
HTML

parser = MiniHTML::Parser.new(source)
ast = parser.parse

ast.first # => #<MiniHTML::AST::Tag name="header" ...>
```

Every node carries positional metadata so you can map AST entries back to their origin in the source string.

### Working with tokens directly

If you only need lexical analysis, you can use the scanner extension on its own:

```ruby
scanner = MiniHTML::Scanner.new("<div title=\"Hello {{name}}\">")
tokens = scanner.tokenize

# => [{ kind: :tag_begin, literal: "<div" }, ...]
scanner.errors # => []
```

Errors gathered during scanning are exposed through `scanner.errors`. The parser raises `MiniHTML::ParseError` when scanning fails, wrapping the collected messages.

## Supported syntax and limitations

- Tag names must start with a letter. Subsequent characters may include digits, underscores, colons, or dots. Hyphenated component names (e.g. `<my-component>`) are not recognised.
- Attribute names must begin with a letter and may contain digits, dashes, underscores, or dots. Unquoted attribute values are limited to identifier characters (`[A-Za-z0-9_:.]`).
- Executable sections are delimited by `{{` and `}}`. Nested braces inside the executable body are balanced correctly, but unmatched blocks trigger an error.
- Interpolation inside strings reuses executable parsing; escaped quotes (`\"`) are preserved in the resulting `AST::String` literal.
- HTML entities are not decoded; the parser leaves them exactly as written.
- Doctype declarations, script/style parsing nuances, and malformed HTML recovery are out of scope.

## Development

```bash
bin/setup    # install dependencies
bundle exec rake
```

Both the scanner and token stream extensions live under `ext/`; rerun `bundle exec rake compile` after making changes to the C sources.

## License

```
The MIT License (MIT)

Copyright (c) 2025 Vito Sartori

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```
