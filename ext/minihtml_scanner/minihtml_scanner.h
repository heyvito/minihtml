#ifndef MINIHTML_H
#define MINIHTML_H 1

#include "ruby.h"

#define SPACE ' '
#define CARRIAGE_RETURN '\r'
#define FORM_FEED '\f'
#define NEWLINE  0x0A
#define HORIZONTAL_TAB '\t'
#define VERTICAL_TAB '\v'
#define ANGLED_LEFT '<'
#define ANGLED_RIGHT '>'
#define SOLIDUS '/'
#define CURLY_LEFT '{'
#define CURLY_RIGHT '}'
#define BANG '!'
#define MINUS_HYPHEN '-'
#define UNDERSCORE '_'
#define EQUAL '='
#define APOSTROPHE '\''
#define QUOTE '"'
#define INVERTED_SOLIDUS '\\'
#define PERIOD '.'
#define COLON ':'

#define DEFINE_REUSABLE_SYMBOL(name) static ID id_type_##name; static VALUE sym_##name;
#define INITIALIZE_REUSABLE_SYMBOL(name) id_type_##name = rb_intern(#name); sym_##name = ID2SYM(id_type_##name);

DEFINE_REUSABLE_SYMBOL(line);
DEFINE_REUSABLE_SYMBOL(column);
DEFINE_REUSABLE_SYMBOL(offset);
DEFINE_REUSABLE_SYMBOL(start_line);
DEFINE_REUSABLE_SYMBOL(start_column);
DEFINE_REUSABLE_SYMBOL(start_offset);
DEFINE_REUSABLE_SYMBOL(end_line);
DEFINE_REUSABLE_SYMBOL(end_column);
DEFINE_REUSABLE_SYMBOL(end_offset);
DEFINE_REUSABLE_SYMBOL(new);
DEFINE_REUSABLE_SYMBOL(literal);
DEFINE_REUSABLE_SYMBOL(self_closing);
DEFINE_REUSABLE_SYMBOL(tag_begin);
DEFINE_REUSABLE_SYMBOL(tag_end);
DEFINE_REUSABLE_SYMBOL(tag_closing_start);
DEFINE_REUSABLE_SYMBOL(tag_closing_end);
DEFINE_REUSABLE_SYMBOL(right_angled);
DEFINE_REUSABLE_SYMBOL(attr_key);
DEFINE_REUSABLE_SYMBOL(kind);
DEFINE_REUSABLE_SYMBOL(string);
DEFINE_REUSABLE_SYMBOL(string_interpolation);
DEFINE_REUSABLE_SYMBOL(interpolated_executable);
DEFINE_REUSABLE_SYMBOL(executable);
DEFINE_REUSABLE_SYMBOL(equal);
DEFINE_REUSABLE_SYMBOL(quote_char);
DEFINE_REUSABLE_SYMBOL(tag_comment_end);
DEFINE_REUSABLE_SYMBOL(attr_value_unquoted);

typedef struct {
    VALUE str;
    VALUE tokens;
    VALUE errors;
    const uint8_t *p;
    const uint8_t *end;
    long idx_cp;
    int look[4];
    long line;
    long col;
    long start_token_offset;
    long start_token_line;
    long start_token_column;
} scanner_t;

#endif /* MINIHTML_H */
