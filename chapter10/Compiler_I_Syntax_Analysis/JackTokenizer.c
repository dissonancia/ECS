#include "JackTokenizer.h"
#include "Utils.h"

typedef struct {
    char* items;
    size_t count;
    size_t capacity;
} Buffer;

static void buf_clear(Buffer *b) {
    b->count = 0;
    if (b->items && b->capacity > 0)
        b->items[0] = '\0';
}

static void buf_push(Buffer *b, char c) {
    da_append(b, c);
}

static char* buf_take_cstr(Buffer *b) {
    buf_push(b, '\0');
    b->count--;

    char *res = strdup(b->items);

    buf_clear(b);

    return res;
}

static void add_token(Tokens *ts, JackTokenType type, const char* lexeme) {
    Token t = { type, strdup(lexeme) };
    da_append(ts, t);
}

static inline int jt_peek(const JackTokenizer* jt) {
    return (jt->pos < jt->content.count) ? (unsigned char)jt->content.data[jt->pos] : -1;
}

static inline int jt_next(JackTokenizer* jt) {
    if (jt->pos < jt->content.count) {
	jt->ch = (unsigned char)jt->content.data[jt->pos++];
    } else {
	jt->ch = -1; // EOF
    }
    return jt->ch;
}

bool tokenizer_init(JackTokenizer* jt, const char* path) {
    jt->data = (String_Builder){0};
    if (!read_entire_file(path, &jt->data)) {
	errno = EIO;
	return false;
    }

    jt->content = sb_to_sv(jt->data);
    jt->pos = 0;
    jt->ch  = -2;
    return true;
}

void tokenizer_free(JackTokenizer* jt) {
    sb_free(jt->data);
}

typedef enum {
    ST_START, ST_IDENT, ST_INT, ST_STRING, ST_COMMENT_LINE, ST_COMMENT_BLOCK
} State;

void tokenizer_scan_all(JackTokenizer* jt, Tokens* out) {
    *out = (Tokens){0};
    Buffer buf = {0};
    State state = ST_START;

    for (jt_next(jt); jt->ch != -1; jt_next(jt)) {
	char c = (char)jt->ch;

	switch (state) {
	case ST_START:
	    if (isspace((unsigned char)c)) {
		state = ST_START;
	    } else if (isalpha((unsigned char)c) || c == '_') {
		buf_push(&buf, c); state = ST_IDENT;
	    } else if (isdigit((unsigned char)c)) {
		buf_push(&buf, c); state = ST_INT;
	    } else if (c == '"') {
		state = ST_STRING;
	    } else if (c == '/') {
		int n = jt_peek(jt);
		if (n == '/') { state = ST_COMMENT_LINE; }
		else if (n == '*') { jt_next(jt); state = ST_COMMENT_BLOCK; }
		else { add_token(out, SYMBOL, "/"); }
	    } else if (strchr("{}()[].,;+-*&|<>=~", c)) {
		char symbol[2] = {c, 0};
		add_token(out, SYMBOL, symbol);
	    } else {
		fprintf(stderr, "lex error: '%c'\n", c);
		state = ST_START;
	    }
	    break;

	case ST_IDENT:
	    if (isalnum((unsigned char)c) || c == '_') {
		buf_push(&buf, c);
	    } else {
		jt->pos--;
		char* lex = buf_take_cstr(&buf);
		static const char* keywords[] = {
		    "class","constructor","function","method","field","static","var","int",
		    "char","boolean","void","true","false","null","this","let","do","if",
		    "else","while","return"
		};
		JackTokenType t = IDENTIFIER;
		for (size_t i = 0; i < 21; ++i) {
		    if (strcmp(lex, keywords[i]) == 0) {
			t = KEYWORD;
			break;
		    }
		}
		add_token(out, t, lex);
		free(lex);
		state = ST_START;
	    }
	    break;

	case ST_INT:
	    if (isdigit((unsigned char)c)) {
		buf_push(&buf, c);
	    } else {
		jt->pos--;
		char* lex = buf_take_cstr(&buf);
		add_token(out, INT_CONST, lex);
		free(lex);
		state = ST_START;
	    }
	    break;

	case ST_STRING:
	    if (c == '"') {
		char* lex = buf_take_cstr(&buf);
		add_token(out, STRING_CONST, lex);
		free(lex);
		state = ST_START;
	    } else if (c == '\n' || c == -1) {
		fprintf(stderr, "Unterminated string constant\n");
		buf_clear(&buf);
		state = ST_START;		
	    } else {
		buf_push(&buf, c);
	    }
	    break;

	case ST_COMMENT_LINE:
	    if (c == '\n') state = ST_START;
	    break;

	case ST_COMMENT_BLOCK:
	    if (c == '*' && jt_peek(jt) == '/') {
		jt_next(jt);
		state = ST_START;
	    }
	    break;
	}
    }

    if (state == ST_IDENT) {
	char* lex = buf_take_cstr(&buf);
	static const char* keywords[] = {
	    "class","constructor","function","method","field","static","var","int",
	    "char","boolean","void","true","false","null","this","let","do","if",
	    "else","while","return"
	};
	JackTokenType t = IDENTIFIER;
	for (size_t i = 0; i < 21; ++i) {
	    if (strcmp(lex, keywords[i]) == 0) {
		t = KEYWORD;
		break;
	    }
	}
	add_token(out, t, lex);
	free(lex);
    } else if (state == ST_INT) {
	char* lex = buf_take_cstr(&buf);
	add_token(out, INT_CONST, lex);
	free(lex);
    } else if (state == ST_STRING) {
	fprintf(stderr, "Unterminated string constant\n");
    }

    add_token(out, _EOF, "");
    da_free(buf);
}

char symbol(const Token *t) {
    if (t->type != SYMBOL) {
	fprintf(stderr, "symbol() called on non-symbol token\n");
	exit(EXIT_FAILURE);
    }
    return t->lexeme[0];
}

const char* identifier(const Token *t) {
    if (t->type != IDENTIFIER) {
	fprintf(stderr, "identifier() called on non-identifier token\n");
	exit(EXIT_FAILURE);
    }
    return t->lexeme;
}

uint16_t int_val(const Token *t) {
    if (t->type != INT_CONST) {
	fprintf(stderr, "int_val() called on non-int token\n");
	exit(EXIT_FAILURE);
    }
    int n = atoi(t->lexeme);
    if (n < 0 || n > 32767) {
	fprintf(stderr, "int not in 0..32767\n");
	exit(EXIT_FAILURE);
    }
    return (uint16_t)n;
}

const char* string_val(const Token *t) {
    if (t->type != STRING_CONST) {
	fprintf(stderr, "string_val() called on non-string_const token\n");
	exit(EXIT_FAILURE);
    }
    return t->lexeme;
};

void tokens_free(Tokens *t) {
    if (!t) return;
    for (size_t i = 0; i < t->count; ++i) {
	free(t->items[i].lexeme);
	t->items[i].lexeme = NULL;
    }
    free(t->items);
    t->items = NULL;
    t->count = 0;
    t->capacity = 0;
}
