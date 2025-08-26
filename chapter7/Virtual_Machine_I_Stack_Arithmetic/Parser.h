#ifndef PARSER_H_
#define PARSER_H_

//#define UDEF static inline
#include "Utils.h"

typedef enum {
    C_ARITHMETIC,
    C_PUSH,
    C_POP,
    C_LABEL,
    C_GOTO,
    C_IF,
    C_FUNCTION,
    C_RETURN,
    C_CALL
} Command_Type;

typedef struct {
    String_Builder data;   // allocated data
    String_View content;   // entire file content
    String_View current;   // current command (line already cleared)
    Words       ws;        // tokens of the current line
} Parser;

// Initialize the parser with the contents of a .vm file
bool parser_init(Parser *p, const char *path);

// Check if there are still commands
bool has_more_commands(Parser *p);

// Advance to the next command
void advance(Parser *p);

// What is the type of the current command
Command_Type command_type(Parser *p);

// First argument
String_View arg1(Parser *p);

// Second argument
int16_t arg2(Parser *p);

// Frees resources
void parser_free(Parser *p);

#endif // PARSER_H_