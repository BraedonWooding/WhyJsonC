# Introduction

## Pros / Cons

### Pros

- Memory efficient (very relevant for extremely large JSONs) only needs to store the current JSON token at any point i.e. doesn't store the entire JSON file into a single map
- Fast (benchmarks [here](__benchmarks))
- Doesn't require reading the entire file in to read the JSON meaning the file can be some kind of device that blocks providing the JSON over time (and I'll release a streaming version eventually too)
- Doesn't hammer your IO for the reasons above

### Cons

- You can't just ask for a certain thing i.e. give me the value for the key 'type' this would require the loading of the entire file which is inherently against the point of this library
- Doing some stuff that requires above can potentially be ugly.  For example if you had a game where all entities were in a single big file and were distinguished by types it would potentially by annoying to use this.
  - Of course doing it this way is kinda bad since it's messy and hard to edit the JSON and you want some structure to your types but whatever floats your boat.
  - A better way would be to have different files for each type and refer to them by name if you need crossreferencing (and lazily verify the names) or do something like;
```json
{
  "enemies": [
    // all the enemies
  ],
  "tiles": [
    // all the tiles
  ]
  // so on...
}
```

?> You could also use the new type tag syntax (that is planned but not yet implemented) that will popup soon which basiaclly allows you to specify that all objects need a type associated with them like `"a": type Enemy { }` which just wraps that type as the first member of the object with key being `type` and value being `Enemy`

## Philosophy / Overview of how it works

Effectively acts as a json iterator for example the following JSON sequence

```json
{
  "a": "hey",
  "c": [1, 2]
}
```

Becomes

`OBJECT, STRING("a", "hey"), ARRAY("c"), INT(1), INT(2), END_ARRAY, END_OBJECT`

This basically means that the usage of the library is something like;

```c
int main(int argc, char *argv[]) {
  FILE *f = fopen(argv[1], "r");
  // don't need to default initialise
  JsonIt it;
  JsonTok tok;
  if (!json_file(&it, f)) {
    fprintf(stderr, "%s:%d:%d Err: %s\n", argv[1], it.cur_line, it.cur_loc, it.err);
    return 1;
  }
  
  while (json_next(&tok, &it) && tok.type != JSON_END && tok.type != JSON_ERROR) {
    switch (tok.type) {
      case JSON_STRING: {} break;
      case JSON_FLT: {} break;
      case JSON_INT: {} break;
      case JSON_ARRAY: {} break;
      case JSON_OBJECT: {} break;
      case JSON_OBJECT_END: {} break;
      case JSON_ARRAY_END: {} break;
      case JSON_NULL: {} break;
      case JSON_BOOL: {} break;
    }
  }
  
  if (tok.type == JSON_ERROR) {
    fprintf(stderr, "%s:%d:%d Err: %s\n", argv[1], it.cur_line, it.cur_loc, it.err); 
    return 1;
  }
  
  return 0;
}
```

Typically you would even have custom parsing functions when you reach an object (or array) that requires it (i.e. in a game you may want to pass multiple entities of a given type).  You can either use object type syntax (upcoming new syntax to allow you to specify a 'type' tag for an object it is just implemented as forcing the first field to be `"type": "tag"`) or either just always put 'type' at the front OR just read the tokens and copy them into buffers or various other solutions :).

## Structures

### `JsonIt`

Represents the iterator itself most of the fields shouldn't be touched (you also don't have to default initialise it since the json_file/json_str functions will).

You can touch the following however without any fear:

- `char err[WHY_JSON_ERR_BUF_SIZE]` holds the current error message you can check errno to detect if an error occurred (or just see if the json token type is JSON_ERROR)
- `int cur_line` the current line the iterator is at (more useful for errors than anything)
- `int cur_col` the current column the iterator is at
- `int depth` the depth of the current token (i.e. nesting depth)

### `JsonTok`

Holds a specific token you probably want to just place this on the stack :) this holds the output for a given iteration.

!> All token data is overriden upon getting the next token!

- `JsonStr key` holds the key (can hold a null string if no key)
- `JsonType type` the type of the token
- `JsonValue value` holds the value of the token
- `char first` is this the first token in the given depth
  - i.e. for `{"a": 2, "c": 3, "d": [10, 100]}` `a: 2` and `10` will have the first flag flipped true

### `JsonStr`

Holds a null terminated string.  We store it this way so you can take the string and edit it (turning the allocation flag off) or not edit it and have the iterator re-use it for later calls.  Currently only keys are re-used if they are after each other.

- `const char *buf` holds the string data (is null terminated)
- `size_t len` the length of the string
- `char allocated` is the string allocated

To get the string that is editable (and won't be overriden) you can use `char json_get_str(JsonStr *str, size_t *len)` (it returns the string and you can also extract the length via a size_t pointer passed in)

### `JsonType`

Is an emum holding one of:
- `JSON_ERROR` an error occurred check the iterator error buffer and cur\_col, cur\_line fields
- `JSON_STRING` the token is a string (the value that is)
- `JSON_BOOL` the token is a boolean (true/false)
- `JSON_INT` the token holds an integer (of type `long`)
- `JSON_FLT` the token holds a floating point (of type `double`)
- `JSON_NULL` the token value is 'null'
- `JSON_OBJECT` we reached an object (can have a key) keep reading to read the members of the object or use `json_skip` to skip the object members
- `JSON_ARRAY` we reached an array (can have a key) keep reading to read the members of the array or use `json_skip` to skip the array members
- `JSON_OBJECT_END` we are at the end of the object (can't have a key and is always first)
- `JSON_ARRAY_END` we are at the end of the array (can't have a key and is always first)
- `JSON_END` we are at the end of the json document (i.e. finished reading) will call `json_destroy()` for you

### `JsonValue`

Is a union of `long _int, double _flt JsonStr _str, char _bool` you should check the type before accessing the values.

## Functions

There are only 6 functions

### `int json_file(JsonIt *it, FILE *file);`

This opens an iterator given a file, initialises `it` as well.

?> Performs reads into an intermediate buffer meaning it doesn't need the whole file at once, if you want the file to be streaming just don't send EOF till you finish writing or make it a blocking read till you get data.

### `int json_str(JsonIt *it, const char *str);`

Pretty much identical to the file one but uses a string to read from.

?> Very efficient string reading it doesn't use an intermediate buffer it just iterates directly over the string.

### `int json_next(JsonTok *tok, JsonIt *it);`

Gets the next token, will free all strings and cleanup memory from the last token.

!> Tokens are always cleared upon this call.  Also if the result is JSON\_DONE OR JSON\_ERROR then json_destroy is called to cleanup the iterator for you

### `int json_skip(JsonTok *tok JsonIt *it);`

Skips the object/array in the case that you don't want to visit it's members.  Will error if the type of the token isn't object/array (i.e. if the previous one wasn't an object/array).

Will also invalidate previous tok just like json_next.

### `int json_destroy(JsonTok *tok, JsonIt *it);`

Destroys the iterator and token data.  Either / both can be null (it won't do anything on NULL tokens/iterators) i.e. to just destroy token you can do `json_destroy(&tok, NULL);`.

?> Basically just frees the strings inside token this is safe to call prior to using next if you don't destroy the iterator.

!> DO NOT CALL json_next if the iterator has been destroyed it will just error out

### `char *json_get_str(JsonStr *str, size_t *len);`

Gives you a mutable string version (that also won't be freed upon the next call of json_next) of str.  Will avoid allocating if the string was already allocated (in some cases it can avoid allocation).

## Differences from standard JSON

NOTE: all these differences can be disabled by doing `#define WHY_JSON_STRICT`

- Supports extra commas at the end i.e. `[1, 2, 3,]`
- You don't need to quote keys (you still need to quote string values)

## Roadmap

- Support schemas
- Support comments
- Support object type syntax in the form (maybe??)

```json
{
  a: type Enemy {
    hp: 10
    // .. so on
  },
  enemies: [
    type Enemy {
      hp: 20
      // so on
    }
  ]
}
```

?> Note that the `a: ` and `hp: ` syntax is already allowed you don't need to quote keys.  This syntax would have to be turned on for each individual JSON and would force all objects to have a type (except outer object)
