#include "../whyjson.h"
#include "obsidian.h"
#include <string.h>

#include "test_utils.h"

int main(int argc, char *argv[]) {
  OBS_SETUP("WhyJson", argc, argv);

  OBS_TEST_GROUP("Types", {
    ; // To make this formatted nicely we can just chuck this
    OBS_TEST("Number", {
      setup_str("{ \"a\": 5, \"b\": 10 }");
      expect_next_obj_value(WHY_JSON_INT | WHY_JSON_OBJECT_FLAG, "a", long, 5);
      expect_next_obj_value(WHY_JSON_INT | WHY_JSON_OBJECT_FLAG, "b", long, 10);
      expect_next_type(WHY_JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("String", {
      setup_str("{ \"a\": \"swifty\", \"b\": \"nice\", \"unicode_character\": "
                "\"🐻\" }");
      expect_next_obj_value(WHY_JSON_STRING | WHY_JSON_OBJECT_FLAG, "a", char,
                            "swifty");
      expect_next_obj_value(WHY_JSON_STRING | WHY_JSON_OBJECT_FLAG, "b", char,
                            "nice");
      expect_next_obj_value(WHY_JSON_STRING | WHY_JSON_OBJECT_FLAG,
                            "unicode_character", char, "🐻");
      expect_next_type(WHY_JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Bool", {
      setup_str("{ \"first_one\": true, \"second_one\": false }");
      expect_next_obj_value(WHY_JSON_BOOL | WHY_JSON_OBJECT_FLAG, "first_one",
                            int, 1);
      expect_next_obj_value(WHY_JSON_BOOL | WHY_JSON_OBJECT_FLAG, "second_one",
                            int, 0);
      expect_next_type(WHY_JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Null", {
      setup_str("{ \"this\": null }");
      expect_next_key_only(WHY_JSON_NULL | WHY_JSON_OBJECT_FLAG, "this");
      expect_next_type(WHY_JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Float", {
      setup_str("{ \"a\": 2.2, \"b\": 0.0, \"c\": 4.3e+9, \"d\": 2e-10, \"e\": "
                "-4.4e22 }");
      expect_next_obj_value(WHY_JSON_FLT | WHY_JSON_OBJECT_FLAG, "a", double, 2.2);
      expect_next_obj_value(WHY_JSON_FLT | WHY_JSON_OBJECT_FLAG, "b", double, 0.0);
      expect_next_obj_value(WHY_JSON_FLT | WHY_JSON_OBJECT_FLAG, "c", double, 4.3e+9);
      expect_next_obj_value(WHY_JSON_FLT | WHY_JSON_OBJECT_FLAG, "d", double, 2e-10);
      expect_next_obj_value(WHY_JSON_FLT | WHY_JSON_OBJECT_FLAG, "e", double, -4.4e22);
      expect_next_type(WHY_JSON_END);
      obs_test_eq(int, errno, 0);
    })
  })

#ifndef WHY_JSON_STRICT
  OBS_TEST_GROUP("Non strict", {
    ; // TODO
    OBS_TEST("Extra Commmas", {
      setup_str("{ \"a\": 2, \"b\": [1, ], }");
      expect_next_obj_value(WHY_JSON_INT | WHY_JSON_OBJECT_FLAG, "a", long, 2);
      expect_next_key_only(WHY_JSON_ARRAY | WHY_JSON_OBJECT_FLAG, "b");
      expect_next_array_value(WHY_JSON_INT | WHY_JSON_ARRAY_FLAG, long, 1);
      expect_next_type(WHY_JSON_ARRAY_END);
      expect_next_type(WHY_JSON_END);
      obs_test_eq(int, errno, 0);
    });
    OBS_TEST("Outer braces", {
      setup_str("   \"a\": 94, \"b\": \"hey 🦍\", \"this is a long name\": "
                "2.223e9, \"d\": true, "
                "\"e\": null ");
      expect_next_obj_value(WHY_JSON_INT, "a", long, 94);
      expect_next_obj_value(WHY_JSON_STRING, "b", char, "hey 🦍");
      expect_next_obj_value(WHY_JSON_FLT, "this is a long name", double,
                            2.223e9);
      expect_next_obj_value(WHY_JSON_BOOL, "d", int, 1);
      expect_next_key_only(WHY_JSON_NULL, "e");
      expect_next_type(WHY_JSON_END);
      obs_test_eq(int, errno, 0);
    })
  })
#else
  OBS_TEST_GROUP("Strict", {
    ;
    OBS_TEST("Extra commas object", {
      setup_str("{ \"a\": 2, }");
      expect_next_obj_value(WHY_JSON_INT | WHY_JSON_OBJECT_FLAG, "a", long, 2);
      obs_test_false(why_json_next(&tok, &it));
      obs_test_eq(int, errno, WHY_JSON_ERR_MISSING_QUOTE);
      obs_test_eq(uint8_t, tok.type, WHY_JSON_ERROR);
    })

    OBS_TEST("Outer braces", {
      setup_str("   \"a\": 94, \"b\": \"hey 🦍\", \"this is a long name\": "
                "2.223e9, \"d\": true, "
                "\"e\": null ");
      obs_test_false(why_json_next(&tok, &it));
      obs_test_eq(int, errno, WHY_JSON_ERR_UNKNOWN_TOK);
      obs_test_eq(uint8_t, it.invalid_char, '"');
      obs_test_eq(uint8_t, tok.type, WHY_JSON_ERROR);
    })
  })
#endif

  OBS_TEST_GROUP("Files", {
    ; // Collected from the internet
  })

  OBS_REPORT;
  return tests_failed;
}
