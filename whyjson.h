/*
Mit License

Copyright (C) 2019 Braedon WoodinG

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
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHY_JSON_MAJOR_V "1"
#define WHY_JSON_MINOR_V "0"
#define WHY_JSON_PATCH_V "a"

#define WHY_JSON_VERSION                                                       \
  (WHY_JSON_MAJOR_V "." WHY_JSON_MINOR_V "." WHY_JSON_PATCH_V)

#if defined WHY_JSON_NO_DEFINITIONS
#define WHY_JSON_STATIC
#else
#define WHY_JSON_STATIC static
#endif

#if defined _MSC_VER || defined __MINGW32__ || defined _WIN32
#define _WHY_JSON_FUNC_ WHY_JSON_STATIC __inline
#elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
#define _WHY_JSON_FUNC_ WHY_JSON_STATIC __inline__
#else
#define _WHY_JSON_FUNC_ WHY_JSON_STATIC inline
#endif

#ifndef WHY_JSON_BUF_SIZE
#define WHY_JSON_BUF_SIZE (256)
#endif

#ifndef WHY_JSON_INITIAL_TMP_BUF_SIZE
#define WHY_JSON_INITIAL_TMP_BUF_SIZE (16)
#endif

#ifndef WHY_JSON_ERR_BUF_SIZE
#define WHY_JSON_ERR_BUF_SIZE (256)
#endif

#ifndef WHY_JSON_INITIAL_MATCH_STACK
#define WHY_JSON_INITIAL_MATCH_STACK (32)
#endif

#define WHY_JSON_UTF8_ACCEPT (1)
#define WHY_JSON_UTF8_REJECT (0)

#if defined __cplusplus
extern "C" {
#endif

typedef struct json_it_t JsonIt;
typedef struct json_tok_t JsonTok;
typedef union json_value_t JsonValue;
typedef struct json_str_t JsonStr;

typedef int JsonErr;
enum json_err_t {
  JSON_ERR_NO_ERROR = 0,
  JSON_ERR_CANT_READ = -1,
  JSON_ERR_UNKNOWN_TOK = -2,
  JSON_ERR_NOT_SKIPPABLE = -3,
  JSON_ERR_INVALID_ARGS = -4,
  JSON_ERR_UNDEFINED_NEXT_CHAR = -5,
  JSON_ERR_INVALID_UTF8 = -6,
  JSON_ERR_UNMATCHED_TOKENS = -7,
  JSON_ERR_OOM = -8,
  JSON_ERR_MISSING_COMMA = -9,
  JSON_ERR_MISSING_QUOTE = -10,
  JSON_ERR_INVALID_IDENT = -11,
  JSON_ERR_INVALID_VALUE = -12,
};

typedef uint8_t JsonType;
enum json_type_t {
  JSON_ERROR = 0,
  JSON_STRING = 1,
  JSON_BOOL = 2,
  JSON_INT = 3,
  JSON_FLT = 4,
  JSON_NULL = 5,
  JSON_OBJECT = 7,
  JSON_ARRAY = 8,
  JSON_OBJECT_END = 9,
  JSON_ARRAY_END = 10,
  JSON_END = 11,
};

struct json_str_t {
  const char *buf;
  size_t len;
  // was this allocated you can toggle this off if you want
  // to control the allocation from now on
  int allocated;
};

union json_value_t {
  long _int;
  double _flt;
  JsonStr _str;
  int _bool;
};

struct json_tok_t {
  JsonStr key;
  JsonType type;
  JsonValue value;
  int first; // is this the first token for this array/object
};

struct json_it_t {
  FILE *stream;
  // Either you'll use a stream or a source string
  // The other will be NULL
  const char *source_str;
  size_t source_str_len;
  size_t source_str_cur;
  int is_buf;

  char err[WHY_JSON_ERR_BUF_SIZE];

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

/*
 Use a file as a source for the json iterator.
 Note: Won't seek to the start so make sure file is set to the location
       You want reading to start at.
 */
_WHY_JSON_FUNC_ int json_file(JsonIt *it, FILE *file);

/*
 Initialises a json iterator from a constant string
 You can use literals in this, it won't attempt to edit it.
 */
_WHY_JSON_FUNC_ int json_str(JsonIt *it, const char *str);

/*
  Goes to the next element in the json.  If the current element is at an object
  or array it will stop at the key allowing you to skip it else if you call
  next it'll step into it.
*/
_WHY_JSON_FUNC_ int json_next(JsonTok *tok, JsonIt *it);

/*
  Skips the json object useful for when you just want to visit the outer
  objects or don't want to visit an object for whatever reason.

  Errors if the current key is not an object or array.
*/
_WHY_JSON_FUNC_ int json_skip(JsonTok *tok, JsonIt *it);

/*
 Destroys the given token and iterator data.
 Won't free the token itself nor the iterator since presumably
 you have stack allocated them (or should).

 They both can be NULL in the case you don't know if they are null or
 you want to just free one.
 */
_WHY_JSON_FUNC_ void json_destroy(JsonTok *tok, JsonIt *it);

/*
 If you want a writeable version you can use this
 It will avoid allocations by just invalidating the string after this call

 Our strings are by default readonly because it is often more efficient
 and possibly allows us to re-use buffers in the future!
 Less Allocations are the better.
 */
_WHY_JSON_FUNC_ char *json_get_str(JsonStr *str, size_t *len);

#ifndef WHY_JSON_NO_DEFINITIONS

/*
  == Helper Functions ==
  These are helper functions and should not be called by your code
 */

/*
 Report an error.  Given a format and some var args.
 */
_WHY_JSON_FUNC_ int json_internal_error(JsonIt *it, int err, const char *fmt,
                                        ...);

/*
  Returns JSON_ACCEPT if the json is legal else JSON_REJECT
  if neither than it needs more characters to determine.
 */
_WHY_JSON_FUNC_ uint32_t json_internal_is_legal_utf8(uint32_t *state,
                                                     const char *bytes,
                                                     size_t length,
                                                     size_t *stop);

/*
  Reads into buffer.
  Make sure that it->cur_loc == it->buf_len prior to calling this
  Else you'll lose the rest of the buffer.
 */
_WHY_JSON_FUNC_ int json_internal_read(JsonIt *it);

/*
  Peeks the next character
 */
_WHY_JSON_FUNC_ int json_internal_peek_char(JsonIt *it);

/*
  Gets the next character moving forward cur_loc and cur_col/line
  Avoid just doing cur_loc++ and prefer using this to make sure
  our line numbers are right.
*/
_WHY_JSON_FUNC_ int json_internal_next_char(JsonIt *it);

/*
 Initialise the iterator.
 Prefer to call json_string/json_file rather than initialising it yourself
 */
_WHY_JSON_FUNC_ int json_internal_init(JsonIt *it);

/*
 Writes the character given into the temporary buffer reallocating as needed
 Uses a typical reallocation as min 4 (start 8) and doubling each time.
 */
_WHY_JSON_FUNC_ int json_internal_into_buf(char **tmp, size_t *tmp_len,
                                           size_t *tmp_cap, JsonIt *it, int c);

/*
 Is the character whitespace.

 NOTE: As according to the JSON RPC Spec it will only check for ascii
 whitespace i.e. '\r', '\n', '\t', ' ' and won't check for Unicode whitespace
 */
_WHY_JSON_FUNC_ int json_internal_is_whitespace(char c);

/*
 Ignore all whitespace moving the iterator to the first non-whitespace char
 */
_WHY_JSON_FUNC_ int json_internal_ignore_whitespace(JsonIt *it);

/*
 Resize the match stack to allow atleast one more member.
 */
_WHY_JSON_FUNC_ int json_internal_resize_match_stack(JsonIt *it);

/*
 Count all closing braces and return 0 (with errno == 0 == JSON_ERR_NO_ERROR)
 If a proper closing brace was found else return 1 if no closing braces were
 found and return 0 (with errno != 0) if an error occurred.

 TODO: Ugh this is ugly, let's fix it.
       Honestly we could just return 1 and check the type?
       Still not great
 */
_WHY_JSON_FUNC_ int json_internal_count_braces(JsonTok *tok, JsonIt *it);

/*
 Free a json string
 Automatically done for token and iterator upon next or destroy calls
 so you shouldn't have to do it manually!
 */
_WHY_JSON_FUNC_ void json_internal_free_str(JsonStr *str);

/*
 Does the character need a '\' before it
 */
_WHY_JSON_FUNC_ int json_internal_char_needs_escaping(int c);

/*
 Convert the character to hex equivalent (i.e. 0 => 0, A/a => 10, ...)
 Returns -1 if it failed to convert.
 */
_WHY_JSON_FUNC_ int json_internal_hex(int c);

/*
 Parse a codepoint of size `len` i.e. used for \uXXXX and \UXXXXXXXX
 */
_WHY_JSON_FUNC_ int json_internal_parse_codepoint(JsonIt *it,
                                                  uint32_t *codepoint, int len);

/*
 Converts a codepoint to utf8 writing into buf
 */
_WHY_JSON_FUNC_ int json_internal_to_utf8(char **tmp, size_t *tmp_len,
                                          size_t *tmp_cap, JsonIt *it,
                                          uint32_t cp);

/*
 Parses a 'string' like object till the given ending character.
 Will stop at the ending character
 */
_WHY_JSON_FUNC_ int json_internal_parse_str_till(JsonStr *out, JsonIt *it,
                                                 char ending);

/*
 Parse an identifier that is a key without the quotes
 */
_WHY_JSON_FUNC_ int json_internal_parse_identifier(JsonStr *out, JsonIt *it);

/*
 Parse a key for an object.
 */
_WHY_JSON_FUNC_ int json_internal_parse_key(JsonTok *tok, JsonIt *it);

/*
 Check if the iterator characters match the given string
 - 1 if they do
 - 0 if they don't
 - -1 if error
 Will stop at the character that failed to match
 */
_WHY_JSON_FUNC_ int json_internal_matches(const char *str, JsonIt *it);

/*
 Parses integers and floats.
 */
_WHY_JSON_FUNC_ int json_internal_parse_num(JsonType *type, JsonValue *value,
                                            JsonIt *it);

/*
 Parses json values i.e. flts, ints, objects, arrays, bool, null, strings
 */
_WHY_JSON_FUNC_ int json_internal_parse_value(JsonType *type, JsonValue *value,
                                              JsonIt *it);

/*
 Parses opening braces that is `{` and `[` and handles the stack response
 */
_WHY_JSON_FUNC_ int json_internal_parse_opening_braces(JsonIt *it);

// == Definitions ==

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

_WHY_JSON_FUNC_ char *json_get_str(JsonStr *str, size_t *len) {
  if (len) {
    *len = str->len;
  }
  if (!str->buf) {
    return NULL;
  }
  if (str->allocated) {
    str->allocated = 0;
    char *tmp = (char *)str->buf;
    str->buf = NULL;
    str->len = 0;
    return tmp;
  } else {
    char *tmp = malloc(sizeof(char) * (str->len + 1));
    strncpy(tmp, str->buf, str->len + 1);
    return tmp;
  }
}

_WHY_JSON_FUNC_ int json_internal_error(JsonIt *it, int err, const char *fmt,
                                        ...) {
  va_list ap;
  va_start(ap, fmt);
  errno = err;
  int res = vsnprintf(it->err, WHY_JSON_ERR_BUF_SIZE, fmt, ap);
  va_end(ap);
  return res;
}

_WHY_JSON_FUNC_ uint32_t json_internal_is_legal_utf8(uint32_t *state,
                                                     const char *bytes,
                                                     size_t length,
                                                     size_t *stop) {
  uint32_t type;
  if (bytes == NULL || state == NULL) {
    return 0;
  }

  size_t i;
  for (i = 0; i < length; i++) {
    type = utf8d[(uint8_t)bytes[i]];
    *state = utf8d[256 + (*state) * 16 + type];

    if (*state == 0) {
      *stop = i;
      break;
    }
  }

  return *state;
}

#define WHY_JSON_GET_COUNT(byte) ((byte) & ~0x80)
#define WHY_JSON_CAN_ADD(byte) (WHY_JSON_GET_COUNT(byte) < (UINT8_MAX & ~0x80))

_WHY_JSON_FUNC_ int json_internal_read(JsonIt *it) {
  if (it->cur_loc != it->buf_len) {
    errno = JSON_ERR_CANT_READ;
    return 0;
  }

  if (it->state == WHY_JSON_UTF8_REJECT) {
    errno = JSON_ERR_INVALID_UTF8;
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
        json_internal_error(it, JSON_ERR_CANT_READ, "Failed to read");
        return 0;
      } else {
        // unknown error
        json_internal_error(it, JSON_ERR_UNDEFINED_NEXT_CHAR, "Unknown Error");
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
    errno = JSON_ERR_INVALID_ARGS;
    return 0;
  }

  // verify that the json is legal
  size_t stop;
  it->state =
      json_internal_is_legal_utf8(&it->state, it->buf, it->buf_len, &stop);
  if (it->state == WHY_JSON_UTF8_REJECT) {
    json_internal_error(it, JSON_ERR_INVALID_UTF8,
                        "Invalid UTF8 JSON Sequence 0x%X", *(it->buf + stop));
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ int json_internal_peek_char(JsonIt *it) {
  if (it->cur_loc == it->buf_len &&
      (!json_internal_read(it) || it->buf_len == 0)) {
    return EOF;
  } else {
    return it->buf[it->cur_loc];
  }
}

_WHY_JSON_FUNC_ int json_internal_init(JsonIt *it) {
  it->stream = NULL;
  it->source_str = NULL;
  it->source_str_len = 0;
  it->source_str_cur = 0;
  it->cur_line = it->cur_col = 1;
  it->state = WHY_JSON_UTF8_ACCEPT;
  it->buf_len = 0;
  it->buf[0] = '\0';
  it->depth = 0;
  it->cur_loc = 0;
  it->err[0] = '\0';
  it->match_stack = malloc(sizeof(uint8_t) * WHY_JSON_INITIAL_MATCH_STACK);
  it->match_len = 0;

  if (it->match_stack == NULL) {
    json_internal_error(it, JSON_ERR_OOM, "Out of memory");
    return 0;
  }

  memset(it->match_stack, 0, sizeof(uint8_t) * WHY_JSON_INITIAL_MATCH_STACK);
  // our 'null terminating' char
  it->match_stack[WHY_JSON_INITIAL_MATCH_STACK - 1] = UINT8_MAX;
  return 1;
}

_WHY_JSON_FUNC_ int json_file(JsonIt *it, FILE *file) {
  if (file == NULL) {
    json_internal_error(it, JSON_ERR_INVALID_ARGS, "File should be valid");
    return 0;
  }

  int res = json_internal_init(it);
  it->stream = file;
  return res;
}

_WHY_JSON_FUNC_ int json_str(JsonIt *it, const char *str) {
  if (str == NULL) {
    json_internal_error(it, JSON_ERR_INVALID_ARGS, "String should be valid");
    return 0;
  }

  int res = json_internal_init(it);
  it->source_str = str;
  it->source_str_len = strlen(str);
  return res;
}

_WHY_JSON_FUNC_ int json_internal_next_char(JsonIt *it) {
  int next = json_internal_peek_char(it);
  it->cur_loc++;
  if (next == '\n') {
    it->cur_col = 0;
    it->cur_line++;
  } else {
    it->cur_col++;
  }
  return next;
}

_WHY_JSON_FUNC_ int json_internal_into_buf(char **tmp, size_t *tmp_len,
                                           size_t *tmp_cap, JsonIt *it, int c) {
  if (*tmp == NULL) {
    *tmp = (char *)malloc(sizeof(char) * WHY_JSON_INITIAL_TMP_BUF_SIZE);
    if (*tmp == NULL) {
      json_internal_error(it, JSON_ERR_OOM, "Out of memory");
      return 0;
    }
    *tmp_cap = 8;
    *tmp_len = 1;
    (*tmp)[0] = c;
    (*tmp)[1] = '\0';
  } else if (*tmp_len < *tmp_cap) {
    (*tmp)[(*tmp_len)++] = c;
    (*tmp)[*tmp_len] = '\0';
  } else {
    if (*tmp_cap < 4) {
      *tmp_cap = 4;
    }
    char *new = (char *)realloc(*tmp, sizeof(char) * (*tmp_cap * 2 + 1));
    if (new == NULL) {
      json_internal_error(it, JSON_ERR_OOM, "Out of memory");
      return 0;
    }
    *tmp = new;
    *tmp_cap *= 2;
    (*tmp)[(*tmp_len)++] = c;
    (*tmp)[*tmp_len] = '\0';
  }

  return 1;
}

_WHY_JSON_FUNC_ int json_internal_is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

_WHY_JSON_FUNC_ int json_internal_ignore_whitespace(JsonIt *it) {
  while (1) {
    if (it->cur_loc == it->buf_len && !json_internal_read(it)) {
      return 0;
    }
    if (it->buf_len == 0) {
      // EOF means no more whitespace
      break;
    }

    if (json_internal_is_whitespace(json_internal_peek_char(it))) {
      json_internal_next_char(it);
    } else {
      break;
    }
  }
  return 1;
}

_WHY_JSON_FUNC_ int json_internal_resize_match_stack(JsonIt *it) {
  // if the next one is uint8_max then we have found end
  // of our buffer and we have to reallocate
  if (it->match_stack[it->match_len] == UINT8_MAX) {
    uint8_t *tmp =
        realloc(it->match_stack, sizeof(uint8_t) * it->match_len * 2);
    if (tmp == NULL) {
      json_internal_error(it, JSON_ERR_OOM, "Out of memory");
      return 0;
    }
    it->match_stack = tmp;
    memset(it->match_stack + it->match_len, 0, it->match_len);
    it->match_stack[it->match_len * 2 - 1] = UINT8_MAX;
  }
  return 1;
}

_WHY_JSON_FUNC_ int json_internal_count_braces(JsonTok *tok, JsonIt *it) {
  if (!json_internal_ignore_whitespace(it)) {
    tok->type = JSON_ERROR;
    return 0;
  }

  char open = '[';
  char close = ']';
  uint8_t end_type = JSON_ARRAY_END;
  uint8_t required_mask = 0;
  if (it->match_len > 0 &&
      (it->match_stack[it->match_len - 1] & 0x80) == 0x80) {
    open = '{';
    close = '}';
    end_type = JSON_OBJECT_END;
    required_mask = 0x80;
  }

  if (it->buf_len == 0) {
    // EOF
    if (it->match_len == 0) {
      // We have no matches and are at EOF this is fine
      tok->type = JSON_END;
      errno = JSON_ERR_NO_ERROR;
      return 0;
    } else {
      json_internal_error(it, JSON_ERR_UNMATCHED_TOKENS, "Unmatched %c", open);
      return 0;
    }
  }

  if (json_internal_peek_char(it) == close) {
    json_internal_next_char(it);
    it->depth--;
    if (it->match_len == 0 ||
        (it->match_stack[it->match_len - 1] & 0x80) != required_mask) {
      json_internal_error(it, JSON_ERR_UNMATCHED_TOKENS,
                          "Extraneous unmatched %c", close);
      return 0;
    }

    if (WHY_JSON_GET_COUNT(--it->match_stack[it->match_len - 1]) == 0) {
      it->match_stack[--it->match_len] = 0;
    }

    if (!json_internal_ignore_whitespace(it)) {
      tok->type = JSON_ERROR;
      return 0;
    }

    tok->type = end_type;
    errno = JSON_ERR_NO_ERROR;
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ void json_internal_free_str(JsonStr *str) {
  if (str->allocated) {
    free((char *)str->buf);
  }
  str->buf = NULL;
  str->len = 0;
  str->allocated = 0;
}

_WHY_JSON_FUNC_ void json_destroy(JsonTok *tok, JsonIt *it) {
  if (it && it->match_stack) {
    free(it->match_stack);
    it->match_stack = NULL;
    it->match_len = 0;
  }
  if (tok) {
    if (tok->key.buf) {
      json_internal_free_str(&tok->key);
    }
    if (tok->type == JSON_STRING && tok->value._str.buf) {
      json_internal_free_str(&tok->value._str);
    }
    // NOTE: Error is the default value
    tok->type = JSON_ERROR;
  }
}

_WHY_JSON_FUNC_ int json_internal_char_needs_escaping(int c) {
  return ((c >= 0) && (c < 0x20 || c == 0x22 || c == 0x5c));
}

_WHY_JSON_FUNC_ int json_internal_hex(int c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'a' && c <= 'z') {
    return c - 'a' + 10;
  } else if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 10;
  } else {
    return -1;
  }
}

_WHY_JSON_FUNC_ int
json_internal_parse_codepoint(JsonIt *it, uint32_t *codepoint, int len) {
  *codepoint = 0;
  for (int i = 0; i < len; i++) {
    int c = json_internal_next_char(it);
    int hex = json_internal_hex(c);
    if (c == EOF || hex == -1) {
      json_internal_error(it, JSON_ERR_INVALID_UTF8, "Invalid Hex Character %c",
                          c);
      return 0;
    }
    *codepoint = (*codepoint << 4) | (hex & 0xF);
  }
  return 1;
}

_WHY_JSON_FUNC_ int json_internal_to_utf8(char **tmp, size_t *tmp_len,
                                          size_t *tmp_cap, JsonIt *it,
                                          uint32_t cp) {
  if (cp <= 0x7Ful) {
    return json_internal_into_buf(tmp, tmp_len, tmp_cap, it, cp);
  } else if (cp <= 0x7FFul) {
    int first = (cp >> 6 & 0x1F) | 0xC0;
    int second = (cp & 0x3F) | 0x80;
    return json_internal_into_buf(tmp, tmp_len, tmp_cap, it, first) &&
           json_internal_into_buf(tmp, tmp_len, tmp_cap, it, second);
  } else if (cp <= 0xFFFF) {
    int first = (cp >> 12 & 0x0F) | 0xE0;
    int second = (cp >> 6 & 0x3F) | 0x80;
    int third = (cp & 0x3F) | 0x80;
    return json_internal_into_buf(tmp, tmp_len, tmp_cap, it, first) &&
           json_internal_into_buf(tmp, tmp_len, tmp_cap, it, second) &&
           json_internal_into_buf(tmp, tmp_len, tmp_cap, it, third);
  } else if (cp <= 0x10FFFF) {
    int first = (cp >> 18 & 0x07) | 0xF0;
    int second = (cp >> 12 & 0x3F) | 0x80;
    int third = (cp >> 6 & 0x3F) | 0x80;
    int fourth = (cp & 0x3F) | 0x80;
    return json_internal_into_buf(tmp, tmp_len, tmp_cap, it, first) &&
           json_internal_into_buf(tmp, tmp_len, tmp_cap, it, second) &&
           json_internal_into_buf(tmp, tmp_len, tmp_cap, it, third) &&
           json_internal_into_buf(tmp, tmp_len, tmp_cap, it, fourth);
  } else {
    json_internal_error(it, JSON_ERR_INVALID_UTF8, "Invalid Utf8 Character %u",
                        cp);
    return 0;
  }
}

_WHY_JSON_FUNC_ int json_internal_parse_str_till(JsonStr *out, JsonIt *it,
                                                 char ending) {
  char *tmp = malloc(sizeof(char));
  size_t tmp_len = 0;
  size_t tmp_cap = 0;
  tmp[0] = '\0';
  int next = 0;

  while (1) {
    next = json_internal_next_char(it);
    if (next == EOF) {
      break;
    }

    if (next == ending) {
      // success!
      break;
    } else if (next == '\\') {
      int c = json_internal_next_char(it);
      if (c == 'u') {
        uint32_t cp = 0;
        if (!json_internal_parse_codepoint(it, &cp, 4)) {
          return 0;
        }

        uint32_t low = 0;
        uint32_t high = 0;
        if (cp >= 0xD800 && cp <= 0xDBFF) {
          // we have the high point
          // now need low
          high = cp;
          cp = 0;
          c = json_internal_next_char(it);
          int next = json_internal_next_char(it);
          if (c == EOF || c != '\\' || next == EOF || next != 'u') {
            json_internal_error(
                it, JSON_ERR_INVALID_UTF8,
                "Was expecting low surrogate character and not %c", next);
            errno = JSON_ERR_INVALID_UTF8;
            return 0;
          }

          if (!json_internal_parse_codepoint(it, &cp, 4)) {
            return 0;
          }

          low = cp;
          if (low < 0xDC00 || low > 0xDFFF) {
            json_internal_error(
                it, JSON_ERR_INVALID_UTF8,
                "Was expecting low surrogate codepoint and not %u", low);
            return 0;
          }
          cp = ((high - 0xD800) * 0x400) + (low - 0xDC00) + 0x10000;
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
          json_internal_error(
              it, JSON_ERR_INVALID_UTF8,
              "Out of place low surrogate (no high surrogate before it) %u",
              cp);
          return 0;
        }
        if (!json_internal_to_utf8(&tmp, &tmp_len, &tmp_cap, it, cp)) {
          return 0;
        }
      } else if (c == 'U') {
        uint32_t cp = 0;
        if (!json_internal_parse_codepoint(it, &cp, 8)) {
          return 0;
        }
        if (!json_internal_to_utf8(&tmp, &tmp_len, &tmp_cap, it, cp)) {
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
          json_internal_error(it, JSON_ERR_UNKNOWN_TOK,
                              "Invalid Escaping char %c", c);
          return 0;
        }
        if (!json_internal_into_buf(&tmp, &tmp_len, &tmp_cap, it, to_write)) {
          return 0;
        }
      }
    } else if (json_internal_char_needs_escaping(next)) {
      break;
    } else {
      if (!json_internal_into_buf(&tmp, &tmp_len, &tmp_cap, it, next)) {
        return 0;
      }
    }
  }

  if (it->buf_len == 0 || next != ending) {
    json_internal_error(it, JSON_ERR_MISSING_QUOTE, "Missing \"");
    return 0;
  }

  out->buf = tmp;
  out->len = tmp_len;
  out->allocated = 1;

  return 1;
}

_WHY_JSON_FUNC_ int json_internal_parse_identifier(JsonStr *out, JsonIt *it) {
  // We support identifiers that are just numbers
  // but also identifiers that are a sequence of any bytes before a `:`
  // we don't support multi-line identifiers
  // We also cut all trailing spaces
  if (!json_internal_parse_str_till(out, it, ':')) {
    return 0;
  }

  // note we have to then backtrack one so that our cur_loc is on ':'
  // @DEBT: @NOTE: We are taking some technical debt here
  // But since strings can't have newlines in them this isn't too bad
  // Just gotta be a bit more careful.
  it->cur_loc--;
  it->cur_col--;
  while (out->len > 0 && json_internal_is_whitespace(out->buf[out->len - 1])) {
    out->len--;
  }
  ((char *)out->buf)[out->len] = '\0';

  // NOTE: Do we want to realloc this to make the buffer smaller
  //       Is it expected of us??
  // NOTE: An empty identifier is still valid, should it be?
  //       I used to think NO, then I thought YES now I'm back to NO

  return 1;
}

_WHY_JSON_FUNC_ int json_internal_parse_key(JsonTok *tok, JsonIt *it) {
  if (!json_internal_ignore_whitespace(it)) {
    return 0;
  }

  if (json_internal_peek_char(it) == '"') {
    // this is a valid key
    json_internal_next_char(it);
    if (!json_internal_parse_str_till(&tok->key, it, '"')) {
      return 0;
    }
  } else {
#ifndef WHY_JSON_STRICT
    if (!json_internal_parse_identifier(&tok->key, it)) {
      return 0;
    }
#else
    errno = JSON_ERR_MISSING_QUOTE;
    it->err = "Missing initial \"";
    return 0;
#endif
  }

  return 1;
}

_WHY_JSON_FUNC_ int json_internal_matches(const char *str, JsonIt *it) {
  while (*str != '\0') {
    if (it->cur_loc == it->buf_len && !json_internal_read(it)) {
      return -1;
    }

    if (*str == json_internal_peek_char(it)) {
      str++;
      json_internal_next_char(it);
    } else {
      break;
    }
  }

  // make sure the next character is not some kind of possible part of the
  // string i.e. just check it is a comma or whitespace
  int peek = json_internal_peek_char(it);
  if (*str != '\0' ||
      (peek != EOF && !json_internal_is_whitespace(peek) && peek != ',')) {
    json_internal_error(
        it, JSON_ERR_INVALID_VALUE,
        "Iterator doesn't match %s, the invalid character is %c", str, peek);
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ int json_internal_parse_num(JsonType *type, JsonValue *value,
                                            JsonIt *it) {
  int sign = 1;
  int peek = json_internal_peek_char(it);
  if (peek == '-') {
    sign = -1;
    json_internal_next_char(it);
  } else if (peek == '+') {
    json_internal_next_char(it);
  }

  *type = JSON_INT;
  char *tmp = NULL;
  size_t tmp_len = 0;
  size_t tmp_cap = 0;
  int seen_dot = 0;
  int seen_exp = 0;
  int prev_exp = 0;
  int prev_underscore = 0;

  while (1) {
    int next = json_internal_peek_char(it);
    if (json_internal_is_whitespace(next) || next == '}' || next == ',' ||
        next == ']') {
      break;
    }

    next = json_internal_next_char(it);
    if (next == EOF && tmp_len > 0) {
      break;
    } else if (next == EOF) {
      json_internal_error(it, JSON_ERR_INVALID_VALUE, "Unexpected EOF");
      return 0;
    } else if (next >= '0' && next <= '9') {
      // this is fine
      json_internal_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
      prev_exp = prev_underscore = 0;
    } else if (next == '.' && !seen_dot && !seen_exp) {
      // still fine if only one
      seen_dot = 1;
      json_internal_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
      prev_underscore = 0;
      *type = JSON_FLT;
    } else if ((next == 'e' || next == 'E') && !seen_exp &&
               tmp_len - seen_dot > 0) {
      seen_exp = 1;
      prev_exp = 1;
      json_internal_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
      prev_underscore = 0;
      *type = JSON_FLT;
    } else if (next == '_' && !prev_underscore) {
      // ignore underscores
      prev_underscore = 1;
    } else if ((next == '+' || next == '-') && prev_exp) {
      prev_exp = prev_underscore = 0;
      json_internal_into_buf(&tmp, &tmp_len, &tmp_cap, it, next);
    } else {
      json_internal_error(it, JSON_ERR_INVALID_VALUE, "Invalid character %c",
                          next);
      return 0;
    }
  }

  if (tmp_len - seen_dot == 0) {
    // NOTE: Do we have to do this??  Or does it automatically check for null
    // strings
    json_internal_error(it, JSON_ERR_INVALID_VALUE, "Not a valid number %s",
                        tmp ? tmp : "null");
    free(tmp);
    return 0;
  }

  if (*type == JSON_INT) {
    value->_int = sign * strtol(tmp, NULL, 10);
  } else {
    value->_flt = sign * strtod(tmp, NULL);
  }

  free(tmp);

  return 1;
}

_WHY_JSON_FUNC_ int json_internal_parse_value(JsonType *type, JsonValue *value,
                                              JsonIt *it) {
  if (!json_internal_ignore_whitespace(it)) {
    return 0;
  }

  int next = json_internal_peek_char(it);
  if (next == '"') {
    // parse str
    *type = JSON_STRING;
    json_internal_next_char(it);
    return json_internal_parse_str_till(&value->_str, it, '"');
  } else if (next == 't') {
    // true
    int res = json_internal_matches("true", it);
    if (res == -1) {
      return 0;
    } else if (res == 1) {
      *type = JSON_BOOL;
      value->_bool = 1;
      return 1;
    }
  } else if (next == 'f') {
    // false
    int res = json_internal_matches("false", it);
    if (res == -1) {
      return 0;
    } else if (res == 1) {
      *type = JSON_BOOL;
      value->_bool = 0;
      return 1;
    }
  } else if (next == 'n') {
    // null
    int res = json_internal_matches("null", it);
    if (res == -1) {
      return 0;
    } else if (res == 1) {
      *type = JSON_NULL;
      return 1;
    }
  } else if (next == '{') {
    // object
    // we don't want to move forward because our next
    // need to know this
    *type = JSON_OBJECT;
    return 1;
  } else if (next == '[') {
    // array
    *type = JSON_ARRAY;
    return 1;
  } else if (next == EOF) {
    json_internal_error(it, JSON_ERR_INVALID_VALUE, "Unexpected EOF");
  } else if (json_internal_parse_num(type, value, it)) {
    // number/int
    // TODO: Do we want someway to see if the parse num failed?
    //       idk maybe?
    return 1;
  } else {
    json_internal_error(it, JSON_ERR_INVALID_VALUE, "Expected value and not %c",
                        next);
  }

  return 0;
}

_WHY_JSON_FUNC_ int json_internal_parse_opening_braces(JsonIt *it) {
  int next = json_internal_next_char(it);
  uint8_t mask = 0;
  if (next == '{') {
    mask = 0x80;
  } else if (next != '[') {
    json_internal_error(it, JSON_ERR_UNKNOWN_TOK, "Invalid %c", next);
    return 0;
  }

  it->depth++;
  if (!json_internal_ignore_whitespace(it)) {
    return 0;
  }

  if (it->match_len > 0 &&
      (it->match_stack[it->match_len - 1] & 0x80) == mask &&
      WHY_JSON_CAN_ADD(it->match_stack[it->match_len - 1])) {
    it->match_stack[it->match_len - 1]++;
  } else {
    if (!json_internal_resize_match_stack(it)) {
      return 0;
    }
    it->match_stack[it->match_len++] = 1 | mask;
  }

  if (it->buf_len == it->cur_loc && !json_internal_ignore_whitespace(it)) {
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ int json_next(JsonTok *tok, JsonIt *it) {
  if (it->buf_len == 0 && it->cur_loc == 0 &&
      ((it->source_str_cur == 0 && it->source_str_len > 0 &&
        it->source_str != NULL) ||
       (it->stream != NULL && !feof(it->stream)))) {
    memset(tok, 0, sizeof(JsonTok));
    tok->first = 1;

    // we are at the start
    // this just means we have a single value
    if (json_internal_parse_value(&tok->type, &tok->value, it) &&
        json_internal_ignore_whitespace(it)) {
      if (tok->type != JSON_ARRAY && tok->type != JSON_OBJECT &&
          json_internal_peek_char(it) != EOF) {
        // if we are not at EOF then clearly something is wrong
        // i.e. "a": 2, "b": 3 is not valid
        json_internal_error(it, JSON_ERR_UNKNOWN_TOK,
                            "Expected EOF since single value was read");
        json_destroy(tok, it);
        return 0;
      } else {
        return 1;
      }
    }
    json_destroy(tok, it);
    return 0;
  }

  if (!json_internal_ignore_whitespace(it)) {
    json_destroy(tok, it);
    return 0;
  }

  tok->first = 0;
  // was the previous a key for object/array
  int prev_was_key = tok->type == JSON_ARRAY || tok->type == JSON_OBJECT;

  // Deallocate the strings but not the iterator
  json_destroy(tok, NULL);

  if (prev_was_key) {
    if (!json_internal_parse_opening_braces(it)) {
      json_destroy(tok, it);
      return 0;
    } else {
      tok->first = 1;
    }
  }

  int comma_done = 0;
#ifndef WHY_JSON_STRICT
  if (json_internal_peek_char(it) == ',') {
    comma_done = 1;
    json_internal_next_char(it);
  }
#endif

  if (!json_internal_count_braces(tok, it)) {
    if (errno == JSON_ERR_NO_ERROR) {
      // We count all ending braces ] as being first
      // This just helps most algorithms treat them
      // as special.
      tok->first = 1;
      return 1;
    } else {
      json_destroy(tok, it);
      return 0;
    }
  }

  if ((it->cur_loc >= it->source_str_len && it->source_str != NULL) ||
      (it->stream != NULL && feof(it->stream))) {
    // The token stream has finished so cleanup
    json_destroy(tok, it);
    tok->type = JSON_END;
    return 1;
  }

  if (!prev_was_key && !comma_done && json_internal_next_char(it) != ',') {
    json_internal_error(it, JSON_ERR_MISSING_COMMA, "Was expecting a comma");
    json_destroy(tok, it);
    return 0;
  }

  if (!json_internal_ignore_whitespace(it)) {
    json_destroy(tok, it);
    return 0;
  }

  // parse key
  if (it->match_len == 0 ||
      (it->match_stack[it->match_len - 1] & 0x80) == 0x80) {
    if (!json_internal_parse_key(tok, it)) {
      json_destroy(tok, it);
      return 0;
    }

    if (!json_internal_ignore_whitespace(it)) {
      json_destroy(tok, it);
      return 0;
    }

    int next = json_internal_next_char(it);
    if (next == EOF || next != ':') {
      // EOF
      json_destroy(tok, it);
      json_internal_error(it, JSON_ERR_UNKNOWN_TOK,
                          "Didn't expect %c was expecting ':'", next);
      return 0;
    }

    if (!json_internal_ignore_whitespace(it)) {
      json_destroy(tok, it);
      return 0;
    }
  }

  // then parse value
  if (json_internal_parse_value(&tok->type, &tok->value, it) != 1) {
    json_destroy(tok, it);
    return 0;
  }

  return 1;
}

_WHY_JSON_FUNC_ int json_skip(JsonTok *tok, JsonIt *it) {
  uint8_t wait;
  if (tok->type == JSON_ARRAY) {
    wait = JSON_ARRAY_END;
  } else if (tok->type == JSON_OBJECT) {
    wait = JSON_OBJECT_END;
  } else {
    json_internal_error(it, JSON_ERR_INVALID_ARGS,
                        "Type of token (%d) isn't array or object can't skip",
                        tok->type);
    return 0;
  }

  errno = 0;
  while (json_next(tok, it) && tok->type != wait) {
  }
  return errno == 0;
}

#undef WHY_JSON_GET_COUNT
#undef WHY_JSON_CAN_ADD

#endif

#if defined __cplusplus
}
#endif

#endif
