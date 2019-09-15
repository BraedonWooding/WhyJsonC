#include "whyjson.h"

int main(void) {
  char *json =
      "{ \"a\": 2, \"b\": \"Coolies 🎫\", \"c\": [1, 2, []], \"k\": 5.4 }";
  WhyJsonIt it;
  why_json_str(&it, json);
  WhyJsonTok tok;

  errno = 0;
  printf("{");
  while (why_json_next(&tok, &it) &&
         !WHY_JSON_TYPE_IS(tok.type, WHY_JSON_END)) {
    if (!tok.first)
      printf(",\n");
    else
      printf("\n");
    for (int i = 0; i < it.depth; i++) {
      printf("  ");
    }
    if (tok.key.buf != NULL)
      printf("\"%s\": ", tok.key.buf);
    switch (WHY_JSON_TYPE_STRIP(tok.type)) {
    case WHY_JSON_ARRAY: {
      printf("[");
    } break;
    case WHY_JSON_OBJECT: {
      printf("{");
    } break;
    case WHY_JSON_ARRAY_END: {
      printf("]");
    } break;
    case WHY_JSON_OBJECT_END: {
      printf("}");
    } break;
    case WHY_JSON_NONE: {
      printf("NONE");
    } break;
    case WHY_JSON_STRING: {
      printf("%s", tok.value._str.buf);
    } break;
    case WHY_JSON_BOOL: {
      printf("%s", tok.value._bool ? "true" : "false");
    } break;
    case WHY_JSON_INT: {
      printf("%ld", tok.value._int);
    } break;
    case WHY_JSON_FLT: {
      printf("%lf", tok.value._flt);
    } break;
    case WHY_JSON_NULL: {
      printf("null");
    } break;
    case WHY_JSON_ERROR: {
      printf("Error!!! %s", it.err);
    } break;
    default: { printf("WAT %d\n", tok.type); } break;
    }
  }
  printf("\n}\n");

  return 0;
}
