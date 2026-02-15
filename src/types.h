#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>



typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t b8;
typedef uint32_t b32;

struct LH_String {
    char*   data;
    u64     len;
    u64     cap;
};

#define LH_STRING_NULL LH_String { NULL, 0, 0 }

LH_String   string_init(const u64 cap);
b8          string_compare(const LH_String* str1, const LH_String* str2);
u8          string_from(LH_String* dst,  const char* src);
u8          string_append(LH_String* str, char ch);
LH_String   string_copy_data(const LH_String* src);

#ifdef LH_TYPES_IMPLEMENTATION
LH_String string_init(const u64 cap) {
    LH_String str;

    str.cap     = cap;
    str.len     = 0;
    str.data    = (char*)malloc(str.cap);

    return str;
}

b8 string_compare(const LH_String* str1, const LH_String* str2) {
    return strncmp(str1->data, str2->data, str1->len);
}

u8 string_from(LH_String* dst,  const char* src) {
    dst->cap    = strlen(src);
    dst->len    = strlen(src);
    dst->data   = (char*)malloc(dst->cap);
    strcpy(dst->data, src);

    return 0;
}

u8 string_append(LH_String* str, char ch) {
    if (str->len == str->cap) {
        str->cap *= 2;
        str->data = (char*)realloc(str->data, str->cap);
    }

    str->data[str->len++] = ch;
    str->data[str->len] = '\0';

    return 0;
}

LH_String string_copy_data(const LH_String* src) {
    if (src->data == NULL) { return LH_STRING_NULL; }
    LH_String res;
    string_from(&res, src->data);
    return res;
}

#endif