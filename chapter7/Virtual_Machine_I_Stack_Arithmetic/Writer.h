#ifndef WRITER_H_
#define WRITER_H_

#include "Parser.h"

typedef struct {
    FILE *out;                 // .asm output
    char file_name[MAX_PATH];  // base name of the current file (.vm)
    int label_counter;         // counter to generate unique labels (e.g. eq_true1, eq_true2)
} Code_Writer;

// Initialize and open the output file (.asm)
bool code_writer_init(Code_Writer *cw, const char *output_path);

// Initialize Stack
void write_init(Code_Writer *cw);

// Informs that it has started translating a new VM file
void set_file_name(Code_Writer *cw, const char *file_name);

// Write code for arithmetic commands (add, sub, neg, eq, gt, lt, and, or, not)
void write_arithmetic(Code_Writer *cw, String_View command);

// Write code for push/pop
void write_push_pop(Code_Writer *cw,
                    Command_Type command,
                    String_View segment,
                    int index);

// Close the output file
void code_writer_close(Code_Writer *cw);

#endif // WRITER_H_