#include "CompilationEngine.h"

// --- Helpers ---

// safe peek
const Token *ce_peek(CompilationEngine *ce) {
    if (!ce) return NULL;
    if (ce->current_token < ce->tokens.count)
        return &ce->tokens.items[ce->current_token];
    return NULL;
}

// safe advance
const Token *ce_advance(CompilationEngine *ce) {
    const Token *t = ce_peek(ce);
    if (t) ce->current_token++;
    return t;
}

// predicate: single-char operator (+-&|<>==?)
static bool is_op_symbol(const Token *t) {
    if (!t || t->type != SYMBOL || !t->lexeme)
        return false;
    size_t L = strlen(t->lexeme);
    if (L != 1) return false;
    char c = t->lexeme[0];
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '<' || c == '>' || c == '=');
}

static Segment seg_from_kind(Kind k) {
    switch (k) {
        case STATIC: return S_STATIC;
        case FIELD:  return S_THIS;
        case ARG:    return S_ARG;
        case VAR:    return S_LOCAL;
        default:     return S_NONE;
    }
}

bool compilation_engine_init(CompilationEngine *ce, Tokens tokens, const char *output_path) {
    if (!ce) return false;
    FILE *out = fopen(output_path, "w");
    if (!out) return false;

    ce->tokens        = tokens;
    ce->current_token = 0;
    ce->vm_out        = out;
    ce->had_error     = false;
    ce->label_counter = 0; 
    return true;
}

void compilation_engine_free(CompilationEngine *ce) {
    if (!ce) return;
    if (ce->vm_out) fclose(ce->vm_out);
    ce->vm_out = NULL;
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
size_t compile_expression_list(CompilationEngine *ce);

// --- Implementations ---

void compile_class(CompilationEngine *ce) {
    if (!ce || !ce->vm_out) return;

    // class keyword
    const Token *t = ce_advance(ce);
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "class") != 0) {
        fprintf(stderr, "Expected 'class' (keyword) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // class name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected class name (identifier) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    free(ce->current_class);
    ce->current_class = strdup((const char *)t->lexeme);

    // { symbol
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) {
        fprintf(stderr, "Expected '{' (symbol) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // classVarDec*
    while ((t = ce_peek(ce)) && t->type == KEYWORD &&
           (strcmp(t->lexeme, "static") == 0 || strcmp(t->lexeme, "field") == 0))
    {
        compile_class_var_dec(ce);
        if (ce->had_error) return;
    }

    // subroutineDec*
    while ((t = ce_peek(ce)) && t->type == KEYWORD &&
           (strcmp(t->lexeme, "constructor") == 0 ||
            strcmp(t->lexeme, "function") == 0 ||
            strcmp(t->lexeme, "method") == 0))
    {
        compile_subroutine(ce);
        if (ce->had_error) return;
    }

    // } symbol
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) {
        fprintf(stderr, "Expected '}' (symbol) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    free(ce->current_class);
    ce->current_class = NULL;
}

void compile_class_var_dec(CompilationEngine *ce) {
    if (!ce || !ce->vm_out) return;

    // (static | field) keyword
    const Token *t = ce_advance(ce);
    if (!t || t->type != KEYWORD ||
        (strcmp(t->lexeme, "static") != 0 && strcmp(t->lexeme, "field") != 0))
    {
        fprintf(stderr, "Expected 'static' or 'field' (keyword) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    Kind kind;
    if (strcmp(t->lexeme, "static") == 0) kind = STATIC;
    else kind = FIELD;

    // type
    t = ce_advance(ce);
    if (!t) {
        fprintf(stderr, "Expected a type at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    if (!((t->type == KEYWORD &&
           (strcmp(t->lexeme, "int") == 0 ||
            strcmp(t->lexeme, "char") == 0 ||
            strcmp(t->lexeme, "boolean") == 0)) ||
          t->type == IDENTIFIER))
    {
        fprintf(stderr, "Expected type (int, char, boolean, or className) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    const char *type_name = (const char *)t->lexeme;

    // var name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected var name (identifier) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    // define the first variable
    st_define(ce->symtab, (const char *)t->lexeme, type_name, kind);

    // (',' varName)*
    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        t = ce_advance(ce);

        t = ce_advance(ce);
        if (!t || t->type != IDENTIFIER) {
            fprintf(stderr, "Expected var name after ',' at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        st_define(ce->symtab, (const char *)t->lexeme, type_name, kind);
    }

    // ;
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) {
        fprintf(stderr, "Expected ';' at end of classVarDec at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
}

void compile_subroutine(CompilationEngine *ce) {
    if (!ce || !ce->vm_out) return;

    // ('constructor' | 'function' | 'method')
    const Token *t = ce_advance(ce);
    if (!t || t->type != KEYWORD ||
        (strcmp(t->lexeme, "constructor") != 0 &&
         strcmp(t->lexeme, "function") != 0 &&
         strcmp(t->lexeme, "method") != 0))
    {
        fprintf(stderr, "Expected constructor, function, or method at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // set subtypes flags
    if (strcmp(t->lexeme, "constructor") == 0)
        ce->sub_type = SUBROUTINE_CONSTRUCTOR;
    else if (strcmp(t->lexeme, "function") == 0)
        ce->sub_type = SUBROUTINE_FUNCTION;
    else
        ce->sub_type = SUBROUTINE_METHOD;

    // ('void' | type)
    t = ce_advance(ce);
    if (!t) {
        fprintf(stderr, "Expected 'void' or a type at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // subroutineName
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected subroutine name (identifier) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    free(ce->current_subroutine);
    ce->current_subroutine = strdup((const char *)t->lexeme);

    // '('
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) {
        fprintf(stderr, "Expected '(' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // starts subroutine scope
    st_start_subroutine(ce->symtab);

    // If method, define 'this' as ARG 0 in subroutine scope before parameter list
    if (ce->sub_type == SUBROUTINE_METHOD) {
        if (!ce->current_class) {
            fprintf(stderr, "Internal error: current_class is NULL at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        st_define(ce->symtab, "this", ce->current_class, ARG);
    }

    // parameterList: should call st_define for each parameter
    compile_parameter_list(ce);
    if (ce->had_error) return;

    // ')'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
        fprintf(stderr, "Expected ')' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // subroutineBody
    // '{'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) {
        fprintf(stderr, "Expected '{' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // varDec* (each varDec must call st_define(..., VAR))
    while ((t = ce_peek(ce)) && t->type == KEYWORD && strcmp(t->lexeme, "var") == 0) {
        compile_var_dec(ce);
        if (ce->had_error) return;
    }

    // Now we know how many local variables there are
    ce->current_n_locals = st_var_count(ce->symtab, VAR);

    // Emit VM function header: ClassName.subName nLocals
    if (!ce->current_class) {
        fprintf(stderr, "Internal error: current_class is NULL at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    size_t sz = strlen(ce->current_class) + strlen(ce->current_subroutine) + 3;
    char *temp = malloc(sz);
    if (!temp) { perror("malloc"); ce->had_error = true; return; }
    snprintf(temp, sz, "%s.%s", ce->current_class, ce->current_subroutine);
    vmw_write_function(ce->vm_out, (const char *)temp, ce->current_n_locals);
    free(temp);

    // Constructor: allocate memory and set THIS = allocated base
    if (ce->sub_type == SUBROUTINE_CONSTRUCTOR) {
        size_t n_fields = st_var_count(ce->symtab, FIELD); // class-scope
        vmw_write_push(ce->vm_out, S_CONST, n_fields);
        vmw_write_call(ce->vm_out, "Memory.alloc", 1);
        vmw_write_pop(ce->vm_out, S_POINTER, 0); // pointer 0 = THIS
    }

    // Method: set THIS to argument 0
    if (ce->sub_type == SUBROUTINE_METHOD) {
        vmw_write_push(ce->vm_out, S_ARG, 0);
        vmw_write_pop(ce->vm_out, S_POINTER, 0);
    }

    // compile statements inside the body (let/do/if/while/return)
    compile_statements(ce);
    if (ce->had_error) return;

    // '}'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) {
        fprintf(stderr, "Expected '}' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
}

void compile_parameter_list(CompilationEngine *ce) {
    if (!ce || !ce->vm_out) return;

    const Token *t = ce_peek(ce);
    if (!t) return;

    // empty list if next is ')'
    if (t->type == SYMBOL && strcmp(t->lexeme, ")") == 0)
        return;

    // first type
    const char *type_name;
    t = ce_advance(ce);
    if (!t) {
        fprintf(stderr, "Expected a type in parameterList at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    if ((t->type == KEYWORD && (strcmp(t->lexeme, "int") == 0 || strcmp(t->lexeme, "char") == 0 || strcmp(t->lexeme, "boolean") == 0)) || t->type == IDENTIFIER)
        type_name = (const char *)t->lexeme;
    else {
        fprintf(stderr, "Expected type in parameterList at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // var name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected var name (identifier) in parameterList at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    st_define(ce->symtab, (const char *)t->lexeme, type_name, ARG);

    // (',' type varName)*
    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        ce_advance(ce); // consume ','

        t = ce_advance(ce); // type
        if (!t) {
            fprintf(stderr, "Expected type after ',' in parameterList at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        if ((t->type == KEYWORD && (strcmp(t->lexeme, "int") == 0 || strcmp(t->lexeme, "char") == 0 || strcmp(t->lexeme, "boolean") == 0)) || t->type == IDENTIFIER)
            type_name = (const char *)t->lexeme;
        else {
            fprintf(stderr, "Expected type in parameterList at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        t = ce_advance(ce); // varName
        if (!t || t->type != IDENTIFIER) {
            fprintf(stderr, "Expected var name after type in parameterList at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        st_define(ce->symtab, (const char *)t->lexeme, type_name, ARG);
    }
}

void compile_var_dec(CompilationEngine *ce) {
    if (!ce || !ce->vm_out) return;

    // 'var'
    const Token *t = ce_advance(ce);
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "var") != 0) {
        fprintf(stderr, "Expected 'var' (keyword) at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // type
    t = ce_advance(ce);
    const char *type_name;
    if (!t) {
        fprintf(stderr, "Expected type in varDec at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    if ((t->type == KEYWORD && (strcmp(t->lexeme, "int") == 0 || strcmp(t->lexeme, "char") == 0 || strcmp(t->lexeme, "boolean") == 0)) || t->type == IDENTIFIER)
        type_name = (const char *)t->lexeme;
    else {
        fprintf(stderr, "Expected type in varDec at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // var name
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected var name in varDec at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    st_define(ce->symtab, (const char *)t->lexeme, type_name, VAR);

    // (',' varName)*
    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        t = ce_advance(ce); // consume ','

        t = ce_advance(ce);
        if (!t || t->type != IDENTIFIER) {
            fprintf(stderr, "Expected var name after ',' in varDec at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        st_define(ce->symtab, (const char *)t->lexeme, type_name, VAR);
    }

    // ;
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) {
        fprintf(stderr, "Expected ';' at end of varDec at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
}

void compile_statements(CompilationEngine *ce) {
    if (!ce || !ce->vm_out) return;

    const Token *t;
    while ((t = ce_peek(ce)) && t->type == KEYWORD)
    {
        if (ce->had_error) return;

        if (strcmp(t->lexeme, "let") == 0)
            compile_let(ce);
        else if (strcmp(t->lexeme, "if") == 0)
            compile_if(ce);
        else if (strcmp(t->lexeme, "while") == 0)
            compile_while(ce);
        else if (strcmp(t->lexeme, "do") == 0)
            compile_do(ce);
        else if (strcmp(t->lexeme, "return") == 0)
            compile_return(ce);
        else
            break;
    }
    if (ce->had_error) return;
}

void compile_do(CompilationEngine *ce) {
    if (!ce || !ce->vm_out) return;

    const Token *t = ce_advance(ce); // 'do'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "do") != 0) {
        fprintf(stderr, "Expected 'do' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // first identifier (qualifier or subroutine name)
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected identifier after 'do' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    const char *first = (const char*)t->lexeme;

    const Token *next = ce_peek(ce);
    if (!next) {
        fprintf(stderr, "Unexpected EOF after identifier in do\n");
        ce->had_error = true;
        return;
    }

    if (next->type == SYMBOL && strcmp(next->lexeme, "(") == 0) {
        // unqualified call: first(...)  => method of current class 
        ce_advance(ce); // consume '('

        // push current object as receiver
        vmw_write_push(ce->vm_out, S_POINTER, 0);

        // compile args
        size_t expr_count = compile_expression_list(ce);
        if (ce->had_error) return;

        // expect ')'
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
            fprintf(stderr, "Expected ')' in doStatement at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        // build target: CurrentClass.first
        if (!ce->current_class) {
            fprintf(stderr, "Internal error: current_class is NULL at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        size_t size_needed = strlen(ce->current_class) + strlen(first) + 2;
        char *target = malloc(size_needed);
        if (!target) { perror("malloc"); ce->had_error = true; return; }
        snprintf(target, size_needed, "%s.%s", ce->current_class, first);

        vmw_write_call(ce->vm_out, target, (int)(expr_count + 1)); // +1 for receiver
        vmw_write_pop(ce->vm_out, S_TEMP, 0); // discard return

        free(target);
    }
    else if (next->type == SYMBOL && strcmp(next->lexeme, ".") == 0) {
        // qualified call: qualifier.method(...)
        ce_advance(ce); // consume '.'

        // read method name
        t = ce_advance(ce);
        if (!t || t->type != IDENTIFIER) {
            fprintf(stderr, "Expected subroutine name after '.' in doStatement at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        const char *method = (const char*)t->lexeme;

        // expect '('
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) {
            fprintf(stderr, "Expected '(' in doStatement at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        // Decide variable vs class:
        //   - if first exists in symbol table => variable (instance) case
        //   - else => class name (static or function) case
        Kind kval = st_kind_of(ce->symtab, first);
        if (kval != NONE) {
            // variable: push receiver before compiling args
            int seg = seg_from_kind(kval);
            if (seg == S_NONE) {
                fprintf(stderr, "Invalid kind for '%s' at token %zu\n", first, ce->current_token);
                ce->had_error = true;
                return;
            }
            size_t idx = st_index_of(ce->symtab, first);

            const char *type = st_type_of(ce->symtab, first); // class name string
            vmw_write_push(ce->vm_out, (Segment)seg, idx);

            // compile arguments
            size_t expr_count = compile_expression_list(ce);
            if (ce->had_error) return;

            // expect ')'
            t = ce_advance(ce);
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
                fprintf(stderr, "Expected ')' in doStatement at token %zu\n", ce->current_token);
                ce->had_error = true;
                return;
            }

            // build target: <type>.<method>
            size_t size_needed = strlen(type) + strlen(method) + 2;
            char *target = malloc(size_needed);
            if (!target) { perror("malloc"); ce->had_error = true; return; }
            snprintf(target, size_needed, "%s.%s", type, method);

            vmw_write_call(ce->vm_out, target, (int)(expr_count + 1)); // +1 for receiver
            vmw_write_pop(ce->vm_out, S_TEMP, 0); // discard return
            free(target);
        } else {
            // class name: no receiver push
            size_t expr_count = compile_expression_list(ce);
            if (ce->had_error) return;

            // expect ')'
            t = ce_advance(ce);
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
                fprintf(stderr, "Expected ')' in doStatement at token %zu\n", ce->current_token);
                ce->had_error = true;
                return;
            }

            // build target: <first>.<method>
            size_t size_needed = strlen(first) + strlen(method) + 2;
            char *target = malloc(size_needed);
            if (!target) { perror("malloc"); ce->had_error = true; return; }
            snprintf(target, size_needed, "%s.%s", first, method);

            vmw_write_call(ce->vm_out, target, (int)expr_count);
            vmw_write_pop(ce->vm_out, S_TEMP, 0); // discard return
            free(target);
        }
    }
    else {
        fprintf(stderr, "Unexpected token after subroutine identifier in doStatement at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // expect terminating ';'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) {
        fprintf(stderr, "Expected ';' at end of doStatement at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
}

void compile_let(CompilationEngine *ce)
{
    if (!ce || !ce->vm_out) return;

    const Token *t = ce_advance(ce); // 'let'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "let") != 0) {
        fprintf(stderr, "Expected 'let' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // varName
    t = ce_advance(ce);
    if (!t || t->type != IDENTIFIER) {
        fprintf(stderr, "Expected varName after 'let' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
    const char *var_name = (const char *)t->lexeme;

    // lookup
    Kind kval = st_kind_of(ce->symtab, var_name);
    if (kval == NONE) {
        fprintf(stderr, "Unknown variable '%s' at token %zu\n", var_name, ce->current_token);
        ce->had_error = true;
        return;
    }
    Segment seg = seg_from_kind(kval);
    if (seg == S_NONE) {
        fprintf(stderr, "Invalid kind for '%s' at token %zu\n", var_name, ce->current_token);
        ce->had_error = true;
        return;
    }
    int idx_i = st_index_of(ce->symtab, var_name);
    if (idx_i < 0) {
        fprintf(stderr, "Invalid index for '%s' at token %zu\n", var_name, ce->current_token);
        ce->had_error = true;
        return;
    }
    size_t idx = (size_t)idx_i;

    // check for array access: varName '[' expression ']' '=' expression
    const Token *next = ce_peek(ce);
    if (next && next->type == SYMBOL && strcmp(next->lexeme, "[") == 0) {
        // consume '['
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "[") != 0) {
            fprintf(stderr, "Expected '[' in letStatement at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        // compile index expression first (pushes index)
        compile_expression(ce);
        if (ce->had_error) return;

        // expect ']'
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "]") != 0) {
            fprintf(stderr, "Expected ']' in letStatement at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        // now push base address (array reference stored in variable)
        vmw_write_push(ce->vm_out, seg, idx);

        // compute effective address = index + base
        vmw_write_arithmetic(ce->vm_out, C_ADD);

        // expect '=' next
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "=") != 0) {
            fprintf(stderr, "Expected '=' in letStatement at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        // compile RHS expression (pushes value)
        compile_expression(ce);
        if (ce->had_error) return;

        // store: temp0 = value; pointer 1 = address; that 0 = value
        vmw_write_pop(ce->vm_out, S_TEMP, 0);
        vmw_write_pop(ce->vm_out, S_POINTER, 1);
        vmw_write_push(ce->vm_out, S_TEMP, 0);
        vmw_write_pop(ce->vm_out, S_THAT, 0);
    }
    else {
        // simple assignment: varName = expression
        // expect '='
        t = ce_advance(ce);
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "=") != 0) {
            fprintf(stderr, "Expected '=' in letStatement at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        // compile RHS value (pushes value)
        compile_expression(ce);
        if (ce->had_error) return;

        // pop into variable's segment/index
        vmw_write_pop(ce->vm_out, seg, idx);
    }

    // expect ';'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) {
        fprintf(stderr, "Expected ';' at end of letStatement at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
}

void compile_while(CompilationEngine *ce)
{
    if (!ce || !ce->vm_out) return;

    const Token *t = ce_advance(ce); // 'while'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "while") != 0) {
        fprintf(stderr, "Expected 'while' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    char exp_label[64], end_label[64];
    snprintf(exp_label, sizeof(exp_label), "%s_%zu", ce->current_class, ce->label_counter++);
    snprintf(end_label, sizeof(end_label), "%s_%zu", ce->current_class, ce->label_counter++);

    // start label 
    vmw_write_label(ce->vm_out, exp_label);

    // expect '(' 
    t = ce_advance(ce); // '('
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) {
        fprintf(stderr, "Expected '(' after while at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // compile condition (leaves value on stack) 
    compile_expression(ce);
    if (ce->had_error) return;

    // invert and if false -> jump to end 
    vmw_write_arithmetic(ce->vm_out, C_NOT);
    vmw_write_if(ce->vm_out, end_label);

    // expect ')' 
    t = ce_advance(ce); // ')'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
        fprintf(stderr, "Expected ')' after while expression at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // expect '{' 
    t = ce_advance(ce); // '{'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) {
        fprintf(stderr, "Expected '{' in whileStatement at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // body 
    compile_statements(ce);
    if (ce->had_error) return;

    // jump back to start and close with end label 
    vmw_write_goto(ce->vm_out, exp_label);
    vmw_write_label(ce->vm_out, end_label);

    // expect closing '}' 
    t = ce_advance(ce); // '}'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) {
        fprintf(stderr, "Expected '}' at end of whileStatement at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
}

void compile_return(CompilationEngine *ce)
{
    if (!ce || !ce->vm_out) return;

    const Token *t = ce_advance(ce); // 'return'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "return") != 0) {
        fprintf(stderr, "Expected 'return' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    const Token *next = ce_peek(ce);
    if (!next) {
        fprintf(stderr, "Unexpected EOF after 'return' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    if (!(next->type == SYMBOL && strcmp(next->lexeme, ";") == 0)) {
        // there is an expression to return

        // leaves value on stack
        compile_expression(ce);
        if (ce->had_error) return;
        vmw_write_return(ce->vm_out);
    } else {
        // void return -> push 0 then return
        vmw_write_push(ce->vm_out, S_CONST, 0);
        vmw_write_return(ce->vm_out);
    }

    // consume the terminating ';'
    t = ce_advance(ce);
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ";") != 0) {
        fprintf(stderr, "Expected ';' after return at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }
}

void compile_if(CompilationEngine *ce)
{
    if (!ce || !ce->vm_out) return;

    const Token *t = ce_advance(ce); // 'if'
    if (!t || t->type != KEYWORD || strcmp(t->lexeme, "if") != 0) {
        fprintf(stderr, "Expected 'if' at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    t = ce_advance(ce); // '('
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) {
        fprintf(stderr, "Expected '(' after if at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    char if_false[64], if_end[64];
    snprintf(if_false, sizeof(if_false), "%s_%zu", ce->current_class, ce->label_counter++);
    snprintf(if_end, sizeof(if_end), "%s_%zu", ce->current_class, ce->label_counter++);

    compile_expression(ce);
    if (ce->had_error) return;

    t = ce_advance(ce); // ')'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
        fprintf(stderr, "Expected ')' after if expression at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // invert and if false -> jump to end 
    vmw_write_arithmetic(ce->vm_out, C_NOT);
    vmw_write_if(ce->vm_out, if_false);

    t = ce_advance(ce); // '{'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) {
        fprintf(stderr, "Expected '{' in ifStatement at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    compile_statements(ce);
    if (ce->had_error) return;

    t = ce_advance(ce); // '}'
    if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) {
        fprintf(stderr, "Expected '}' at end of ifStatement at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // optional else
    const Token *next = ce_peek(ce);
    if (next && next->type == KEYWORD && strcmp(next->lexeme, "else") == 0) {
        ce_advance(ce); // consume else

        vmw_write_goto(ce->vm_out, if_end);
        vmw_write_label(ce->vm_out, if_false);

        t = ce_advance(ce); // '{'
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "{") != 0) {
            fprintf(stderr, "Expected '{' after else at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }

        compile_statements(ce);
        if (ce->had_error) return;

        vmw_write_label(ce->vm_out, if_end);

        t = ce_advance(ce); // '}'
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, "}") != 0) {
            fprintf(stderr, "Expected '}' at end of else block at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
    } else {
        vmw_write_label(ce->vm_out, if_false);
    }
}

void compile_expression(CompilationEngine *ce)
{
    if (!ce || !ce->vm_out) return;

    // left operand 
    compile_term(ce);
    if (ce->had_error) return;

    const Token *t;
    while ((t = ce_peek(ce)) && is_op_symbol(t)) {
        // operator token (advance it now) 
        t = ce_advance(ce);
        char op = t->lexeme[0];

        // compile right operand first so stack has: left right 
        compile_term(ce);
        if (ce->had_error) return;

        // then emit the operation that consumes the two operands and pushes result 
        switch (op) {
            case '+': vmw_write_arithmetic(ce->vm_out, C_ADD); break;
            case '-': vmw_write_arithmetic(ce->vm_out, C_SUB); break;
            case '&': vmw_write_arithmetic(ce->vm_out, C_AND); break;
            case '|': vmw_write_arithmetic(ce->vm_out,  C_OR); break;
            case '<': vmw_write_arithmetic(ce->vm_out,  C_LT); break;
            case '>': vmw_write_arithmetic(ce->vm_out,  C_GT); break;
            case '=': vmw_write_arithmetic(ce->vm_out,  C_EQ); break;
            case '*':
                vmw_write_call(ce->vm_out, "Math.multiply", 2);
                break;
            case '/':
                vmw_write_call(ce->vm_out, "Math.divide", 2);
                break;
            default:
                // shouldn't happen because is_op_symbol filtered 
                fprintf(stderr, "Unknown operator '%c' at token %zu\n", op, ce->current_token);
                ce->had_error = true;
                return;
        }
    }
}

void compile_term(CompilationEngine *ce)
{
    if (!ce || !ce->vm_out) return;

    const Token *t = ce_peek(ce);
    if (!t) {
        fprintf(stderr, "Expected a term but found EOF at token %zu\n", ce->current_token);
        ce->had_error = true;
        return;
    }

    // integerConstant 
    if (t->type == INT_CONST) {
        t = ce_advance(ce);
        int val = atoi((const char*)t->lexeme);
        vmw_write_push(ce->vm_out, S_CONST, val);
        return;
    }

    // stringConstant 
    if (t->type == STRING_CONST) {
        t = ce_advance(ce);
        const char *s = (const char*)t->lexeme;
        size_t len = strlen(s);

        // create string object 
        vmw_write_push(ce->vm_out, S_CONST, (int)len);
        vmw_write_call(ce->vm_out, "String.new", 1);

        // append chars 
        for (size_t i = 0; i < len; ++i) {
            unsigned char ch = (unsigned char)s[i];
            vmw_write_push(ce->vm_out, S_CONST, (int)ch);
            vmw_write_call(ce->vm_out, "String.appendChar", 2);
        }
        return;
    }

    // keywordConstant: true | false | null | this 
    if (t->type == KEYWORD &&
        (strcmp(t->lexeme, "true") == 0 ||
         strcmp(t->lexeme, "false") == 0 ||
         strcmp(t->lexeme, "null") == 0 ||
         strcmp(t->lexeme, "this") == 0))
    {
        t = ce_advance(ce);
        if (strcmp(t->lexeme, "false") == 0 || strcmp(t->lexeme, "null") == 0) {
            vmw_write_push(ce->vm_out, S_CONST, 0);
        } else if (strcmp(t->lexeme, "true") == 0) {
            vmw_write_push(ce->vm_out, S_CONST, 1);
            vmw_write_arithmetic(ce->vm_out, C_NEG); // yields -1 
        } else { // "this" 
            vmw_write_push(ce->vm_out, S_POINTER, 0);
        }
        return;
    }

    // '(' expression ')' 
    if (t->type == SYMBOL && strcmp(t->lexeme, "(") == 0) {
        ce_advance(ce); // consume '(' 
        compile_expression(ce);
        if (ce->had_error) return;
        t = ce_advance(ce); // expect ')' 
        if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
            fprintf(stderr, "Expected ')' after expression at token %zu\n", ce->current_token);
            ce->had_error = true;
            return;
        }
        return;
    }

    // unaryOp term: - or ~ 
    if (t->type == SYMBOL && (strcmp(t->lexeme, "-") == 0 || strcmp(t->lexeme, "~") == 0)) {
        t = ce_advance(ce); // consume unary 
        compile_term(ce);   // compile operand 
        if (ce->had_error) return;
        if (strcmp(t->lexeme, "-") == 0)
            vmw_write_arithmetic(ce->vm_out, C_NEG);
        else
            vmw_write_arithmetic(ce->vm_out, C_NOT);
        return;
    }

    // identifier-based: varName, varName[expression], subroutineCall, className.subroutine 
    if (t->type == IDENTIFIER) {
        const Token *name_tok = ce_advance(ce); // consume identifier 
        const char *name = (const char *)name_tok->lexeme;

        const Token *next = ce_peek(ce);
        // array access: name[expr] 
        if (next && next->type == SYMBOL && strcmp(next->lexeme, "[") == 0) {
            ce_advance(ce); // consume '['

            compile_expression(ce);
            if (ce->had_error) return;

            Kind kval = st_kind_of(ce->symtab, name);
            if (kval == NONE) {
                fprintf(stderr, "Unknown variable '%s' at token %zu\n", name, ce->current_token);
                ce->had_error = true;
                return;
            }
            int seg = seg_from_kind(kval);
            if (seg == S_NONE) {
                fprintf(stderr, "Invalid kind for '%s' at token %zu\n", name, ce->current_token);
                ce->had_error = true;
                return;
            }
            size_t idx = st_index_of(ce->symtab, name);
            vmw_write_push(ce->vm_out, seg, idx);

            vmw_write_arithmetic(ce->vm_out, C_ADD);

            vmw_write_pop(ce->vm_out, S_POINTER, 1);
            vmw_write_push(ce->vm_out, S_THAT, 0);

            t = ce_advance(ce); // expect ']' 
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, "]") != 0) {
                fprintf(stderr, "Expected ']' after array expression at token %zu\n", ce->current_token);
                ce->had_error = true;
                return;
            }
            return;
        }

        // subroutine call without qualifier: name( ... )  => method of current class 
        if (next && next->type == SYMBOL && strcmp(next->lexeme, "(") == 0) {
            ce_advance(ce); // consume '(' 

            // push receiver 
            vmw_write_push(ce->vm_out, S_POINTER, 0);

            size_t expr_count = compile_expression_list(ce);
            if (ce->had_error) return;

            t = ce_advance(ce); // expect ')' 
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
                fprintf(stderr, "Expected ')' after subroutine call at token %zu\n", ce->current_token);
                ce->had_error = true;
                return;
            }

            // build target CurrentClass.name
            if (!ce->current_class) {
                fprintf(stderr, "Internal error: current_class is NULL at token %zu\n", ce->current_token);
                ce->had_error = true;
                return;
            }
            size_t size_needed = strlen(ce->current_class) + strlen(name) + 2;
            char *target = malloc(size_needed);
            if (!target) { perror("malloc"); ce->had_error = true; return; }
            snprintf(target, size_needed, "%s.%s", ce->current_class, name);

            vmw_write_call(ce->vm_out, target, (expr_count + 1)); // +1 receiver 
            free(target);
            return;
        }

        // qualified call or className.method 
        if (next && next->type == SYMBOL && strcmp(next->lexeme, ".") == 0) {
            ce_advance(ce); // consume '.' 

            // read method name 
            t = ce_advance(ce);
            if (!t || t->type != IDENTIFIER) {
                fprintf(stderr, "Expected subroutine name after '.' at token %zu\n", ce->current_token);
                ce->had_error = true;
                return;
            }
            const char *method = (const char*)t->lexeme;

            // expect '(' 
            t = ce_advance(ce);
            if (!t || t->type != SYMBOL || strcmp(t->lexeme, "(") != 0) {
                fprintf(stderr, "Expected '(' after subroutine name at token %zu\n", ce->current_token);
                ce->had_error = true;
                return;
            }

            Kind kval = st_kind_of(ce->symtab, name);
            if (kval != NONE) {
                // variable: push receiver, compile args, call type.method (expr_count+1) 
                int seg = seg_from_kind(kval);
                if (seg == S_NONE) {
                    fprintf(stderr, "Invalid kind for '%s' at token %zu\n", name, ce->current_token);
                    ce->had_error = true;
                    return;
                }
                size_t idx = st_index_of(ce->symtab, name);
                const char *type = st_type_of(ce->symtab, name);

                vmw_write_push(ce->vm_out, seg, idx);

                size_t expr_count = compile_expression_list(ce);
                if (ce->had_error) return;

                t = ce_advance(ce); // expect ')' 
                if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
                    fprintf(stderr, "Expected ')' after subroutine call at token %zu\n", ce->current_token);
                    ce->had_error = true;
                    return;
                }

                size_t size_needed = strlen(type) + strlen(method) + 2;
                char *target = malloc(size_needed);
                if (!target) { perror("malloc"); ce->had_error = true; return; }
                snprintf(target, size_needed, "%s.%s", type, method);

                vmw_write_call(ce->vm_out, target, (int)(expr_count + 1)); // +1 receiver 
                free(target);
                return;
            } else {
                // class name: compile args and call ClassName.method(expr_count) 
                size_t expr_count = compile_expression_list(ce);
                if (ce->had_error) return;

                t = ce_advance(ce); // expect ')' 
                if (!t || t->type != SYMBOL || strcmp(t->lexeme, ")") != 0) {
                    fprintf(stderr, "Expected ')' after subroutine call at token %zu\n", ce->current_token);
                    ce->had_error = true;
                    return;
                }

                size_t size_needed = strlen(name) + strlen(method) + 2;
                char *target = malloc(size_needed);
                if (!target) { perror("malloc"); ce->had_error = true; return; }
                snprintf(target, size_needed, "%s.%s", name, method);

                vmw_write_call(ce->vm_out, target, (int)expr_count);
                free(target);
                return;
            }
        }

        // simple variable access: push seg idx 
        {
            Kind kval = st_kind_of(ce->symtab, name);
            if (kval == NONE) {
                fprintf(stderr, "Unknown variable '%s' at token %zu\n", name, ce->current_token);
                ce->had_error = true;
                return;
            }
            int seg = seg_from_kind(kval);
            if (seg == S_NONE) {
                fprintf(stderr, "Invalid kind for '%s' at token %zu\n", name, ce->current_token);
                ce->had_error = true;
                return;
            }
            size_t idx = st_index_of(ce->symtab, name);
            vmw_write_push(ce->vm_out, seg, idx);
            return;
        }
    }

    // else: unexpected token 
    fprintf(stderr, "Unexpected token in term at token %zu: '%s'\n",
            ce->current_token, t->lexeme ? t->lexeme : "EOF");
    ce->had_error = true;
}

size_t compile_expression_list(CompilationEngine *ce) {
    if (!ce) return 0;

    const Token *t = ce_peek(ce);
    if (!t) return 0;
    // empty list if next is ')'
    if (t->type == SYMBOL && strcmp(t->lexeme, ")") == 0) return 0;

    size_t count = 0;

    // at least one expression
    compile_expression(ce);
    count++;

    while ((t = ce_peek(ce)) && t->type == SYMBOL && strcmp(t->lexeme, ",") == 0) {
        ce_advance(ce); // consume ','
        compile_expression(ce);
        count++;
    }

    return count;
}