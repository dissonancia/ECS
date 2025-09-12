#ifndef COMPILATIONENGINE_H_
#define COMPILATIONENGINE_H_

#include "SymbolTable.h"
#include "VMWriter.h"

typedef enum {
    SUBROUTINE_FUNCTION,
    SUBROUTINE_METHOD,
    SUBROUTINE_CONSTRUCTOR
} SubroutineType;

typedef struct {
    Tokens tokens;
    size_t current_token;

    SymbolTable *symtab;
    FILE *vm_out;

    char *current_class;
    char *current_subroutine;
    SubroutineType sub_type;

    size_t label_counter;
    
    size_t current_n_locals;

    bool had_error;
} CompilationEngine;


bool compilation_engine_init(CompilationEngine *ce, Tokens tokens, const char *output_path);
void compilation_engine_free(CompilationEngine *ce);


const Token* ce_peek(CompilationEngine *ce);
const Token* ce_advance(CompilationEngine *ce);


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

#endif // COMPILATIONENGINE_H_