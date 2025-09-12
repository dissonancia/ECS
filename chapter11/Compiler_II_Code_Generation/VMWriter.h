#ifndef VMWRITER_H_
#define VMWRITER_H_

#include "JackTokenizer.h"

typedef enum {
    S_CONST,
    S_ARG,
    S_LOCAL,
    S_STATIC,
    S_THIS,
    S_THAT,
    S_POINTER,
    S_TEMP,
    S_NONE
} Segment;

typedef enum {
    C_ADD,
    C_SUB,
    C_NEG,
    C_EQ,
    C_GT,
    C_LT,
    C_AND,
    C_OR,
    C_NOT,
    C_NONE
} Command;

bool vmw_open(FILE **out, const char *path);
void vmw_close(FILE *out);

// Writes a VM push command. 
void vmw_write_push(FILE *out, Segment seg, size_t index);

// Writes a VM pop command. 
void vmw_write_pop(FILE *out, Segment seg, size_t index);

// Writes a VM arithmetic command. 
void vmw_write_arithmetic(FILE *out, Command comm);

// Writes a VM label command.
void vmw_write_label(FILE *out, const char *label);

// Writes a VM goto command.
void vmw_write_goto(FILE *out, const char *label);

// Writes a VM If-goto command.
void vmw_write_if(FILE *out, const char *label);

// Writes a VM call command. 
void vmw_write_call(FILE *out, const char *name, size_t n_args);

// Writes a VM function command. 
void vmw_write_function(FILE *out, const char *name, size_t n_locals);

// Writes a VM return command.
void vmw_write_return(FILE *out);

#endif // VMWRITER_H_