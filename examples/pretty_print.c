#include "../whyjson.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Error: Provide a json file to load");
    return 1;
  }

  JsonIt it;
  json_file(&it, fopen(argv[1], "r"));
  JsonTok tok;
  int first = 1;
  errno = 0;
  while (json_next(&tok, &it) && tok.type != JSON_END) {
    if (!first) {
      printf((!tok.first) ? ",\n" : "\n");
    } else {
      first = 0;
    }
    for (int i = 0; i < it.depth; i++) {
      printf("  ");
    }
    if (tok.key.buf != NULL) {
      printf("\"%s\": ", tok.key.buf);
    }
    switch (tok.type) {
    case JSON_ARRAY: {
      printf("[");
    } break;
    case JSON_OBJECT: {
      printf("{");
    } break;
    case JSON_ARRAY_END: {
      printf("]");
    } break;
    case JSON_OBJECT_END: {
      printf("}");
    } break;
    case JSON_STRING: {
      printf("%s", tok.value._str.buf);
    } break;
    case JSON_BOOL: {
      printf("%s", tok.value._bool ? "true" : "false");
    } break;
    case JSON_INT: {
      printf("%ld", tok.value._int);
    } break;
    case JSON_FLT: {
      printf("%lf", tok.value._flt);
    } break;
    case JSON_NULL: {
      printf("null");
    } break;
    case JSON_ERROR: {
      printf("Error!!! %s", it.err);
    } break;
    default: { printf("Unknown: %d\n", tok.type); } break;
    }
  }
  printf("\n");
  if (errno != 0) {
    printf("%d: %s\n", errno, it.err);
  }

  return 0;
}
