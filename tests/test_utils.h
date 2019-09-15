#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#define key_eql(a, str)                                                        \
  obs_test_str_eq(a.buf, str);                                                 \
  obs_test_eq(size_t, a.len, strlen(str));

#define expect_next_key_only(expect_type, expect_key)                          \
  obs_test_true(why_json_next(&tok, &it));                                     \
  obs_test_eq(uint8_t, expect_type, tok.type);                                 \
  uint8_t type_strip = WHY_JSON_TYPE_STRIP(expect_type);                       \
  if (type_strip != WHY_JSON_ARRAY && type_strip != WHY_JSON_OBJECT &&         \
      type_strip != WHY_JSON_NULL) {                                           \
    obs_err("Invalid Test: Type has to be either WHY_JSON_ARRAY, "             \
            "WHY_JSON_OBJECT or WHY_JSON_NULL for expect_next_key_only");      \
  }                                                                            \
  key_eql(tok.key, expect_key);

#define expect_next_obj_value(expect_type, expect_key, val_type, expect_value) \
  do {                                                                         \
    obs_test_true(why_json_next(&tok, &it));                                   \
    uint8_t type_strip = WHY_JSON_TYPE_STRIP(expect_type);                     \
    obs_test_eq(uint8_t, (expect_type), tok.type);                             \
    if (type_strip != WHY_JSON_STRING && type_strip != WHY_JSON_FLT &&         \
        type_strip != WHY_JSON_INT && type_strip != WHY_JSON_BOOL) {           \
      obs_err("Invalid Test: Type has to be either WHY_JSON_STRING, "          \
              "WHY_JSON_FLT, WHY_JSON_BOOL or WHY_JSON_INT for "               \
              "expect_next_obj_value");                                        \
    }                                                                          \
    key_eql(tok.key, expect_key);                                              \
    if (type_strip == WHY_JSON_STRING) {                                       \
      key_eql(tok.value._str, (char *)(size_t)expect_value);                   \
    } else {                                                                   \
      val_type tmp = (val_type)expect_value;                                   \
      obs_test_mem_eq(val_type, &tok.value, &tmp);                             \
    }                                                                          \
  } while (0)

#define expect_next_array_value(expect_type, val_type, expect_value)           \
  do {                                                                         \
    obs_test_true(why_json_next(&tok, &it));                                   \
    uint8_t type_strip = WHY_JSON_TYPE_STRIP(expect_type);                     \
    obs_test_eq(uint8_t, (expect_type), tok.type);                             \
    if (type_strip != WHY_JSON_STRING && type_strip != WHY_JSON_FLT &&         \
        type_strip != WHY_JSON_INT && type_strip != WHY_JSON_BOOL) {           \
      obs_err("Invalid Test: Type has to be either WHY_JSON_STRING, "          \
              "WHY_JSON_FLT, WHY_JSON_BOOL or WHY_JSON_INT for "               \
              "expect_next_obj_value");                                        \
    }                                                                          \
    if (type_strip == WHY_JSON_STRING) {                                       \
      key_eql(tok.value._str, (char *)(size_t)expect_value);                   \
    } else {                                                                   \
      val_type tmp = (val_type)expect_value;                                   \
      obs_test_mem_eq(val_type, &tok.value, &tmp);                             \
    }                                                                          \
  } while (0)

#define expect_next_type(expect_type)                                          \
  obs_test_true(why_json_next(&tok, &it));                                     \
  obs_test_eq(uint8_t, expect_type, tok.type);                                 \
  if (expect_type != WHY_JSON_END && expect_type != WHY_JSON_OBJECT_END &&     \
      expect_type != WHY_JSON_ARRAY_END) {                                     \
    obs_err("Invalid Test: Type has to be either WHY_JSON_ARRAY_END, "         \
            "WHY_JSON_OBJECT_END or WHY_JSON_END for expect_next_type");       \
  }

#define setup_str(str)                                                         \
  WhyJsonIt it;                                                                \
  WhyJsonTok tok;                                                              \
  errno = 0;                                                                   \
  obs_test_true(why_json_str(&it, str));

#define setup_file(filename)                                                   \
  FILE *file = fopen(filename, "r");                                           \
  WhyJsonIt it;                                                                \
  WhyJsonTok tok;                                                              \
  errno = 0;                                                                   \
  obs_test_true(why_json_file(&it, file));

#endif
