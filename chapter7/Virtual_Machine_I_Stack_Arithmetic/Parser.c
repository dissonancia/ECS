#include "Parser.h"

static inline Command_Type parser_command_type(String_View cmd) {
    if (sv_eq(cmd, sv_from_cstr("push")))     return C_PUSH;
    if (sv_eq(cmd, sv_from_cstr("pop")))      return C_POP;
    if (sv_eq(cmd, sv_from_cstr("label")))    return C_LABEL;
    if (sv_eq(cmd, sv_from_cstr("goto")))     return C_GOTO;
    if (sv_eq(cmd, sv_from_cstr("if-goto")))  return C_IF;
    if (sv_eq(cmd, sv_from_cstr("function"))) return C_FUNCTION;
    if (sv_eq(cmd, sv_from_cstr("return")))   return C_RETURN;
    if (sv_eq(cmd, sv_from_cstr("call")))     return C_CALL;

    return C_ARITHMETIC;
}

static inline String_View parser_arg1(Command_Type type, Words ws) {
    if (type == C_RETURN) {
        fprintf(stderr, "arg1 called on C_RETURN!\n");
        exit(1);
    }
    if (type == C_ARITHMETIC) return ws.items[0];
    return ws.items[1];
}

static inline int16_t parser_arg2(Command_Type type, Words ws) {
    if (!(type == C_PUSH || type == C_POP ||
          type == C_FUNCTION || type == C_CALL)) {
        fprintf(stderr, "arg2 called on invalid command type!\n");
        exit(1);
    }

    return sv_to_i16(ws.items[2]);
}

bool parser_init(Parser *p, const char *path) {
    p->data = (String_Builder){0};
    if (!read_entire_file(path, &p->data)) {
        fprintf(stderr, "parser_init: failed to read %s\n", path);
        return false;
    }

    p->content = sb_to_sv(p->data);
    p->current = (String_View){0};
    p->ws = (Words){0};
    return true;
}

bool has_more_commands(Parser *p) {
    String_View tmp = p->content;
    while (tmp.count > 0) {
        String_View line = sv_chop_by_delim(&tmp, '\n');
        line = sv_strip_comment(line);
        line = sv_trim(line);
        if (line.count > 0) {
            return true; // has at least 1 valid line
        }
    }
    return false;
}

void advance(Parser *p) {
    // release previous tokens
    da_free(p->ws);

    while (p->content.count > 0) {
        String_View line = sv_chop_by_delim(&p->content, '\n');
        line = sv_strip_comment(line);
        line = sv_trim(line);

        if (line.count > 0) {
            p->current = line;
            p->ws = words(line);
            return;
        }
    }

    p->current = (String_View){0};
}

Command_Type command_type(Parser *p) {
    return parser_command_type(p->ws.items[0]);
}

String_View arg1(Parser *p) {
    Command_Type type = command_type(p);
    return parser_arg1(type, p->ws);
}

int16_t arg2(Parser *p) {
    Command_Type type = command_type(p);
    return parser_arg2(type, p->ws);
}

void parser_free(Parser *p) {
    da_free(p->ws);
    sb_free(p->data);
}