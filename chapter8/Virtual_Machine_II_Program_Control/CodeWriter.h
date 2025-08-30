#ifndef CODEWRITER_H_
#define CODEWRITER_H_

#include "Parser.h"

typedef struct {
    FILE *out;                 // .asm output
    char file_name[MAX_PATH];  // base name of the current file (.vm)
    char current_function[64]; // current function
    int label_counter;         // counter to generate unique labels (e.g. eq_true1, eq_true2)
} Code_Writer;

// Initialize and open the output file (.asm)
bool code_writer_init(Code_Writer *cw, const char *output_path, bool with_bootstrap);

// Informs that it has started translating a new VM file
void set_file_name(Code_Writer *cw, const char *file_name);

// Write code for arithmetic commands (add, sub, neg, eq, gt, lt, and, or, not)
void write_arithmetic(Code_Writer *cw, String_View command);

// Write code for push/pop
void write_push_pop(Code_Writer *cw,
                    Command_Type command,
                    String_View segment,
                    int index);

// Writes assembly code that effects the label command.
void write_label(Code_Writer *cw, String_View label);

// Writes assembly code that effects the goto command.
void write_goto(Code_Writer *cw, String_View label);

// Writes assembly code that effects the if-goto command.
void write_if(Code_Writer *cw, String_View label);

// Writes assembly code that effects the call command.
void write_call(Code_Writer *cw, String_View f_name, uint16_t num_args);

// Writes assembly code that effects the return command.
void write_return(Code_Writer *cw);

// Writes assembly code that effects the function command.
void write_function(Code_Writer *cw, String_View f_name, uint16_t num_locals);

// Close the output file
void code_writer_close(Code_Writer *cw);

#endif // CODEWRITER_H_