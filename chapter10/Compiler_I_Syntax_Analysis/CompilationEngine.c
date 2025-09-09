#include "CompilationEngine.h"

// --- Helpers ---

// safe peek
const Token* ce_peek(CompilationEngine *ce) {
    if (!ce) return NULL;
    if (ce->current < ce->tokens.count) {
        return &ce->tokens.items[ce->current];
    }
    return NULL;
}

// safe advance
const Token* ce_advance(CompilationEngine *ce) {
    const Token *t = ce_peek(ce);
    if (t) ce->current++;
    return t;
}

// escape xml special chars for symbols/strings
static void escape_and_fprintf(FILE *out, const char *s) {
    if (!s) return;
    for (const char *p = s; *p; ++p) {
        if (*p == '<') fputs("&lt;", out);
        else if (*p == '>') fputs("&gt;", out);
        else if (*p == '&') fputs("&amp;", out);
        else fputc(*p, out);
    }
}

// indentation helpers using ce->indent
static void ce_write_indent(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    for (int i = 0; i < ce->indent; ++i) fputc(' ', ce->out);
}

static void ce_open_tag(CompilationEngine *ce, const char *tag) {
    if (!ce || !ce->out) return;
    ce_write_indent(ce);
    fprintf(ce->out, "<%s>\n", tag);
    ce->indent += 2;
}

static void ce_close_tag(CompilationEngine *ce, const char *tag) {
    if (!ce || !ce->out) return;
    if (ce->indent >= 2) ce->indent -= 2;
    ce_write_indent(ce);
    fprintf(ce->out, "</%s>\n", tag);
}

static void ce_emit_token(CompilationEngine *ce, const char *tag, const Token *t) {
    if (!ce || !ce->out || !t) return;
    ce_write_indent(ce);
    fprintf(ce->out, "<%s> ", tag);
    escape_and_fprintf(ce->out, t->lexeme);
    fprintf(ce->out, " </%s>\n", tag);
}

static void ce_emit_keyword(CompilationEngine *ce, const Token *t) { ce_emit_token(ce, "keyword", t); }
static void ce_emit_identifier(CompilationEngine *ce, const Token *t) { ce_emit_token(ce, "identifier", t); }
static void ce_emit_symbol(CompilationEngine *ce, const Token *t) { ce_emit_token(ce, "symbol", t); }
static void ce_emit_integer(CompilationEngine *ce, const Token *t) { ce_emit_token(ce, "integerConstant", t); }
static void ce_emit_string(CompilationEngine *ce, const Token *t) { ce_emit_token(ce, "stringConstant", t); }

// predicate: single-char operator (+-*/&|<>==?)
static bool is_op_symbol(const Token *t) {
    if (!t || t->type != SYMBOL || !t->lexeme) return false;
    size_t L = strlen(t->lexeme);
    if (L != 1) return false;
    char c = t->lexeme[0];
    return (c=='+' || c=='-' || c=='*' || c=='/' || c=='&' || c=='|' || c=='<' || c=='>' || c=='=');
}

bool compilation_engine_init(CompilationEngine *ce, Tokens tokens, const char *output_path) {
    if (!ce) return false;
    FILE *out = fopen(output_path, "w");
    if (!out) return false;

    ce->tokens = tokens;
    ce->current = 0;
    ce->out = out;
    ce->output_path = output_path;
    ce->had_error = false;
    ce->indent = 0;
    return true;
}

void compilation_engine_free(CompilationEngine *ce) {
    if (!ce) return;
    if (ce->out) fclose(ce->out);
    ce->out = NULL;
}

// Forward declarations of all compile_* to allow mutual recursion
void compile_class(CompilationEngine *ce);
void compile_class_var_dec(CompilationEngine *ce);
void compile_subroutine(CompilationEngine *ce);
void compile_parameter_list(CompilationEngine *ce);
void compile_var_dec(CompilationEngine *ce);
void compile_statements(CompilationEngine *ce);
void compile_do(CompilationEngine *ce);
void compile_let(CompilationEngine *ce);
void compile_while(CompilationEngine *ce);
void compile_return(CompilationEngine *ce);
void compile_if(CompilationEngine *ce);
void compile_expression(CompilationEngine *ce);
void compile_term(CompilationEngine *ce);
void compile_expression_list(CompilationEngine *ce);

// --- Implementations ---

void compile_class(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "class");

    // class keyword
    const Token* t = ce_advance(ce);
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "class") != 0) {
        fprintf(stderr, "Expected 'class' (keyword) at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }
    ce_emit_keyword(ce, t);

    // class name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected class name (identifier) at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }
    ce_emit_identifier(ce, t);

    // { symbol
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) {
        fprintf(stderr, "Expected '{' (symbol) at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }
    ce_emit_symbol(ce, t);

    // classVarDec*
    while ((t = ce_peek(ce)) && t->type == KEYWORD &&
           (strcmp(t->lexeme, "static") == 0 || strcmp(t->lexeme, "field") == 0)) {
        compile_class_var_dec(ce);
    }

    // subroutineDec*
    while ((t = ce_peek(ce)) && t->type == KEYWORD &&
           (strcmp(t->lexeme, "constructor") == 0 ||
            strcmp(t->lexeme, "function") == 0 ||
            strcmp(t->lexeme, "method") == 0)) {
        compile_subroutine(ce);
    }

    // } symbol
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) {
        fprintf(stderr, "Expected '}' (symbol) at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "class");
}

void compile_class_var_dec(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "classVarDec");

    // (static | field) keyword
    const Token* t = ce_advance(ce);
    if (!t || t->type != KEYWORD ||
        (strcmp(t->lexeme, "static") != 0 && strcmp(t->lexeme, "field") != 0))
    {
        fprintf(stderr, "Expected 'static' or 'field' (keyword) at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }
    ce_emit_keyword(ce, t);

    // type
    t = ce_advance(ce);
    if (!t) { fprintf(stderr, "Expected a type at token %zu\n", ce->current); ce->had_error = true; return; }
    if ((t->type == KEYWORD &&
         (strcmp(t->lexeme, "int") == 0 ||
          strcmp(t->lexeme, "char") == 0 ||
          strcmp(t->lexeme, "boolean") == 0))
        || t->type == IDENTIFIER)
    {
        if (t->type == KEYWORD) ce_emit_keyword(ce, t);
        else ce_emit_identifier(ce, t);
    } else {
        fprintf(stderr, "Expected type (int, char, boolean, or className) at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }

    // var name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected var name (identifier) at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_identifier(ce, t);

    // (',' varName)*
    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        t = ce_advance(ce);
        ce_emit_symbol(ce, t);

        t = ce_advance(ce);
        if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected var name after ',' at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_identifier(ce, t);
    }

    // ;
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) {
        fprintf(stderr, "Expected ';' at end of classVarDec at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "classVarDec");
}

void compile_subroutine(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "subroutineDec");

    const Token *t;

    // ('constructor' | 'function' | 'method')
    t = ce_advance(ce);
    if (!t || t->type != KEYWORD ||
        (strcmp(t->lexeme, "constructor") != 0 &&
         strcmp(t->lexeme, "function")    != 0 &&
         strcmp(t->lexeme, "method")      != 0))
    {
        fprintf(stderr, "Expected constructor, function, or method at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }
    ce_emit_keyword(ce, t);

    // ('void' | type)
    t = ce_advance(ce);
    if (!t) { fprintf(stderr, "Expected 'void' or a type at token %zu\n", ce->current); ce->had_error = true; return; }
    if (t->type == KEYWORD && strcmp(t->lexeme, "void") == 0) {
        ce_emit_keyword(ce, t);
    } else if ((t->type == KEYWORD &&
               (strcmp(t->lexeme, "int")     == 0 ||
                strcmp(t->lexeme, "char")    == 0 ||
                strcmp(t->lexeme, "boolean") == 0)) ||
               t->type == IDENTIFIER)
    {
        if (t->type == KEYWORD) ce_emit_keyword(ce, t);
        else ce_emit_identifier(ce, t);
    } else {
        fprintf(stderr, "Expected type (int, char, boolean, or className) at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }

    // subroutineName
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected subroutine name (identifier) at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_identifier(ce, t);

    // '('
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) { fprintf(stderr, "Expected '(' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    // parameterList
    compile_parameter_list(ce);

    // ')'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    // subroutineBody
    ce_open_tag(ce, "subroutineBody");

    // '{'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) { fprintf(stderr, "Expected '{' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    // varDec*
    while ((t = ce_peek(ce)) && t->type == KEYWORD && strcmp(t->lexeme, "var") == 0) {
        compile_var_dec(ce);
    }

    // statements
    compile_statements(ce);

    // '}'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) { fprintf(stderr, "Expected '}' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "subroutineBody");
    ce_close_tag(ce, "subroutineDec");
}

void compile_parameter_list(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "parameterList");

    const Token *t = ce_peek(ce);
    if (!t) { ce_close_tag(ce, "parameterList"); return; }
    // empty list if next is ')'
    if (t->type == SYMBOL && strcmp(t->lexeme, ")") == 0) { ce_close_tag(ce, "parameterList"); return; }

    // first type
    t = ce_advance(ce);
    if (!t) { fprintf(stderr, "Expected a type in parameterList at token %zu\n", ce->current); ce->had_error = true; return; }
    if ((t->type == KEYWORD && (strcmp(t->lexeme, "int") == 0 || strcmp(t->lexeme, "char") == 0 || strcmp(t->lexeme, "boolean") == 0)) || t->type == IDENTIFIER) {
        if (t->type == KEYWORD) ce_emit_keyword(ce, t);
        else ce_emit_identifier(ce, t);
    } else { fprintf(stderr, "Expected type in parameterList at token %zu\n", ce->current); ce->had_error = true; return; }

    // var name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected var name (identifier) in parameterList at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_identifier(ce, t);

    // (',' type varName)*
    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        ce_advance(ce); // consume ','
        ce_emit_symbol(ce, t);

        t = ce_advance(ce); // type
        if (!t) { fprintf(stderr, "Expected type after ',' in parameterList at token %zu\n", ce->current); ce->had_error = true; return; }
        if ((t->type == KEYWORD && (strcmp(t->lexeme,"int")==0 || strcmp(t->lexeme,"char")==0 || strcmp(t->lexeme,"boolean")==0)) || t->type == IDENTIFIER) {
            if (t->type == KEYWORD) ce_emit_keyword(ce, t);
            else ce_emit_identifier(ce, t);
        } else { fprintf(stderr, "Expected type in parameterList at token %zu\n", ce->current); ce->had_error = true; return; }

        t = ce_advance(ce); // varName
        if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected var name after type in parameterList at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_identifier(ce, t);
    }

    ce_close_tag(ce, "parameterList");
}

void compile_var_dec(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "varDec");

    // 'var'
    const Token *t = ce_advance(ce);
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "var") != 0) { fprintf(stderr, "Expected 'var' (keyword) at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_keyword(ce, t);

    // type
    t = ce_advance(ce);
    if (!t) { fprintf(stderr, "Expected type in varDec at token %zu\n", ce->current); ce->had_error = true; return; }
    if ((t->type == KEYWORD && (strcmp(t->lexeme, "int") == 0 || strcmp(t->lexeme, "char") == 0 || strcmp(t->lexeme, "boolean") == 0)) || t->type == IDENTIFIER) {
        if (t->type == KEYWORD) ce_emit_keyword(ce, t);
        else ce_emit_identifier(ce, t);
    } else { fprintf(stderr, "Expected type in varDec at token %zu\n", ce->current); ce->had_error = true; return; }

    // var name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected var name in varDec at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_identifier(ce, t);

    // (',' varName)*
    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        t = ce_advance(ce);
        ce_emit_symbol(ce, t);

        t = ce_advance(ce);
        if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected var name after ',' in varDec at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_identifier(ce, t);
    }

    // ;
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) { fprintf(stderr, "Expected ';' at end of varDec at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "varDec");
}

void compile_statements(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "statements");

    const Token *t;
    while ((t = ce_peek(ce)) && t->type == KEYWORD) {
        if (strcmp(t->lexeme, "let") == 0) compile_let(ce);
        else if (strcmp(t->lexeme, "if") == 0) compile_if(ce);
        else if (strcmp(t->lexeme, "while") == 0) compile_while(ce);
        else if (strcmp(t->lexeme, "do") == 0) compile_do(ce);
        else if (strcmp(t->lexeme, "return") == 0) compile_return(ce);
        else break;
    }

    ce_close_tag(ce, "statements");
}

void compile_do(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "doStatement");

    const Token *t = ce_advance(ce); // do
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "do") != 0) { fprintf(stderr, "Expected 'do' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_keyword(ce, t);

    // subroutine call (two forms)
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected identifier after 'do' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_identifier(ce, t);

    const Token *next = ce_peek(ce);
    if (!next) { fprintf(stderr, "Unexpected EOF after identifier in do\n"); ce->had_error = true; return; }

    if (next->type == SYMBOL && strcmp(next->lexeme, "(") == 0) {
        ce_advance(ce); // consume '('
        ce_emit_symbol(ce, next);
        compile_expression_list(ce);
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' in doStatement at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_symbol(ce, t);
    } else if (next->type == SYMBOL && strcmp(next->lexeme, ".") == 0) {
        ce_advance(ce); // consume '.'
        ce_emit_symbol(ce, next);
        t = ce_advance(ce);
        if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected subroutine name after '.' in doStatement at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_identifier(ce, t);
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) { fprintf(stderr, "Expected '(' in doStatement at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_symbol(ce, t);
        compile_expression_list(ce);
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' in doStatement at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_symbol(ce, t);
    } else {
        fprintf(stderr, "Unexpected token after subroutine identifier in doStatement at token %zu\n", ce->current);
        ce->had_error = true;
        return;
    }

    // ';'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) { fprintf(stderr, "Expected ';' at end of doStatement at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "doStatement");
}

void compile_let(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "letStatement");

    const Token *t = ce_advance(ce); // 'let'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "let") != 0) { fprintf(stderr, "Expected 'let' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_keyword(ce, t);

    t = ce_advance(ce); // varName
    if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected varName after 'let' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_identifier(ce, t);

    const Token *next = ce_peek(ce);
    if (next && next->type == SYMBOL && strcmp(next->lexeme, "[") == 0) {
        // array access
        t = ce_advance(ce); // '['
        ce_emit_symbol(ce, t);
        compile_expression(ce);
        t = ce_advance(ce); // ']'
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "]") != 0) { fprintf(stderr, "Expected ']' in letStatement at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_symbol(ce, t);
    }

    // '='
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "=") != 0) { fprintf(stderr, "Expected '=' in letStatement at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    // expression
    compile_expression(ce);

    // ';'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) { fprintf(stderr, "Expected ';' at end of letStatement at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "letStatement");
}

void compile_while(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "whileStatement");

    const Token *t = ce_advance(ce); // 'while'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "while") != 0) { fprintf(stderr, "Expected 'while' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_keyword(ce, t);

    t = ce_advance(ce); // '('
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) { fprintf(stderr, "Expected '(' after while at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    compile_expression(ce);

    t = ce_advance(ce); // ')'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' after while expression at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    t = ce_advance(ce); // '{'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) { fprintf(stderr, "Expected '{' in whileStatement at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    compile_statements(ce);

    t = ce_advance(ce); // '}'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) { fprintf(stderr, "Expected '}' at end of whileStatement at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "whileStatement");
}

void compile_return(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "returnStatement");

    const Token *t = ce_advance(ce); // 'return'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "return") != 0) { fprintf(stderr, "Expected 'return' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_keyword(ce, t);

    const Token *next = ce_peek(ce);
    if (next && !(next->type == SYMBOL && strcmp(next->lexeme, ";") == 0)) {
        compile_expression(ce);
    }

    t = ce_advance(ce); // ';'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) { fprintf(stderr, "Expected ';' after return at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    ce_close_tag(ce, "returnStatement");
}

void compile_if(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "ifStatement");

    const Token *t = ce_advance(ce); // 'if'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "if") != 0) { fprintf(stderr, "Expected 'if' at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_keyword(ce, t);

    t = ce_advance(ce); // '('
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) { fprintf(stderr, "Expected '(' after if at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    compile_expression(ce);

    t = ce_advance(ce); // ')'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' after if expression at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    t = ce_advance(ce); // '{'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) { fprintf(stderr, "Expected '{' in ifStatement at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    compile_statements(ce);

    t = ce_advance(ce); // '}'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) { fprintf(stderr, "Expected '}' at end of ifStatement at token %zu\n", ce->current); ce->had_error = true; return; }
    ce_emit_symbol(ce, t);

    // optional else
    const Token *next = ce_peek(ce);
    if (next && next->type == KEYWORD && strcmp(next->lexeme, "else") == 0) {
        ce_advance(ce); // consume else
        ce_emit_keyword(ce, next);

        t = ce_advance(ce); // '{'
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) { fprintf(stderr, "Expected '{' after else at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_symbol(ce, t);

        compile_statements(ce);

        t = ce_advance(ce); // '}'
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) { fprintf(stderr, "Expected '}' at end of else block at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_symbol(ce, t);
    }

    ce_close_tag(ce, "ifStatement");
}

void compile_expression(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "expression");

    compile_term(ce);

    const Token *t;
    while ((t = ce_peek(ce)) && is_op_symbol(t)) {
        t = ce_advance(ce); // operator
        ce_emit_symbol(ce, t);
        compile_term(ce);
    }

    ce_close_tag(ce, "expression");
}

void compile_term(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "term");

    const Token *t = ce_peek(ce);
    if (!t) { fprintf(stderr, "Expected a term but found EOF at token %zu\n", ce->current); ce->had_error = true; return; }

    // integerConstant
    if (t->type == INT_CONST) {
        t = ce_advance(ce);
        ce_emit_integer(ce, t);
    }
    // stringConstant
    else if (t->type == STRING_CONST) {
        t = ce_advance(ce);
        ce_emit_string(ce, t);
    }
    // keywordConstant: true | false | null | this
    else if (t->type == KEYWORD && (strcmp(t->lexeme,"true")==0 || strcmp(t->lexeme,"false")==0 || strcmp(t->lexeme,"null")==0 || strcmp(t->lexeme,"this")==0)) {
        t = ce_advance(ce);
        ce_emit_keyword(ce, t);
    }
    // '(' expression ')'
    else if (t->type == SYMBOL && strcmp(t->lexeme, "(") == 0) {
        t = ce_advance(ce); // '('
        ce_emit_symbol(ce, t);
        compile_expression(ce);
        t = ce_advance(ce); // ')'
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' after expression at token %zu\n", ce->current); ce->had_error = true; return; }
        ce_emit_symbol(ce, t);
    }
    // unaryOp term: - or ~
    else if (t->type == SYMBOL && (strcmp(t->lexeme, "-")==0 || strcmp(t->lexeme, "~")==0)) {
        t = ce_advance(ce);
        ce_emit_symbol(ce, t);
        compile_term(ce);
    }
    // identifier-based: varName, varName[expression], subroutineCall, className.subroutine
    else if (t->type == IDENTIFIER) {
        // consume identifier
        const Token *name = ce_advance(ce);
        ce_emit_identifier(ce, name);

        const Token *next = ce_peek(ce);
        if (next && next->type == SYMBOL && strcmp(next->lexeme, "[") == 0) {
            // array access
            t = ce_advance(ce); // '['
            ce_emit_symbol(ce, t);
            compile_expression(ce);
            t = ce_advance(ce); // ']'
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, "]") != 0) { fprintf(stderr, "Expected ']' after array expression at token %zu\n", ce->current); ce->had_error = true; return; }
            ce_emit_symbol(ce, t);
        } else if (next && next->type == SYMBOL && strcmp(next->lexeme, "(") == 0) {
            // subroutineName '(' expressionList ')'
            t = ce_advance(ce); // '('
            ce_emit_symbol(ce, t);
            compile_expression_list(ce);
            t = ce_advance(ce); // ')'
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' after subroutine call at token %zu\n", ce->current); ce->had_error = true; return; }
            ce_emit_symbol(ce, t);
        } else if (next && next->type == SYMBOL && strcmp(next->lexeme, ".") == 0) {
            // (className|varName) '.' subroutineName '(' expressionList ')'
            t = ce_advance(ce); // '.'
            ce_emit_symbol(ce, t);
            t = ce_advance(ce); // subroutineName
            if (!t || t->type != IDENTIFIER) { fprintf(stderr, "Expected subroutine name after '.' at token %zu\n", ce->current); ce->had_error = true; return; }
            ce_emit_identifier(ce, t);
            t = ce_advance(ce); // '('
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) { fprintf(stderr, "Expected '(' after subroutine name at token %zu\n", ce->current); ce->had_error = true; return; }
            ce_emit_symbol(ce, t);
            compile_expression_list(ce);
            t = ce_advance(ce); // ')'
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) { fprintf(stderr, "Expected ')' after expressionList at token %zu\n", ce->current); ce->had_error = true; return; }
            ce_emit_symbol(ce, t);
        } else {
            // simple varName already emitted
        }
    }
    else {
        fprintf(stderr, "Unexpected token in term at token %zu: '%s'\n", ce->current, t->lexeme ? t->lexeme : "EOF");
        ce->had_error = true;
        return;
    }

    ce_close_tag(ce, "term");
}

void compile_expression_list(CompilationEngine *ce) {
    if (!ce || !ce->out) return;
    ce_open_tag(ce, "expressionList");

    const Token *t = ce_peek(ce);
    if (!t) { ce_close_tag(ce, "expressionList"); return; }
    if (t->type == SYMBOL && strcmp(t->lexeme, ")") == 0) { ce_close_tag(ce, "expressionList"); return; }

    // at least one expression
    compile_expression(ce);

    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        t = ce_advance(ce); // ','
        ce_emit_symbol(ce, t);
        compile_expression(ce);
    }

    ce_close_tag(ce, "expressionList");
}