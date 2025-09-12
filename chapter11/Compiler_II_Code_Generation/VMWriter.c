#include "VMWriter.h"

static const char *seg_to_str(Segment s) {
    switch (s) {
        case S_CONST:   return "constant";
        case S_ARG:     return "argument";
        case S_LOCAL:   return "local";
        case S_STATIC:  return "static";
        case S_THIS:    return "this";
        case S_THAT:    return "that";
        case S_POINTER: return "pointer";
        case S_TEMP:    return "temp";
        default:        return "unknown";
    }
}

static const char *cmd_to_str(Command c) {
    switch (c) {
        case C_ADD: return "add";
        case C_SUB: return "sub";
        case C_NEG: return "neg";
        case C_EQ:  return "eq";
        case C_GT:  return "gt";
        case C_LT:  return "lt";
        case C_AND: return "and";
        case C_OR:  return "or";
        case C_NOT: return "not";
        default:    return "unknown";
    }
}

bool vmw_open(FILE **out, const char *path) {
    if (!out || !path) return false;
    FILE *f = fopen(path, "w");
    if (!f) return false;
    *out = f;
    return true;
}

void vmw_close(FILE *out) {
    if (!out) return;
    fclose(out);
}

void vmw_write_push(FILE *out, Segment seg, size_t index) {
    if (!out) return;
    fprintf(out, "    push %s %d\n", seg_to_str(seg), index);
}

void vmw_write_pop(FILE *out, Segment seg, size_t index) {
    if (!out) return;
    fprintf(out, "    pop %s %d\n", seg_to_str(seg), index);
}

void vmw_write_arithmetic(FILE *out, Command cmd) {
    if (!out) return;
    fprintf(out, "    %s\n", cmd_to_str(cmd));
}

void vmw_write_label(FILE *out, const char *label) {
    if (!out || !label) return;
    fprintf(out, "label %s\n", label);
}

void vmw_write_goto(FILE *out, const char *label) {
    if (!out || !label) return;
    fprintf(out, "    goto %s\n", label);
}

void vmw_write_if(FILE *out, const char *label) {
    if (!out || !label) return;
    fprintf(out, "    if-goto %s\n", label);
}

void vmw_write_call(FILE *out, const char *name, size_t nArgs) {
    if (!out || !name) return;
    fprintf(out, "    call %s %d\n", name, nArgs);
}

void vmw_write_function(FILE *out, const char *name, size_t nLocals) {
    if (!out || !name) return;
    fprintf(out, "function %s %d\n", name, nLocals);
}

void vmw_write_return(FILE *out) {
    if (!out) return;
    fprintf(out, "    return\n");
}