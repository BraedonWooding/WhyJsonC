#include "../whyjson.h"

#include <ctype.h>

const char *escape[256] = {['\0'] = "\\0",
                           ['\n'] = "\\n",
                           ['\t'] = "\\t",
                           ['\r'] = "\\r",
                           ['"'] = "\\\"",
                           ['\\'] = "\\",
                           NULL};

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
  char buf[BUFSIZ];
  while (json_next(&tok, &it) && tok.type != JSON_END) {
    int tmp = !first && !tok.first;
    buf[0] = ',' * tmp;
    buf[tmp] = '\n';
    first = 0;

    memset(buf + tmp + 1, ' ', it.depth * 4);
    fwrite(buf, 1, tmp + it.depth * 4 + 1, stdout);

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
      putchar('"');
      size_t cur = 0;
      for (size_t i = 0; i < tok.value._str.len; i++) {
        if ((int)tok.value._str.buf[i] > 0 &&
            escape[(int)tok.value._str.buf[i]] != NULL) {
          if (i - cur > 0) {
            printf("%.*s", (int)(i - cur), tok.value._str.buf + cur);
          }
          cur = i + 1;
          printf("%s", escape[(int)tok.value._str.buf[i]]);
        }
      }
      if (cur < tok.value._str.len) {
        printf("%.*s", (int)(tok.value._str.len - cur),
               tok.value._str.buf + cur);
      }
      putchar('"');
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
