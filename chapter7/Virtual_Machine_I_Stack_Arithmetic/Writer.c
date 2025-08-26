#include "Writer.h"

static inline void write_init(Code_Writer *cw) {
    // SP = 256
    fprintf(cw->out,
        "@256\n"
        "D=A\n"
        "@SP\n"
        "M=D\n"
    );
}

bool code_writer_init(Code_Writer *cw, const char *output_path) {
    cw->out = fopen(output_path, "w");
    if (!cw->out) {
        fprintf(stderr, "codewriter_init: failed to open %s\n", output_path);
        return false;
    }
    write_init(cw);
    cw->label_counter = 0;
    cw->file_name[0] = '\0';
    return true;
}

void set_file_name(Code_Writer *cw, const char *file_path) {
    const char *b1 = strrchr(file_path, '/');
    const char *b2 = strrchr(file_path, '\\');
    const char *base = b1 > b2 ? b1 : b2;
    base = base ? base + 1 : file_path;
    strncpy(cw->file_name, base, sizeof(cw->file_name));
    cw->file_name[sizeof(cw->file_name)-1] = '\0';
    char *dot = strrchr(cw->file_name, '.');
    if (dot) *dot = '\0';
}

// true: -1 | false: 0
void write_arithmetic(Code_Writer *cw, String_View command) {
    if (sv_eq(command, sv_from_cstr("add"))) {
        fprintf(cw->out,
            "@SP\n"
            "AM=M-1\n"     // SP--; D = *SP
            "D=M\n"
            "A=A-1\n"      // A = SP-1
            "M=D+M\n"      // *SP = *SP + D
        );
    } else if (sv_eq(command, sv_from_cstr("sub"))) {
        fprintf(cw->out,
            "@SP\n"
            "AM=M-1\n"   // SP--; A=SP
            "D=M\n"      // D = y
            "A=A-1\n"    // A = SP-1 (x)
            "D=M-D\n"    // D = x - y
            "M=D\n"      // *SP = x - y
        );
    } else if (sv_eq(command, sv_from_cstr("neg"))) {
        fprintf(cw->out,
            "@SP\n"
            "A=M-1\n"
            "M=-M\n"
        );
    } else if (sv_eq(command, sv_from_cstr("eq"))) {
        int id = cw->label_counter++;
        char true_label[32], end_label[32];
        snprintf(true_label, sizeof(true_label), "EQ_TRUE_%d", id);
        snprintf(end_label, sizeof(end_label), "EQ_END_%d", id);

        fprintf(cw->out,
            "@SP\n"
            "AM=M-1\n"       // SP--; A=SP
            "D=M\n"          // D = y
            "A=A-1\n"        // A = SP-1
            "D=M-D\n"        // D = x - y
            "@%s\n"
            "D;JEQ\n"        // if x==y, jump to EQ_TRUE_N
            "@SP\n"
            "A=M-1\n"
            "M=0\n"          // false
            "@%s\n"
            "0;JMP\n"
            "(%s)\n"
            "@SP\n"
            "A=M-1\n"
            "M=-1\n"         // true
            "(%s)\n",
            true_label, end_label, true_label, end_label
        );
    } else if (sv_eq(command, sv_from_cstr("gt"))) {
        int id = cw->label_counter++;
        char true_label[32], end_label[32];
        snprintf(true_label, sizeof(true_label), "GT_TRUE_%d", id);
        snprintf(end_label, sizeof(end_label), "GT_END_%d", id);

        fprintf(cw->out,
            "@SP\n"
            "AM=M-1\n"       // SP--; A=SP
            "D=M\n"          // D = y
            "A=A-1\n"        // A = SP-1
            "D=M-D\n"        // D = x - y
            "@%s\n"
            "D;JGT\n"        // if x > y, jump to GT_TRUE_N
            "@SP\n"
            "A=M-1\n"
            "M=0\n"          // false
            "@%s\n"
            "0;JMP\n"
            "(%s)\n"
            "@SP\n"
            "A=M-1\n"
            "M=-1\n"         // true
            "(%s)\n",
            true_label, end_label, true_label, end_label
        );
    } else if (sv_eq(command, sv_from_cstr("lt"))) {
        int id = cw->label_counter++;
        char true_label[32], end_label[32];
        snprintf(true_label, sizeof(true_label), "LT_TRUE_%d", id);
        snprintf(end_label, sizeof(end_label), "LT_END_%d", id);

        fprintf(cw->out,
            "@SP\n"
            "AM=M-1\n"       // SP--; A=SP
            "D=M\n"          // D = y
            "A=A-1\n"        // A = SP-1
            "D=M-D\n"        // D = x - y
            "@%s\n"
            "D;JLT\n"        // if x < y, jump to LT_TRUE_N
            "@SP\n"
            "A=M-1\n"
            "M=0\n"          // false
            "@%s\n"
            "0;JMP\n"
            "(%s)\n"
            "@SP\n"
            "A=M-1\n"
            "M=-1\n"         // true
            "(%s)\n",
            true_label, end_label, true_label, end_label
        );
    } else if (sv_eq(command, sv_from_cstr("and"))) {
        fprintf(cw->out,
            "@SP\n"
            "AM=M-1\n"   // SP--; A=SP
            "D=M\n"      // D = y
            "A=A-1\n"    // A = SP-1
            "M=D&M\n"    // *SP = x & y
        );
    } else if (sv_eq(command, sv_from_cstr("or"))) {
        fprintf(cw->out,
            "@SP\n"
            "AM=M-1\n"   // SP--; A=SP
            "D=M\n"      // D = y
            "A=A-1\n"    // A = SP-1
            "M=D|M\n"    // *SP = x | y
        );
    } else if (sv_eq(command, sv_from_cstr("not"))) {
        fprintf(cw->out,
            "@SP\n"
            "A=M-1\n"   // A = top of the stack
            "M=!M\n"    // *SP = ~(*SP)
        );
    }
}

void write_push_pop(Code_Writer *cw, Command_Type command, String_View segment, int index) {
    if (command == C_PUSH) {
        if (sv_eq(segment, sv_from_cstr("constant"))) {
            // push constant i
            fprintf(cw->out,
                "@%d\n"
                "D=A\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("local"))) {
            fprintf(cw->out,
                "@LCL\n"
                "D=M\n"
                "@%d\n"
                "A=D+A\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("argument"))) {
            fprintf(cw->out,
                "@ARG\n"
                "D=M\n"
                "@%d\n"
                "A=D+A\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("this"))) {
            fprintf(cw->out,
                "@THIS\n"
                "D=M\n"
                "@%d\n"
                "A=D+A\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("that"))) {
            fprintf(cw->out,
                "@THAT\n"
                "D=M\n"
                "@%d\n"
                "A=D+A\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("static"))) {
            fprintf(cw->out,
                "@%s.%d\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                cw->file_name, index
            );
        } else if (sv_eq(segment, sv_from_cstr("temp"))) {
            fprintf(cw->out,
                "@R%d\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                5 + index
            );
        } else if (sv_eq(segment, sv_from_cstr("pointer"))) {
            // pointer 0 = THIS, pointer 1 = THAT
            fprintf(cw->out,
                "@%s\n"
                "D=M\n"
                "@SP\n"
                "A=M\n"
                "M=D\n"
                "@SP\n"
                "M=M+1\n",
                index == 0 ? "THIS" : "THAT"
            );
        }
    } else if (command == C_POP) {
        if (sv_eq(segment, sv_from_cstr("local"))) {
            fprintf(cw->out,
                "@LCL\n"
                "D=M\n"
                "@%d\n"
                "D=D+A\n"
                "@R13\n"
                "M=D\n"        // R13 = LCL + index
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@R13\n"
                "A=M\n"
                "M=D\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("argument"))) {
            fprintf(cw->out,
                "@ARG\n"
                "D=M\n"
                "@%d\n"
                "D=D+A\n"
                "@R13\n"
                "M=D\n"
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@R13\n"
                "A=M\n"
                "M=D\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("this"))) {
            fprintf(cw->out,
                "@THIS\n"
                "D=M\n"
                "@%d\n"
                "D=D+A\n"
                "@R13\n"
                "M=D\n"
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@R13\n"
                "A=M\n"
                "M=D\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("that"))) {
            fprintf(cw->out,
                "@THAT\n"
                "D=M\n"
                "@%d\n"
                "D=D+A\n"
                "@R13\n"
                "M=D\n"
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@R13\n"
                "A=M\n"
                "M=D\n",
                index
            );
        } else if (sv_eq(segment, sv_from_cstr("static"))) {
            fprintf(cw->out,
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@%s.%d\n"
                "M=D\n",
                cw->file_name, index
            );
        } else if (sv_eq(segment, sv_from_cstr("temp"))) {
            fprintf(cw->out,
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@R%d\n"
                "M=D\n",
                5 + index
            );
        } else if (sv_eq(segment, sv_from_cstr("pointer"))) {
            fprintf(cw->out,
                "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "@%s\n"
                "M=D\n",
                index == 0 ? "THIS" : "THAT"
            );
        }
    }
}

void code_writer_close(Code_Writer *cw) {
    if (cw->out) fclose(cw->out);
}