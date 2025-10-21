#include "ruby.h"
#include "ruby/encoding.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "minihtml_scanner.h"

#define EOF_CP   (-1)

/**
 * next_utf8_cp - Decode the next UTF-8 code point from a byte stream.
 *
 * @p:   Pointer to a pointer to the current byte. This pointer will be
 *       advanced past the consumed sequence on success or error.
 * @end: Pointer to one past the last valid byte in the buffer.
 *
 * Returns:
 *   - A valid Unicode code point (U+0000..U+10FFFF) if decoding succeeds.
 *   - 0xFFFD (replacement character) if the sequence is malformed
 *     (invalid continuation, overlong encoding, surrogate, or out-of-range).
 *   - EOF_CP if @p has reached or passed @end (no more input).
 *
 * Behavior:
 *   - Handles 1–4 byte UTF-8 sequences.
 *   - Performs boundary checks to avoid reading past @end.
 *   - Rejects overlong encodings and surrogate code points (U+D800..U+DFFF).
 *   - Does not allocate memory; operates directly on caller-provided buffer.
 *
 * Example:
 *   const uint8_t *ptr = buf, *end = buf + len;
 *   int cp;
 *   while ((cp = next_utf8_cp(&ptr, end)) != EOF_CP) {
 *       if (cp == 0xFFFD) { handle invalid sequence }
 *       else { process code point }
 *   }
 */
static inline int next_utf8_cp(const uint8_t **p, const uint8_t *end) {
    if (*p >= end) return EOF_CP;

    const uint8_t b0 = **p;
    (*p)++;

    if (b0 < 0x80) return b0;

    if ((b0 & 0xE0) == 0xC0) {
        // 2 bytes
        if (*p >= end) return EOF_CP;

        const uint8_t b1 = **p;
        (*p)++;
        if ((b1 & 0xC0) != 0x80) return 0xFFFD;

        const int cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        return cp < 0x80 ? 0xFFFD : cp;
    }

    if ((b0 & 0xF0) == 0xE0) {
        // 3 bytes
        if (*p + 1 >= end) return EOF_CP;

        const uint8_t b1 = **p;
        const uint8_t b2 = *(*p + 1);
        (*p) += 2;
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return 0xFFFD;

        const int cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);

        // exclude surrogates
        if (cp >= 0xD800 && cp <= 0xDFFF) return 0xFFFD;

        return cp < 0x800 ? 0xFFFD : cp;
    }

    if ((b0 & 0xF8) == 0xF0) {
        // 4 bytes
        if (*p + 2 >= end) return EOF_CP;

        const uint8_t b1 = **p;
        const uint8_t b2 = *(*p + 1);
        const uint8_t b3 = *(*p + 2);
        (*p) += 3;

        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return 0xFFFD;

        const int cp = ((b0 & 0x07) << 18)
                       | ((b1 & 0x3F) << 12)
                       | ((b2 & 0x3F) << 6)
                       | (b3 & 0x3F);
        if (cp > 0x10FFFF) return 0xFFFD;

        return cp < 0x10000 ? 0xFFFD : cp;
    }

    return 0xFFFD; // invalid leading byte
}

static inline bool scanner_is_letter(const int v) {
    return (v >= 'A' && v <= 'Z') || (v >= 'a' && v <= 'z');
}

static inline bool scanner_is_space(const int v) {
    return v == SPACE || v == CARRIAGE_RETURN || v == FORM_FEED || v == NEWLINE || v == HORIZONTAL_TAB || v ==
           VERTICAL_TAB;
}

static inline bool scanner_is_tag_ident(const int v) {
    return v != EOF_CP && (scanner_is_letter(v) || isdigit(v) || v == UNDERSCORE || v == PERIOD || v == COLON);
}

static void scanner_free(void *ptr) {
    xfree(ptr);
}

static size_t scanner_memsize(const void *ptr) {
    return sizeof(scanner_t);
}

static void scanner_mark(void *ptr) {
    const scanner_t *scanner = ptr;
    if (scanner->str) rb_gc_mark(scanner->str);

    if (scanner->tokens) rb_gc_mark(scanner->tokens);

    if (scanner->errors) rb_gc_mark(scanner->errors);
}

static const rb_data_type_t scanner_type = {
    "MiniHTML::Scanner",
    {scanner_mark, scanner_free, scanner_memsize},
    0, 0, RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE scanner_alloc(const VALUE klass) {
    scanner_t *t = ALLOC(scanner_t);
    memset(t, 0, sizeof(scanner_t));
    return TypedData_Wrap_Struct(klass, &scanner_type, t);
}

static VALUE scanner_initialize(VALUE self, VALUE str) {
    Check_Type(str, T_STRING);
    scanner_t *t;
    TypedData_Get_Struct(self, scanner_t, &scanner_type, t);

    str = rb_str_dup(str);
    t->str = str;
    t->p = (const uint8_t *) RSTRING_PTR(str);
    t->end = t->p + RSTRING_LEN(str);
    t->idx_cp = 0;
    t->tokens = rb_ary_new();
    t->errors = rb_ary_new();
    t->line = 1;
    t->col = 1;

    // prime lookahead
    t->look[0] = next_utf8_cp(&t->p, t->end);
    t->look[1] = next_utf8_cp(&t->p, t->end);
    t->look[2] = next_utf8_cp(&t->p, t->end);
    t->look[3] = next_utf8_cp(&t->p, t->end);

    return self;
}

static inline void rotate(scanner_t *t) {
    t->look[0] = t->look[1];
    t->look[1] = t->look[2];
    t->look[2] = t->look[3];
    t->look[3] = next_utf8_cp(&t->p, t->end);
}

static VALUE scanner_start_token(scanner_t *t) {
    t->start_token_offset = t->idx_cp;
    t->start_token_line = t->line;
    t->start_token_column = t->col;
    return Qnil;
}

static void scanner_consume(scanner_t *t) {
    const int v = t->look[0];
    if (v == EOF_CP) return;

    t->idx_cp += 1;
    if (v == NEWLINE) {
        t->line += 1;
        t->col = 1;
    } else {
        t->col += 1;
    }

    rotate(t);
}

static void scanner_consume_tag_ident(scanner_t *t) {
    while (scanner_is_tag_ident(t->look[0])) {
        scanner_consume(t);
    }
}

static void scanner_amend_last_token_kind(const scanner_t *t, const VALUE newKind) {
    const VALUE v = rb_ary_entry(t->tokens, -1);
    rb_hash_aset(v, sym_kind, newKind);
}

static void scanner_push_token_simple(const scanner_t *t, const VALUE type) {
    const VALUE h = rb_hash_new();
    rb_hash_aset(h, sym_kind, type);
    rb_hash_aset(h, sym_start_line, LONG2FIX(t->start_token_line));
    rb_hash_aset(h, sym_start_column, LONG2FIX(t->start_token_column));
    rb_hash_aset(h, sym_start_offset, LONG2FIX(t->start_token_offset));
    rb_hash_aset(h, sym_end_line, LONG2FIX(t->line));
    rb_hash_aset(h, sym_end_column, LONG2FIX(t->col));
    rb_hash_aset(h, sym_end_offset, LONG2FIX(t->idx_cp));
    rb_ary_push(t->tokens, h);
}

static inline void scanner_consume_spaces(scanner_t *t) {
    while (scanner_is_space(t->look[0])) {
        scanner_consume(t);
    }
}

static void scanner_consume_executable(scanner_t *t) {
    scanner_consume(t); // {
    scanner_consume(t); // {
    scanner_start_token(t);

    int bracketLevel = 0;
    while (t->look[0] != EOF_CP) {
        if (bracketLevel == 0 && t->look[0] == CURLY_RIGHT && t->look[1] == CURLY_RIGHT) {
            scanner_push_token_simple(t, sym_executable);
            scanner_consume(t); // }
            scanner_consume(t); // }
            return;
        }

        if (t->look[0] == CURLY_LEFT) bracketLevel++;
        if (t->look[0] == CURLY_RIGHT) bracketLevel--;

        scanner_consume(t);
    }
    char errStr[128] = {0};
    snprintf(errStr, 127, "Unmatched {{ block at line %lu, column %lu, offset %lu", t->line, t->col, t->idx_cp);
    rb_ary_push(t->errors, rb_str_new_cstr(errStr));
}

static void scanner_set_string_quote_value(const scanner_t *t, const char quoteChar) {
    const VALUE token = rb_ary_entry(t->tokens, -1);
    const char value[2] = {quoteChar, 0};
    rb_hash_aset(token, sym_quote_char, rb_str_new_cstr(value));
}

static void scanner_consume_string(scanner_t *t) {
    const int quoteChar = t->look[0];
    scanner_consume(t); // " or '
    scanner_start_token(t);
    while (t->look[0] != EOF) {
        if (t->look[0] == INVERTED_SOLIDUS && t->look[1] == quoteChar) {
            scanner_consume(t); // '\'
            scanner_consume(t); // quoteChar
        } else if (t->look[0] == quoteChar) {
            scanner_push_token_simple(t, sym_string);
            scanner_set_string_quote_value(t, (char)quoteChar);
            scanner_consume(t);
            return;
        } else if (t->look[0] == CURLY_LEFT && t->look[1] == CURLY_LEFT) {
            scanner_push_token_simple(t, sym_string_interpolation);
            scanner_set_string_quote_value(t, (char)quoteChar);
            scanner_consume_executable(t);
            scanner_amend_last_token_kind(t, sym_interpolated_executable);
            scanner_start_token(t);
        } else {
            scanner_consume(t);
        }
    }

    char errStr[128] = {0};
    snprintf(errStr, 127, "Unterminated string value at line %lu, column %lu, offset %lu", t->line, t->col, t->idx_cp);
    rb_ary_push(t->errors, rb_str_new_cstr(errStr));
    scanner_push_token_simple(t, sym_string);
    scanner_set_string_quote_value(t, (char)quoteChar);
}

static inline bool scanner_is_attr_ident(const int p) {
    return (p >= 'A' && p <= 'Z')
        || (p >= 'a' && p <= 'z')
        || (p >= '0' && p <= '9')
        || p == '-' || p == '_' || p == '.' || p == ':';
}

static void scanner_consume_attr_name(scanner_t *t) {
    while (scanner_is_attr_ident(t->look[0])) {
        scanner_consume(t);
    }
}

static void scanner_consume_unquoted_attr_value(scanner_t *t) {
  bool consumed = false;
  while (t->look[0] != EOF && !scanner_is_space(t->look[0]) && t->look[0] != SOLIDUS && t->look[0] != ANGLED_RIGHT) {
    scanner_consume(t);
    consumed = true;
  }
  if (consumed) scanner_push_token_simple(t, sym_attr_value_unquoted);
}

static void scanner_consume_attr(scanner_t *t) {
    scanner_start_token(t);
    scanner_consume_attr_name(t);
    scanner_push_token_simple(t, sym_attr_key);
    scanner_consume_spaces(t);
    if (t->look[0] == EQUAL) {
        scanner_start_token(t);
        scanner_consume(t); // =
        scanner_push_token_simple(t, sym_equal);
        scanner_consume_spaces(t);
        switch (t->look[0]) {
            case APOSTROPHE:
            case QUOTE:
                scanner_consume_string(t);
                break;
            case CURLY_LEFT:
                if (t->look[1] == CURLY_LEFT) {
                    scanner_consume_executable(t);
                    break;
                }
            default:
                scanner_start_token(t);
                scanner_consume_unquoted_attr_value(t);
                break;
        }
    }
}

static void scanner_consume_comment_tag(scanner_t *t) {
    while (t->look[0] != EOF) {
        if (t->look[0] == MINUS_HYPHEN && t->look[1] == MINUS_HYPHEN && t->look[2] == ANGLED_RIGHT) {
            scanner_consume(t); // -
            scanner_consume(t); // -
            scanner_consume(t); // >
            scanner_push_token_simple(t, sym_tag_comment_end);
            return;
        }
        scanner_consume(t);
    }

    // If we reach this point, it's an error.
    char errStr[128] = {0};
    snprintf(errStr, 127, "Unterminated comment tag at line %lu, column %lu, offset %lu", t->line, t->col, t->idx_cp);
    rb_ary_push(t->errors, rb_str_new_cstr(errStr));
    scanner_push_token_simple(t, sym_tag_comment_end);
}

static void scanner_scan_open_tag(scanner_t *t) {
    if (t->look[1] == '!' && t->look[2] == '-' && t->look[3] == '-') {
        scanner_start_token(t);
        scanner_consume(t); // <
        scanner_consume(t); // !
        scanner_consume(t); // -
        scanner_consume(t); // -
        scanner_push_token_simple(t, sym_tag_begin);
        scanner_consume_comment_tag(t);
        return;
    }

    if (scanner_is_letter(t->look[1])) {
        scanner_start_token(t);
        scanner_consume(t); // <
        scanner_consume_tag_ident(t); // \w
        scanner_push_token_simple(t, sym_tag_begin);

        scanner_consume_spaces(t);
        while (scanner_is_letter(t->look[0])) {
            scanner_consume_attr(t);
            scanner_consume_spaces(t);
        }
        return;
    }

    if (t->look[1] == '/') {
        scanner_start_token(t);
        scanner_consume(t); // <
        scanner_consume(t); // /

        if (scanner_is_tag_ident(t->look[0])) {
            scanner_consume_tag_ident(t);
        }
        scanner_push_token_simple(t, sym_tag_closing_start);
        scanner_consume_spaces(t);
        while (scanner_is_tag_ident(t->look[0])) {
            scanner_consume_attr(t);
            scanner_consume_spaces(t);
        }
        scanner_consume_spaces(t);

        if (t->look[0] == ANGLED_RIGHT) {
            scanner_start_token(t);
            scanner_consume(t); // >
            scanner_push_token_simple(t, sym_tag_closing_end);
        }
    }
}

static void scanner_consume_literal(scanner_t *t) {
    scanner_start_token(t);
    bool next = true;
    while (next) {
        switch (t->look[0]) {
            case ANGLED_LEFT:
                next = false;
                break;
            case CURLY_LEFT:
                if (t->look[1] == CURLY_LEFT) {
                    next = false;
                    break;
                }
                scanner_consume(t);
                break;
            case EOF:
                next = false;
                break;
            default:
                scanner_consume(t);
        }
    }
    scanner_push_token_simple(t, sym_literal);
}

static VALUE scanner_scan_token(scanner_t *t) {
    switch (t->look[0]) {
        case ANGLED_LEFT:
            scanner_scan_open_tag(t);
            break;
        case ANGLED_RIGHT:
            scanner_start_token(t);
            scanner_consume(t); // >
            scanner_push_token_simple(t, sym_right_angled);
            break;
        case SOLIDUS:
            if (t->look[1] == '>') {
                scanner_start_token(t);
                scanner_consume(t); // '/'
                scanner_consume(t); // >
                scanner_push_token_simple(t, sym_tag_end);
                break;
            }

            scanner_consume_literal(t);
            break;
        case CURLY_LEFT:
            if (t->look[1] == CURLY_LEFT) {
                scanner_consume_executable(t);
            } else {
                scanner_consume_literal(t);
            }
            break;
        case EOF:
            break;
        default:
            scanner_consume_literal(t);
            break;
    }

    return Qnil;
}

static VALUE scanner_tokens(const VALUE self) {
    scanner_t *t;
    TypedData_Get_Struct(self, scanner_t, &scanner_type, t);
    return t->tokens;
}

static VALUE scanner_errors(const VALUE self) {
    scanner_t *t;
    TypedData_Get_Struct(self, scanner_t, &scanner_type, t);
    return t->errors;
}

static VALUE scanner_stats(const VALUE self) {
    scanner_t *t;
    TypedData_Get_Struct(self, scanner_t, &scanner_type, t);
    const VALUE h = rb_hash_new();
    rb_hash_aset(h, sym_line, LONG2NUM(t->line));
    rb_hash_aset(h, sym_column, LONG2NUM(t->col));
    rb_hash_aset(h, sym_offset, LONG2NUM(t->idx_cp));
    return h;
}

static VALUE scanner_at_eof(const VALUE self) {
    scanner_t *t;
    TypedData_Get_Struct(self, scanner_t, &scanner_type, t);
    return t->look[0] == EOF ? Qtrue : Qfalse;
}

static void scanner_hydrate_tokens(const scanner_t *t) {
    const long len = RARRAY_LEN(t->tokens);
    for (long i = 0; i < len; i++) {
        const VALUE v = rb_ary_entry(t->tokens, i);
        const long startOffset = NUM2LONG(rb_hash_aref(v, sym_start_offset));
        const long endOffset = NUM2LONG(rb_hash_aref(v, sym_end_offset));
        const long strLen = endOffset - startOffset;
        if (strLen < 0) {
            rb_raise(rb_eRuntimeError, "invalid offset boundaries %ld -> %ld", startOffset, endOffset);
        }
        rb_hash_aset(v, sym_literal, rb_str_substr(t->str, startOffset, strLen));
    }
}

static VALUE scanner_tokenize(const VALUE self) {
    scanner_t *t;
    TypedData_Get_Struct(self, scanner_t, &scanner_type, t);
    while (t->look[0] != EOF) {
        scanner_scan_token(t);
    }
    scanner_hydrate_tokens(t);
    return t->tokens;
}

RUBY_FUNC_EXPORTED void Init_minihtml_scanner(void) {
    VALUE rb_mMiniHTML = rb_define_module("MiniHTML");
    VALUE rb_cScanner = rb_define_class_under(rb_mMiniHTML, "Scanner", rb_cObject);

    INITIALIZE_REUSABLE_SYMBOL(line);
    INITIALIZE_REUSABLE_SYMBOL(column);
    INITIALIZE_REUSABLE_SYMBOL(offset);
    INITIALIZE_REUSABLE_SYMBOL(start_line);
    INITIALIZE_REUSABLE_SYMBOL(start_column);
    INITIALIZE_REUSABLE_SYMBOL(start_offset);
    INITIALIZE_REUSABLE_SYMBOL(end_line);
    INITIALIZE_REUSABLE_SYMBOL(end_column);
    INITIALIZE_REUSABLE_SYMBOL(end_offset);
    INITIALIZE_REUSABLE_SYMBOL(new);
    INITIALIZE_REUSABLE_SYMBOL(literal);
    INITIALIZE_REUSABLE_SYMBOL(self_closing);
    INITIALIZE_REUSABLE_SYMBOL(tag_begin);
    INITIALIZE_REUSABLE_SYMBOL(tag_end);
    INITIALIZE_REUSABLE_SYMBOL(tag_closing_start);
    INITIALIZE_REUSABLE_SYMBOL(tag_closing_end);
    INITIALIZE_REUSABLE_SYMBOL(right_angled);
    INITIALIZE_REUSABLE_SYMBOL(attr_key);
    INITIALIZE_REUSABLE_SYMBOL(kind);
    INITIALIZE_REUSABLE_SYMBOL(string);
    INITIALIZE_REUSABLE_SYMBOL(string_interpolation);
    INITIALIZE_REUSABLE_SYMBOL(interpolated_executable);
    INITIALIZE_REUSABLE_SYMBOL(executable);
    INITIALIZE_REUSABLE_SYMBOL(equal);
    INITIALIZE_REUSABLE_SYMBOL(quote_char);
    INITIALIZE_REUSABLE_SYMBOL(tag_comment_end);
    INITIALIZE_REUSABLE_SYMBOL(attr_value_unquoted);

    rb_define_alloc_func(rb_cScanner, scanner_alloc);
    rb_define_method(rb_cScanner, "initialize", scanner_initialize, 1);
    rb_define_method(rb_cScanner, "tokens", scanner_tokens, 0);
    rb_define_method(rb_cScanner, "errors", scanner_errors, 0);
    rb_define_method(rb_cScanner, "stats", scanner_stats, 0);
    rb_define_method(rb_cScanner, "eof?", scanner_at_eof, 0);
    rb_define_method(rb_cScanner, "tokenize", scanner_tokenize, 0);
}
