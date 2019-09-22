#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#define key_eql(a, str)                                                        \
  obs_test_str_eq(a.buf, str);                                                 \
  obs_test_eq(size_t, a.len, strlen(str));

#define test_next_json(err, wanted_res)                                        \
  do {                                                                         \
    errno = 0;                                                                 \
    int res = json_next(&tok, &it);                                            \
    if (res != wanted_res) {                                                   \
      obs_err("Error json_next is not %s failed errno is %d",                  \
              (wanted_res ? "true" : "false"), errno);                         \
    }                                                                          \
    obs_test_eq(int, errno, err);                                              \
  } while (0)

#define expect_next_obj_string(expect_key, str)                                \
  do {                                                                         \
    test_next_json(0, 1);                                                      \
    obs_test_eq(uint8_t, JSON_STRING, tok.type);                               \
    key_eql(tok.key, expect_key);                                              \
    key_eql(tok.value._str, str);                                              \
  } while (0)

#define expect_next_array_string(str)                                          \
  do {                                                                         \
    test_next_json(0, 1);                                                      \
    obs_test_eq(uint8_t, JSON_STRING, tok.type);                               \
    key_eql(tok.value._str, str);                                              \
  } while (0)

#define expect_next_obj_value(expect_type, expect_key, val_type, expect_value) \
  do {                                                                         \
    test_next_json(0, 1);                                                      \
    obs_test_eq(uint8_t, (expect_type), tok.type);                             \
    if (tok.type != JSON_FLT && tok.type != JSON_INT &&                        \
        tok.type != JSON_BOOL) {                                               \
      obs_err("Invalid Test: Type has to be either JSON_FLT, JSON_BOOL or "    \
              "JSON_INT for expect_next_obj_value");                           \
    }                                                                          \
    key_eql(tok.key, expect_key);                                              \
    val_type tmp = (val_type)expect_value;                                     \
    obs_test_mem_eq(val_type, &tok.value, &tmp);                               \
  } while (0)

#define expect_next_key_only(expect_type, expect_key)                          \
  do {                                                                         \
    test_next_json(0, 1);                                                      \
    obs_test_eq(uint8_t, expect_type, tok.type);                               \
    if (tok.type != JSON_ARRAY && tok.type != JSON_OBJECT &&                   \
        tok.type != JSON_NULL) {                                               \
      obs_err("Invalid Test: Type has to be either JSON_ARRAY, "               \
              "JSON_OBJECT or JSON_NULL for expect_next_key_only");            \
    }                                                                          \
    key_eql(tok.key, expect_key);                                              \
  } while (0)

#define expect_next_array_value(expect_type, val_type, expect_value)           \
  do {                                                                         \
    test_next_json(0, 1);                                                      \
    obs_test_eq(uint8_t, (expect_type), tok.type);                             \
    if (tok.type != JSON_FLT && tok.type != JSON_INT &&                        \
        tok.type != JSON_BOOL) {                                               \
      obs_err("Invalid Test: Type has to be either, JSON_FLT, JSON_BOOL or "   \
              "JSON_INT for expect_next_obj_value");                           \
    }                                                                          \
    val_type tmp = (val_type)expect_value;                                     \
    obs_test_mem_eq(val_type, &tok.value, &tmp);                               \
  } while (0)

#define expect_next_type(expect_type)                                          \
  do {                                                                         \
    test_next_json(0, 1);                                                      \
    obs_test_eq(uint8_t, expect_type, tok.type);                               \
    if (tok.type != JSON_END && tok.type != JSON_OBJECT_END &&                 \
        tok.type != JSON_ARRAY_END && tok.type != JSON_ARRAY &&                \
        tok.type != JSON_OBJECT && tok.type != JSON_NULL) {                    \
      obs_err("Invalid Test: Type has to be either JSON_ARRAY_END, "           \
              "JSON_OBJECT_END, JSON_END, JSON_NULL, "                         \
              "JSON_ARRAY or JSON_OBJECT for expect_next_type");               \
    }                                                                          \
  } while (0)

#define expect_error(err)                                                      \
  do {                                                                         \
    test_next_json(err, 0);                                                    \
    obs_test_eq(uint8_t, tok.type, JSON_ERROR);                                \
  } while (0)

#define setup_str(str)                                                         \
  JsonIt it;                                                                   \
  JsonTok tok;                                                                 \
  errno = 0;                                                                   \
  obs_test_true(json_str(&it, str));

#define setup_file(filename)                                                   \
  FILE *file = fopen(filename, "r");                                           \
  JsonIt it;                                                                   \
  JsonTok tok;                                                                 \
  errno = 0;                                                                   \
  obs_test_true(json_file(&it, file));

#endif
