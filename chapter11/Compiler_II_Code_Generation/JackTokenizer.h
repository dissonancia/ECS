#ifndef JACKTOKENIZER_H_
#define JACKTOKENIZER_H_

#include "Utils.h"

typedef enum {
    KEYWORD,
    SYMBOL,
    IDENTIFIER,
    INT_CONST,
    STRING_CONST,
    _EOF
} JackTokenType;

typedef struct {
    JackTokenType type;
    char* lexeme;
} Token;

typedef struct {
    Token* items;
    size_t count;
    size_t capacity;
} Tokens;

typedef struct {
    String_Builder data;
    String_View content;
    size_t pos;
    int ch;
} JackTokenizer;

bool tokenizer_init(JackTokenizer* jt, const char* path);
void tokenizer_scan_all(JackTokenizer* jt, Tokens *out);
void tokenizer_free(JackTokenizer* jt);

void tokens_free(Tokens *t);

#endif // JACKTOKENIZER_H_