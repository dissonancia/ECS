#ifndef COMPILATIONENGINE_H_
#define COMPILATIONENGINE_H_

#include "SymbolTable.h"
#include "VMWriter.h"

typedef enum {
    SUBROUTINE_FUNCTION,
    SUBROUTINE_METHOD,
    SUBROUTINE_CONSTRUCTOR
} SubroutineType;

#define FQ_NAME_BUF_SIZE 256

typedef struct {
    FILE *vm_out;
    Tokens tokens;
    size_t current_token;
    SymbolTable *symtab;
    char *current_class;
    char fq_name[FQ_NAME_BUF_SIZE];
    size_t label_counter;
    bool had_error;
} CompilationEngine;


bool compilation_engine_init(CompilationEngine *ce, Tokens tokens, const char *output_path);
void compilation_engine_free(CompilationEngine *ce);

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