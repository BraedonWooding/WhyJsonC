/*
MIT License

Copyright (c) 2019 Braedon Wooding

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef WHY_JSON_H
#define WHY_JSON_H

/*
  I wanted a single header easy to use json library
  that was relatively small but efficient enough that I wouldn't
  care about it's performance.

  I couldn't find one that would both give me this and easy recursion
  on children nodes.

  So I made one.

  It's simple, it is just an iterator.  It can use a custom buffer if reading
  from a file / stream or you can just use the string you give in as a buffer
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHY_JSON_MAJOR_V "1"
#define WHY_JSON_MINOR_V "0"
#define WHY_JSON_PATCH_V "a"

#define WHY_JSON_VERSION                                                       \
  WHY_JSON_MAJOR_V "." WHY_JSON_MINOR_V "." WHY_JSON_PATCH_V

#if defined _MSC_VER || defined __MINGW32__ || defined _WIN32
#define WHY_JSON_WINDOWS
#endif

#ifdef WHY_JSON_WINDOWS
#define _WHY_JSON_FUNC_ static __inline
#elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
#define _WHY_JSON_FUNC_ static __inline__
#else
#define _WHY_JSON_FUNC_ static inline
#endif

#ifndef WHY_JSON_BUF_SIZE
#define WHY_JSON_BUF_SIZE (256)
#endif

#ifndef WHY_JSON_INITIAL_MATCH_STACK
#define WHY_JSON_INITIAL_MATCH_STACK (32)
#endif

#define WHY_JSON_UTF8_ACCEPT (1)
#define WHY_JSON_UTF8_REJECT (0)

#if defined __cplusplus
namespace whyjson::internals {
extern "C" {
#endif

typedef struct why_json_it_t WhyJsonIt;
typedef struct why_json_tok_t WhyJsonTok;
typedef union why_json_value_t WhyJsonValue;
typedef struct why_json_str_t WhyJsonStr;

typedef int WhyJsonErr;
enum why_json_err_t {
  WHY_JSON_ERR_NO_ERROR = 0,
  WHY_JSON_ERR_CANT_READ = -1,
  WHY_JSON_ERR_UNKNOWN_TOK = -2,
  WHY_JSON_ERR_NOT_SKIPPABLE = -3,
  WHY_JSON_ERR_INVALID_ARGS = -4,
  WHY_JSON_ERR_UNDEFINED_NEXT_CHAR = -5,
  WHY_JSON_ERR_INVALID_UTF8 = -6,
  WHY_JSON_ERR_UNMATCHED_TOKENS = -7,
  WHY_JSON_ERR_OOM = -8,
  WHY_JSON_ERR_MISSING_COMMA = -9,
  WHY_JSON_ERR_MISSING_QUOTE = -10,
  WHY_JSON_ERR_INVALID_IDENT = -11,
  WHY_JSON_ERR_INVALID_VALUE = -12,
};

typedef uint8_t WhyJsonType;
enum why_json_type_t {
  WHY_JSON_NONE = 0,
  WHY_JSON_STRING = 1,
  WHY_JSON_BOOL = 2,
  WHY_JSON_INT = 3,
  WHY_JSON_FLT = 4,
  WHY_JSON_NULL = 5,
  WHY_JSON_ERROR = 6,
  WHY_JSON_OBJECT = 7,
  WHY_JSON_ARRAY = 8,
  WHY_JSON_OBJECT_END = 9,
  WHY_JSON_ARRAY_END = 10,
  WHY_JSON_END = 11,
};

struct why_json_str_t {
  char *buf;
  size_t len;
  // was this allocated you can toggle this off if you want
  // to control the allocation from now on
  int allocated;
};

union why_json_value_t {
  long _int;
  double _flt;
  WhyJsonStr _str;
  int _bool;
};

struct why_json_tok_t {
  WhyJsonStr key;
  WhyJsonType type;
  WhyJsonValue value;
  int first; // is this the first token for this array/object
};

struct why_json_it_t {
  FILE *stream;
  // Either you'll use a stream or a source string
  // The other will be NULL
  const char *source_str;
  size_t source_str_len;
  size_t source_str_cur;

  char *err;
  uint32_t invalid_char;

  uint8_t *match_stack;
  size_t match_len;

  size_t cur_loc;
  int cur_line;
  int cur_col;
  int depth;
  uint32_t state;

  // - Try to keep the buffer size greater than
  //   the key size + element size else we'll have to malloc them
  //   and it'll oncur a performance hit
  char buf[WHY_JSON_BUF_SIZE + 1];
  size_t buf_len;
};

// Taken UTF8 validation from online since building it myself seemed annoying
// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

// clang format doesn't like this
// clang-format off
static const uint8_t utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};
// clang-format on

_WHY_JSON_FUNC_ uint32_t why_json_is_legal_utf8(uint32_t *state,
                                                const char *bytes,
                                                size_t length) {
  uint32_t type;
  if (bytes == NULL || state == NULL) {
    return 0;
  }

  size_t i;
  for (i = 0; i < length; i++) {
    type = utf8d[(uint8_t)bytes[i]];
    *state = utf8d[256 + (*state) * 16 + type];

    if (*state == 0) {
      break;
    }
  }

  return *state;
}

#define WHY_JSON_GET_COUNT(byte) ((byte) & ~0x80)
#define WHY_JSON_CAN_ADD(byte) (WHY_JSON_GET_COUNT(byte) < (UINT8_MAX & ~0x80))

_WHY_JSON_FUNC_ int why_json_read(WhyJsonIt *it) {
  if (it->cur_loc != it->buf_len) {
    errno = WHY_JSON_ERR_CANT_READ;
    return 0;
  }

  if (it->state == WHY_JSON_UTF8_REJECT) {
    errno = WHY_JSON_ERR_INVALID_UTF8;
    return 0;
  }

  it->cur_loc = 0;

  if (it->stream != NULL) {
    it->buf_len = fread(it->buf, 1, WHY_JSON_BUF_SIZE, it->stream);
    if (it->buf_len == 0) {
      if (feof(it->stream)) {
        // EOF isn't an error
        it->buf_len = 0;
        it->buf[0] = '\0';
        return 1;
      } else if (ferror(it->stream)) {
        errno = WHY_JSON_ERR_CANT_READ;
        return 0;
      } else {
        // unknown error
        errno = WHY_JSON_ERR_UNDEFINED_NEXT_CHAR;
        return 0;
      }
    } else {
      it->buf[it->buf_len] = '\0';
    }
  } else if (it->source_str != NULL) {
    size_t remaining = it->source_str_len - it->source_str_cur;
    if (remaining == 0) {
      // EOF
      it->buf_len = 0;
      it->buf[0] = '\0';
      return 1;
    } else {
      if (remaining > WHY_JSON_BUF_SIZE) {
        remaining = WHY_JSON_BUF_SIZE;
      }
      memcpy(it->buf, it->source_str + it->source_str_cur, remaining);
      it->buf[remaining] = '\0';
      it->buf_len = remaining;
      it->source_str_cur += remaining;
    }
  } else {
    errno = WHY_JSON_ERR_INVALID_ARGS;
    return 0;
  }

  // verify that the json is legal
  it->state = why_json_is_legal_utf8(&it->state, it->buf, it->buf_len);
  if (it->state == WHY_JSON_UTF8_REJECT) {
    it->err = "Invalid UTF8 JSON";
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ int why_json_peek_char(WhyJsonIt *it) {
  if (it->cur_loc == it->buf_len && (!why_json_read(it) || it->buf_len == 0)) {
    return EOF;
  } else {
    return it->buf[it->cur_loc];
  }
}

/*
 Initialises a json iterator from a file
 */
_WHY_JSON_FUNC_ int why_json_file(WhyJsonIt *it, FILE *file) {
  if (file == NULL) {
    errno = WHY_JSON_ERR_INVALID_ARGS;
    return 0;
  }

  it->stream = file;
  it->source_str = NULL;
  it->source_str_len = 0;
  it->source_str_cur = 0;
  it->cur_line = it->cur_col = 1;
  it->state = WHY_JSON_UTF8_ACCEPT;
  it->buf_len = 0;
  it->buf[0] = '\0';
  it->depth = 0;
  it->cur_loc = 0;
  it->err = NULL;
  it->match_stack =
      (uint8_t *)malloc(sizeof(uint8_t) * WHY_JSON_INITIAL_MATCH_STACK);
  it->match_len = 0;
  if (it->match_stack == NULL) {
    errno = WHY_JSON_ERR_OOM;
    return 0;
  }

  memset(it->match_stack, 0, sizeof(uint8_t) * WHY_JSON_INITIAL_MATCH_STACK);
  // our 'null terminating' char
  it->match_stack[WHY_JSON_INITIAL_MATCH_STACK - 1] = UINT8_MAX;

  return 1;
}

/*
 Initialises a json iterator from a string
 */
_WHY_JSON_FUNC_ int why_json_str(WhyJsonIt *it, const char *str) {
  if (str == NULL) {
    errno = WHY_JSON_ERR_INVALID_ARGS;
    return 0;
  }

  it->stream = NULL;
  it->source_str = str;
  it->source_str_len = strlen(str);
  it->source_str_cur = 0;
  it->cur_line = it->cur_col = 1;
  it->state = WHY_JSON_UTF8_ACCEPT;
  it->buf_len = 0;
  it->buf[0] = '\0';
  it->depth = 0;
  it->cur_loc = 0;
  it->err = NULL;
  it->match_stack =
      (uint8_t *)malloc(sizeof(uint8_t) * WHY_JSON_INITIAL_MATCH_STACK);
  it->match_len = 0;
  if (it->match_stack == NULL) {
    errno = WHY_JSON_ERR_OOM;
    return 0;
  }

  memset(it->match_stack, 0, sizeof(uint8_t) * WHY_JSON_INITIAL_MATCH_STACK);
  // our 'null terminating' char
  it->match_stack[WHY_JSON_INITIAL_MATCH_STACK - 1] = UINT8_MAX;

  return 1;
}

_WHY_JSON_FUNC_ int why_json_next_char(WhyJsonIt *it) {
  int next = why_json_peek_char(it);
  it->cur_loc++;
  return next;
}

// puts current char into buf
_WHY_JSON_FUNC_ int why_json_into_buf(char **tmp, size_t *tmp_len,
                                      size_t *tmp_cap, WhyJsonIt *it, int c) {
  if (*tmp == NULL) {
    *tmp = (char *)malloc(sizeof(char) * 9); // TODO make #define
    if (*tmp == NULL) {
      errno = WHY_JSON_ERR_OOM;
      it->err = "Ran out of memory";
      return 0;
    }
    memset(*tmp, 0, sizeof(char) * 9);
    *tmp_cap = 8;
    *tmp_len = 1;
    (*tmp)[0] = c;
  } else if (*tmp_len < *tmp_cap) {
    (*tmp)[(*tmp_len)++] = c;
  } else {
    if (*tmp_cap < 4) {
      *tmp_cap = 4;
    }
    char *new = (char *)realloc(*tmp, sizeof(char) * (*tmp_cap * 2 + 1));
    if (new == NULL) {
      errno = WHY_JSON_ERR_OOM;
      it->err = "Ran out of memory";
      return 0;
    }
    memset(new + *tmp_len, 0, sizeof(char) * (*tmp_cap * 2 + 1 - *tmp_len));
    *tmp = new;
    *tmp_cap *= 2;
    (*tmp)[(*tmp_len)++] = c;
  }

  return 1;
}

_WHY_JSON_FUNC_ int why_json_is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

_WHY_JSON_FUNC_ int why_json_ignore_whitespace(WhyJsonIt *it) {
  while (1) {
    if (it->cur_loc == it->buf_len && !why_json_read(it)) {
      return 0;
    }
    if (it->buf_len == 0) {
      // EOF means no more whitespace
      break;
    }

    char c = it->buf[it->cur_loc];
    if (why_json_is_whitespace(c)) {
      it->cur_loc++;
    } else {
      break;
    }
  }
  return 1;
}

_WHY_JSON_FUNC_ int why_json_resize_match_stack(WhyJsonIt *it) {
  // if the next one is uint8_max then we have found end
  // of our buffer and we have to reallocate
  if (it->match_stack[it->match_len] == UINT8_MAX) {
    uint8_t *tmp =
        realloc(it->match_stack, sizeof(uint8_t) * it->match_len * 2);
    if (tmp == NULL) {
      // @TODO: This is pretty meh
      //        idk if I like this
      free(it->match_stack);
      it->match_stack = NULL;
      errno = WHY_JSON_ERR_OOM;
      return 0;
    }
    it->match_stack = tmp;
    memset(it->match_stack + it->match_len, 0, it->match_len);
    it->match_stack[it->match_len * 2 - 1] = UINT8_MAX;
  }
  return 1;
}

_WHY_JSON_FUNC_ int why_json_count_braces(WhyJsonTok *tok, WhyJsonIt *it) {
  if (!why_json_ignore_whitespace(it)) {
    tok->type = WHY_JSON_ERROR;
    return 0;
  }

  if (it->buf_len == 0) {
    // EOF
    if (it->match_len == 0) {
      // We have no matches and are at EOF this is fine
      // We just write the no error
      tok->type = WHY_JSON_END;
      errno = WHY_JSON_ERR_NO_ERROR;
      return 0;
    } else {
      // we still have matches
      // TODO: Check the last match type to get better error
      it->err = "Unmatched { or [";
      errno = WHY_JSON_ERR_UNMATCHED_TOKENS;
      tok->type = WHY_JSON_ERROR;
      return 0;
    }
  }

  if (it->buf[it->cur_loc] == '}') {
    it->depth--;
    if (it->match_len == 0 ||
        (it->match_stack[it->match_len - 1] & 0x80) != 0x80) {
      errno = WHY_JSON_ERR_UNMATCHED_TOKENS;
      it->invalid_char = '}';
      it->err = "Unmatched {";
      tok->type = WHY_JSON_ERROR;
      return 0;
    }

    if (WHY_JSON_GET_COUNT(--it->match_stack[it->match_len - 1]) == 0) {
      it->match_stack[--it->match_len] = 0;
    }
    it->cur_loc++;

    if (!why_json_ignore_whitespace(it)) {
      tok->type = WHY_JSON_ERROR;
      return 0;
    }

    tok->type = WHY_JSON_OBJECT_END;
    errno = WHY_JSON_ERR_NO_ERROR;
    return 0;
  } else if (it->buf[it->cur_loc] == ']') {
    it->depth--;
    if (it->match_len == 0 ||
        (it->match_stack[it->match_len - 1] & 0x80) != 0) {
      errno = WHY_JSON_ERR_UNMATCHED_TOKENS;
      it->invalid_char = ']';
      it->err = "Unmatched [";
      tok->type = WHY_JSON_ERROR;
      return 0;
    }

    if (WHY_JSON_GET_COUNT(--it->match_stack[it->match_len - 1]) == 0) {
      it->match_stack[--it->match_len] = 0;
    }
    it->cur_loc++;

    if (!why_json_ignore_whitespace(it)) {
      tok->type = WHY_JSON_ERROR;
      return 0;
    }

    tok->type = WHY_JSON_ARRAY_END;
    errno = WHY_JSON_ERR_NO_ERROR;
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ void why_json_free_str(WhyJsonStr *str) {
  if (str->allocated) {
    free(str->buf);
  }
  str->buf = NULL;
  str->len = 0;
  str->allocated = 0;
}

_WHY_JSON_FUNC_ int why_json_char_needs_escaping(int c) {
  return ((c >= 0) && (c < 0x20 || c == 0x22 || c == 0x5c));
}

_WHY_JSON_FUNC_ int why_json_hex(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10;
  return -1;
}

_WHY_JSON_FUNC_ int why_json_parse_codepoint(WhyJsonIt *it, uint32_t *codepoint,
                                             int len) {
  *codepoint = 0;
  for (int i = 0; i < len; i++) {
    int c = why_json_next_char(it);
    int hex = why_json_hex(c);
    if (c == EOF || hex == -1) {
      errno = WHY_JSON_ERR_INVALID_UTF8;
      it->err = "Invalid Utf8 Character";
      it->invalid_char = c;
      return 0;
    }
    *codepoint = (*codepoint << 4) | (hex & 0xF);
  }
  return 1;
}

_WHY_JSON_FUNC_ int why_json_to_utf8(char **tmp, size_t *tmp_len,
                                     size_t *tmp_cap, WhyJsonIt *it,
                                     uint32_t cp) {
  if (cp <= 0x7Ful) {
    return why_json_into_buf(tmp, tmp_len, tmp_cap, it, cp);
  } else if (cp <= 0x7FFul) {
    int first = (cp >> 6 & 0x1F) | 0xC0;
    int second = (cp & 0x3F) | 0x80;
    return why_json_into_buf(tmp, tmp_len, tmp_cap, it, first) &&
           why_json_into_buf(tmp, tmp_len, tmp_cap, it, second);
  } else if (cp <= 0xFFFF) {
    int first = (cp >> 12 & 0x0F) | 0xE0;
    int second = (cp >> 6 & 0x3F) | 0x80;
    int third = (cp & 0x3F) | 0x80;
    return why_json_into_buf(tmp, tmp_len, tmp_cap, it, first) &&
           why_json_into_buf(tmp, tmp_len, tmp_cap, it, second) &&
           why_json_into_buf(tmp, tmp_len, tmp_cap, it, third);
  } else if (cp <= 0x10FFFF) {
    int first = (cp >> 18 & 0x07) | 0xF0;
    int second = (cp >> 12 & 0x3F) | 0x80;
    int third = (cp >> 6 & 0x3F) | 0x80;
    int fourth = (cp & 0x3F) | 0x80;
    return why_json_into_buf(tmp, tmp_len, tmp_cap, it, first) &&
           why_json_into_buf(tmp, tmp_len, tmp_cap, it, second) &&
           why_json_into_buf(tmp, tmp_len, tmp_cap, it, third) &&
           why_json_into_buf(tmp, tmp_len, tmp_cap, it, fourth);
  } else {
    errno = WHY_JSON_ERR_INVALID_UTF8;
    it->err = "Invlaid Utf8 Character";
    it->invalid_char = cp;
    return 0;
  }
}

_WHY_JSON_FUNC_ int why_json_parse_str_till(WhyJsonStr *out, WhyJsonIt *it,
                                            char ending) {
  char *tmp = malloc(sizeof(char));
  size_t tmp_len = 0;
  size_t tmp_cap = 0;
  tmp[0] = '\0';
  int next = 0;

  while (1) {
    next = why_json_next_char(it);
    if (next == EOF) {
      break;
    }

    if (next == ending) {
      // success!
      break;
    } else if (next == '\\') {
      int c = why_json_next_char(it);
      if (c == 'u') {
        uint32_t cp = 0;
        if (!why_json_parse_codepoint(it, &cp, 4)) {
          return 0;
        }

        if (cp > UINT16_MAX) {
          fprintf(stderr, __FILE__ ":%d This shouldn't happen!!\n", __LINE__);
          return 0;
        }

        uint32_t low = 0;
        uint32_t high = 0;
        if (cp >= 0xD800 && cp <= 0xDBFF) {
          // we have the high point
          // now need low
          high = cp;
          cp = 0;
          c = why_json_next_char(it);
          int next = why_json_next_char(it);
          if (c == EOF || c != '\\' || next == EOF || next != 'u') {
            errno = WHY_JSON_ERR_INVALID_UTF8;
            it->err = "Invalid Utf8 Character";
            if (c != EOF && c == '\\') {
              it->invalid_char = next;
            }
            return 0;
          }

          if (!why_json_parse_codepoint(it, &cp, 4)) {
            return 0;
          }

          if (cp > UINT16_MAX) {
            fprintf(stderr, __FILE__ ":%d This shouldn't happen!!\n", __LINE__);
            return 0;
          }

          low = cp;
          if (low < 0xDC00 || low > 0xDFFF) {
            errno = WHY_JSON_ERR_INVALID_UTF8;
            it->err = "Invalid Utf8 Character";
            it->invalid_char = cp;
            return 0;
          }
          cp = ((high - 0xD800) * 0x400) + (low - 0xDC00) + 0x10000;
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
          errno = WHY_JSON_ERR_INVALID_UTF8;
          it->err = "Invalid Utf8 Character";
          it->invalid_char = cp;
          return 0;
        }
        if (!why_json_to_utf8(&tmp, &tmp_len, &tmp_cap, it, cp)) {
          return 0;
        }
      } else if (c == 'U') {
        uint32_t cp = 0;
        if (!why_json_parse_codepoint(it, &cp, 8)) {
          return 0;
        }
        if (!why_json_to_utf8(&tmp, &tmp_len, &tmp_cap, it, cp)) {
          return 0;
        }
      } else {
        int to_write = 0;
        if (c == '\\') {
          to_write = '\\';
        } else if (c == 'b') {
          to_write = '\b';
        } else if (c == 'f') {
          to_write = '\f';
        } else if (c == 't') {
          to_write = '\t';
        } else if (c == 'n') {
          to_write = '\n';
        } else if (c == 'r') {
          to_write = '\r';
        } else if (c == '/') {
          to_write = '/';
        } else if (c == '"') {
          to_write = '"';
        } else {
          it->err = "Invalid Escaping Character";
          errno = WHY_JSON_ERR_UNKNOWN_TOK;
          return 0;
        }
        if (!why_json_into_buf(&tmp, &tmp_len, &tmp_cap, it, to_write)) {
          return 0;
        }
      }
    } else if (why_json_char_needs_escaping(next)) {
      break;
    } else {
      if (!why_json_into_buf(&tmp, &tmp_len, &tmp_cap, it, next)) {
        return 0;
      }
    }
  }

  if (it->buf_len == 0 || next != ending) {
    // error!
    errno = WHY_JSON_ERR_MISSING_QUOTE;
    it->err = "Missing ending character";
    it->invalid_char = ending;
    return 0;
  }

  out->buf = tmp;
  out->len = tmp_len;
  out->allocated = 1;

  return 1;
}

_WHY_JSON_FUNC_ int why_json_parse_identifier(WhyJsonStr *out, WhyJsonIt *it) {
  // We support identifiers that are just numbers
  // but also identifiers that are a sequence of any bytes before a `:`
  // we don't support multi-line identifiers
  // We also cut all trailing spaces
  if (!why_json_parse_str_till(out, it, ':')) {
    return 0;
  }

  // note we have to then backtrack one so that our cur_loc is on ':'
  it->cur_loc--;
  while (out->len > 0 && why_json_is_whitespace(out->buf[out->len - 1])) {
    out->len--;
  }

  // note we want to then detect if they just entered whitespace
  // this shouldn't occur since we remove whitespace prior to the key
  // but meh
  if (out->len == 0) {
    errno = WHY_JSON_ERR_INVALID_IDENT;
    it->err = "Invalid indentifier, identifier is empty";
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ int why_json_parse_key(WhyJsonTok *tok, WhyJsonIt *it) {
  if (!why_json_ignore_whitespace(it)) {
    return 0;
  }

  if (it->buf[it->cur_loc] == '"') {
    // this is a valid key
    it->cur_loc++;
    if (!why_json_parse_str_till(&tok->key, it, '"')) {
      return 0;
    }
  } else {
#ifndef WHY_JSON_STRICT
    if (!why_json_parse_identifier(&tok->key, it)) {
      return 0;
    }
#else
    errno = WHY_JSON_ERR_MISSING_QUOTE;
    it->err = "Missing initial \"";
    return 0;
#endif
  }

  return 1;
}

// 1 if it matches, 0 if it doesn't, -1 if error
_WHY_JSON_FUNC_ int why_json_matches(const char *str, WhyJsonIt *it) {
  while (*str != '\0') {
    if (it->cur_loc == it->buf_len && !why_json_read(it)) {
      return -1;
    }

    if (*str == it->buf[it->cur_loc]) {
      str++;
      it->cur_loc++;
    } else {
      return 0;
    }
  }

  return 1;
}

_WHY_JSON_FUNC_ int why_json_parse_num(WhyJsonType *type, WhyJsonValue *value,
                                       WhyJsonIt *it) {
  int sign = 1;
  if (why_json_peek_char(it) == '-') {
    sign = -1;
    it->cur_loc++;
  }

  *type = WHY_JSON_INT;
  char *tmp = NULL;
  size_t tmp_len = 0;
  size_t tmp_cap = 0;
  int seen_dot = 0;
  int seen_exp = 0;
  int prev_exp = 0;
  int prev_underscore = 0;

  while (1) {
    int next = why_json_next_char(it);
    if (next == EOF && tmp_len > 0) {
      break;
    } else if (next == EOF) {
      it->err = "Unexpected EOF";
      errno = WHY_JSON_ERR_INVALID_VALUE;
      return 0;
    } else if (next >= '0' && next <= '9') {
      // this is fine
      why_json_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
      prev_exp = prev_underscore = 0;
    } else if (next == '.' && !seen_dot && !seen_exp) {
      // still fine if only one
      seen_dot = 1;
      why_json_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
      prev_underscore = 0;
      *type = WHY_JSON_FLT;
    } else if ((next == 'e' || next == 'E') && !seen_exp) {
      seen_exp = 1;
      prev_exp = 1;
      why_json_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
      prev_underscore = 0;
      *type = WHY_JSON_FLT;
    } else if (next == '_' && !prev_underscore) {
      // ignore underscores
      prev_underscore = 1;
    } else if ((next == '+' || next == '-') && prev_exp) {
      prev_exp = prev_underscore = 0;
      why_json_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
    } else if (why_json_is_whitespace(next) || next == '}' || next == ',' ||
               next == ']') {
      it->cur_loc--;
      break;
    } else {
      // error
      it->err = "Invalid numeric character";
      it->invalid_char = next;
      errno = WHY_JSON_ERR_INVALID_VALUE;
      return 0;
    }
  }

  if (*type == WHY_JSON_INT) {
    value->_int = sign * strtol(tmp, NULL, 10);
  } else {
    value->_flt = sign * strtod(tmp, NULL);
  }

  return 1;
}

_WHY_JSON_FUNC_ int why_json_parse_value(WhyJsonType *type, WhyJsonValue *value,
                                         WhyJsonIt *it) {
  if (!why_json_ignore_whitespace(it)) {
    return 0;
  }

  int next = why_json_peek_char(it);
  if (next == EOF) {
    errno = WHY_JSON_ERR_UNKNOWN_TOK;
    it->err = "Unexpected EOF was expecting a value";
    it->invalid_char = EOF;
    return 0;
  }

  if (next == '"') {
    // parse str
    *type = WHY_JSON_STRING;
    it->cur_loc++;
    return why_json_parse_str_till(&value->_str, it, '"');
  } else if (next == 't') {
    // true
    int res = why_json_matches("true", it);
    if (res == -1) {
      return 0;
    } else if (res == 1) {
      *type = WHY_JSON_BOOL;
      value->_bool = 1;
      return 1;
    }
  } else if (next == 'f') {
    // false
    int res = why_json_matches("false", it);
    if (res == -1) {
      return 0;
    } else if (res == 1) {
      *type = WHY_JSON_BOOL;
      value->_bool = 0;
      return 1;
    }
  } else if (next == 'n') {
    // null
    int res = why_json_matches("null", it);
    if (res == -1) {
      return 0;
    } else if (res == 1) {
      *type = WHY_JSON_NULL;
      return 1;
    }
  } else if (next == '{') {
    // object
    // we don't want to move forward because our next
    // need to know this
    *type = WHY_JSON_OBJECT;
    return 1;
  } else if (next == '[') {
    // array
    *type = WHY_JSON_ARRAY;
    return 1;
  } else if (why_json_parse_num(type, value, it)) {
    // number/int
    // TODO: Do we want someway to see if the parse num failed?
    //       idk maybe?
    return 1;
  }

  // unknown type
  it->err = "Unknown value";
  errno = WHY_JSON_ERR_INVALID_VALUE;
  return 0;
}

/*
 Goes to the next element in the json.  If the current element is at an object
 or array it will stop at the key allowing you to skip it else if you call
 next it'll step into it.
 */
_WHY_JSON_FUNC_ int why_json_next(WhyJsonTok *tok, WhyJsonIt *it) {
  if (it->buf_len == 0 && it->cur_loc == 0 &&
      ((it->source_str_cur == 0 && it->source_str_len > 0 &&
        it->source_str != NULL) ||
       (it->stream != NULL && !feof(it->stream)))) {
    memset(tok, 0, sizeof(WhyJsonTok));
    tok->first = 1;

    // we are at the start
    // this just means we have a single value
    if (why_json_parse_value(&tok->type, &tok->value, it)) {
      if (!why_json_ignore_whitespace(it)) {
        return 0;
      }
      if (tok->type != WHY_JSON_ARRAY && tok->type != WHY_JSON_OBJECT &&
          why_json_peek_char(it) != EOF) {
        // if we are not at EOF then clearly something is wrong
        // i.e. "a": 2, "b": 3 is not valid
        errno = WHY_JSON_ERR_UNKNOWN_TOK;
        it->err = "Was expecting EOF since single value was read.";
        if (tok->type == WHY_JSON_STRING) {
          why_json_free_str(&tok->value._str);
        }
        tok->type = WHY_JSON_ERROR;
        return 0;
      }
      return 1;
    }
    tok->type = WHY_JSON_ERROR;
    return 0;
  }

  if (!why_json_ignore_whitespace(it)) {
    tok->type = WHY_JSON_ERROR;
    return 0;
  }

  tok->first = 0;

  // was the previous a key for object/array
  int prev_was_key =
      tok->type == WHY_JSON_ARRAY || tok->type == WHY_JSON_OBJECT;
  // Deallocate the strings
  why_json_free_str(&tok->key);

  if (tok->type == WHY_JSON_STRING) {
    why_json_free_str(&tok->value._str);
  }

  if (prev_was_key) {
    switch (it->buf[it->cur_loc]) {
    case '{': {
      it->cur_loc++;
      it->depth++;
      if (!why_json_ignore_whitespace(it)) {
        tok->type = WHY_JSON_ERROR;
        return 0;
      }
      if (it->match_len > 0 &&
          (it->match_stack[it->match_len - 1] & 0x80) == 0x80 &&
          WHY_JSON_CAN_ADD(it->match_stack[it->match_len - 1])) {
        it->match_stack[it->match_len - 1]++;
      } else {
        if (!why_json_resize_match_stack(it)) {
          tok->type = WHY_JSON_ERROR;
          return 0;
        }
        it->match_stack[it->match_len++] = 1 | 0x80;
      }

      tok->first = 1;

      if (it->buf_len == it->cur_loc && !why_json_ignore_whitespace(it)) {
        tok->type = WHY_JSON_ERROR;
        return 0;
      }
      // now we just have to parse the code as usual
      // looking out for our ending '}'
    } break;
    case '[': {
      it->cur_loc++;
      it->depth++;
      if (!why_json_ignore_whitespace(it)) {
        tok->type = WHY_JSON_ERROR;
        return 0;
      }

      if (it->match_len > 0 &&
          (it->match_stack[it->match_len - 1] & 0x80) == 0 &&
          WHY_JSON_CAN_ADD(it->match_stack[it->match_len - 1])) {
        it->match_stack[it->match_len - 1]++;
      } else {
        if (!why_json_resize_match_stack(it)) {
          tok->type = WHY_JSON_ERROR;
          return 0;
        }
        // and technically `|` with 0
        it->match_stack[it->match_len++] = 1;
      }

      tok->first = 1;

      if (it->buf_len == it->cur_loc && !why_json_ignore_whitespace(it)) {
        tok->type = WHY_JSON_ERROR;
        return 0;
      }
      // now we just have to parse the code as usual
      // looking out for our ending ']'
    } break;
    default: {
      // @TODO: Get better error message about which token
      errno = WHY_JSON_ERR_UNKNOWN_TOK;
      it->err = "Invalid Character";
      // NOTE: We should fix this and get the next codepoint
      //       Not just u8 char
      it->invalid_char = it->buf[it->cur_loc];
      tok->type = WHY_JSON_ERROR;
      return 0;
    }
    }
  }

  int comma_done = 0;
#ifndef WHY_JSON_STRICT
  if (why_json_peek_char(it) == ',') {
    comma_done = 1;
    it->cur_loc++;
  }
#endif

  if (!why_json_count_braces(tok, it)) {
    // i.e. [a, [ ]] can trick it into thinking that
    // the ending brace `]` is not first but really
    // it makes no sense them having first/not first
    // more cases want them to be first (i.e. pretty print)
    // then not and you can always override it by checking
    // the type.
    tok->first = 1;
    return errno == WHY_JSON_ERR_NO_ERROR;
  }

  if (!why_json_ignore_whitespace(it)) {
    tok->type = WHY_JSON_ERROR;
    return 0;
  }

  if ((it->cur_loc >= it->source_str_len && it->source_str != NULL) ||
      (it->stream != NULL && feof(it->stream))) {
    tok->type = WHY_JSON_END;
    return 1;
  }

  if (!prev_was_key) {
    // then we expect a comma!
    if (!comma_done && why_json_next_char(it) != ',') {
      it->err = "Missing comma";
      errno = WHY_JSON_ERR_MISSING_COMMA;
      tok->type = WHY_JSON_ERROR;
      return 0;
    }
  }

  if (!why_json_ignore_whitespace(it)) {
    tok->type = WHY_JSON_ERROR;
    return 0;
  }

  // parse key
  if (it->match_len == 0 ||
      (it->match_stack[it->match_len - 1] & 0x80) == 0x80) {
    if (!why_json_parse_key(tok, it)) {
      tok->type = WHY_JSON_ERROR;
      return 0;
    }

    if (!why_json_ignore_whitespace(it)) {
      why_json_free_str(&tok->key);
      tok->type = WHY_JSON_ERROR;
      return 0;
    }

    int next = why_json_next_char(it);
    if (next == EOF || next != ':') {
      // EOF
      why_json_free_str(&tok->key);
      errno = WHY_JSON_ERR_UNKNOWN_TOK;
      it->err = "Didn't expect char was expecting ':'";
      it->invalid_char = it->buf_len == 0 ? EOF : it->buf[it->cur_loc];
      tok->type = WHY_JSON_ERROR;
      return 0;
    }

    if (!why_json_ignore_whitespace(it)) {
      why_json_free_str(&tok->key);
      tok->type = WHY_JSON_ERROR;
      return 0;
    }
  }

  // then parse value
  if (!why_json_parse_value(&tok->type, &tok->value, it)) {
    why_json_free_str(&tok->key);
    tok->type = WHY_JSON_ERROR;
    return 0;
  }

  return 1;
}

/*
 Skips the json object useful for when you just want to visit the outer
 objects or don't want to visit an object for whatever reason.

 Errors if the current key is not an object or array.
 */
_WHY_JSON_FUNC_ int why_json_skip(WhyJsonTok *tok, WhyJsonIt *it) {
  uint8_t wait;
  if (tok->type == WHY_JSON_ARRAY) {
    wait = WHY_JSON_ARRAY_END;
  } else if (tok->type == WHY_JSON_OBJECT) {
    wait = WHY_JSON_OBJECT_END;
  } else {
    errno = WHY_JSON_ERR_INVALID_ARGS;
    it->err = "Type of token isn't array or object";
    return 0;
  }

  errno = 0;
  while (why_json_next(tok, it) && tok->type != wait) {
  }
  return errno == 0;
}

#undef WHY_JSON_GET_COUNT
#undef WHY_JSON_CAN_ADD

#if defined __cplusplus
}
}

class Json {};
#endif

#endif
