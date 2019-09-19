#include "../whyjson.h"
#include "obsidian.h"
#include <string.h>

#include "test_utils.h"

int main(int argc, char *argv[]) {
  OBS_SETUP("Json", argc, argv);

  OBS_TEST_GROUP("Types", {
    ; // To make this formatted nicely we can just chuck this
    OBS_TEST("Number", {
      setup_str("{ \"a\": 5, \"b\": 10 }");
      expect_next_type(JSON_OBJECT);
      expect_next_obj_value(JSON_INT, "a", long, 5);
      expect_next_obj_value(JSON_INT, "b", long, 10);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("String", {
      setup_str("{ \"a\": \"swifty\", \"b\": \"nice\", \"unicode_character\": "
                "\"🐻\" }");
      expect_next_type(JSON_OBJECT);
      expect_next_obj_value(JSON_STRING, "a", char, "swifty");
      expect_next_obj_value(JSON_STRING, "b", char, "nice");
      expect_next_obj_value(JSON_STRING, "unicode_character", char, "🐻");
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Bool", {
      setup_str("{ \"first_one\": true, \"second_one\": false }");
      expect_next_type(JSON_OBJECT);
      expect_next_obj_value(JSON_BOOL, "first_one", int, 1);
      expect_next_obj_value(JSON_BOOL, "second_one", int, 0);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Null", {
      setup_str("{ \"this\": null }");
      expect_next_type(JSON_OBJECT);
      expect_next_key_only(JSON_NULL, "this");
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Float", {
      setup_str("{ \"a\": 2.2, \"b\": 0.0, \"c\": 4.3e+9, \"d\": 2e-10, \"e\": "
                "-4.4e22 }");
      expect_next_type(JSON_OBJECT);
      expect_next_obj_value(JSON_FLT, "a", double, 2.2);
      expect_next_obj_value(JSON_FLT, "b", double, 0.0);
      expect_next_obj_value(JSON_FLT, "c", double, 4.3e+9);
      expect_next_obj_value(JSON_FLT, "d", double, 2e-10);
      expect_next_obj_value(JSON_FLT, "e", double, -4.4e22);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Array", {
      setup_str(
          "{ \"hello sailor\": [1, 2.5, [\"hey\", 9]], \"cool\": [1.0, []] }");
      expect_next_type(JSON_OBJECT);
      expect_next_key_only(JSON_ARRAY, "hello sailor");
      expect_next_array_value(JSON_INT, long, 1);
      expect_next_array_value(JSON_FLT, double, 2.5);
      expect_next_type(JSON_ARRAY);
      expect_next_array_value(JSON_STRING, char, "hey");
      expect_next_array_value(JSON_INT, long, 9);
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_ARRAY_END);
      expect_next_key_only(JSON_ARRAY, "cool");
      expect_next_array_value(JSON_FLT, double, 1.0);
      expect_next_type(JSON_ARRAY);
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Object", {
      setup_str("{ \"hey\": { \"one\": {} }, \"array\": [ {\"1\": 2}, {} ] }");
      expect_next_type(JSON_OBJECT);
      expect_next_key_only(JSON_OBJECT, "hey");
      expect_next_key_only(JSON_OBJECT, "one");
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_OBJECT_END);
      expect_next_key_only(JSON_ARRAY, "array");
      expect_next_type(JSON_OBJECT);
      expect_next_obj_value(JSON_INT, "1", long, 2);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_OBJECT);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })

    OBS_TEST("Outer Object", {
      setup_str("[ 1, \"hey\", null, { \"one\": [] }, [] ]");
      expect_next_type(JSON_ARRAY);
      expect_next_array_value(JSON_INT, long, 1);
      expect_next_array_value(JSON_STRING, char, "hey");
      expect_next_type(JSON_NULL);
      expect_next_type(JSON_OBJECT);
      expect_next_key_only(JSON_ARRAY, "one");
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_ARRAY);
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    })
  })

  OBS_TEST_GROUP("Unicode", {
    ;
    OBS_TEST("Ascii Unicode", {
      setup_str("\"\\u0068\\u0065\\u006c\\u006c\\u006F\"");
      expect_next_array_value(JSON_STRING, char, "hello");
    })

    OBS_TEST("Surrogate pair invalid", {
      setup_str("\"\\uD800\\u0065\"");
      expect_error(JSON_ERR_INVALID_UTF8);
    })

    OBS_TEST("Lonely Surrogate pair", {
      setup_str("\"\\uDC00\"");
      expect_error(JSON_ERR_INVALID_UTF8);
    })

    OBS_TEST("Full Codepoint", {
      setup_str("\"\\U00010348\"");
      expect_next_array_value(JSON_STRING, char, "\U00010348");
    })

    OBS_TEST("Surrogate halves wrong order", {
      setup_str("\"\\uDc00\\uD800\"");
      expect_error(JSON_ERR_INVALID_UTF8);
    })

    OBS_TEST("Valid Pair", {
      setup_str("\"\\uD800\\uDC00\"");
      expect_next_array_value(JSON_STRING, char, "\xf0\x90\x80\x80");
    })
  })

  OBS_TEST_GROUP("Errors", {
    ;
    OBS_TEST("No outer braces", {
      setup_str("   \"a\": 94, \"b\": \"hey 🦍\", \"this is a long name\": "
                "2.223e9, \"d\": true, "
                "\"e\": null ");
      expect_error(JSON_ERR_UNKNOWN_TOK);
    })

    OBS_TEST("String with newlines", {
      setup_str("\"1, 2, 34\n\"");
      expect_error(JSON_ERR_MISSING_QUOTE);
    })

#ifndef JSON_STRICT
    OBS_TEST("Key with newlines", {
      setup_str("{ 1, 2, 3\n: 4 }");
      expect_next_type(JSON_OBJECT);
      expect_error(JSON_ERR_MISSING_QUOTE);
    })
#endif
  })

#ifndef JSON_STRICT
  OBS_TEST_GROUP("Non strict", {
    ; // TODO
    OBS_TEST("Extra Commmas", {
      setup_str("{ \"a\": 2, \"b\": [1, ], }");
      expect_next_type(JSON_OBJECT);
      expect_next_obj_value(JSON_INT, "a", long, 2);
      expect_next_key_only(JSON_ARRAY, "b");
      expect_next_array_value(JSON_INT, long, 1);
      expect_next_type(JSON_ARRAY_END);
      expect_next_type(JSON_OBJECT_END);
      expect_next_type(JSON_END);
      obs_test_eq(int, errno, 0);
    });

    OBS_TEST("Keys not quoted", {
        setup_str("{ a: 5, b a h da : \"bob\", k: [], : \"empty\" }");
        expect_next_type(JSON_OBJECT);
        expect_next_obj_value(JSON_INT, "a", long, 5);
        expect_next_obj_value(JSON_STRING, "b a h da", char, "bob");
        expect_next_key_only(JSON_ARRAY, "k");
        expect_next_type(JSON_ARRAY_END);
        expect_next_obj_value(JSON_STRING, "", char, "epty");
        expect_next_type(JSON_OBJECT_END);
        expect_next_type(JSON_END);
        obs_test_eq(int, errno, 0);
      })
  })
#else
  OBS_TEST_GROUP("Strict", {
    ;
    OBS_TEST("Extra commas object", {
      setup_str("{ \"a\": 2, }");
      expect_next_type(JSON_OBJECT);
      expect_next_obj_value(JSON_INT, "a", long, 2);
      expect_error(JSON_ERR_MISSING_QUOTE);
    })

    OBS_TEST("Extra commas array", {
      // TODO: Make the error more accurate
      setup_str("[ 2, ]");
      expect_next_type(JSON_ARRAY);
      expect_next_array_value(JSON_INT, long, 2);
      expect_error(JSON_ERR_MISSING_QUOTE);
    })
  })
#endif

  OBS_TEST_GROUP("Files", {
    ; // Collected from the internet
  })

  OBS_REPORT;
  return tests_failed;
}
