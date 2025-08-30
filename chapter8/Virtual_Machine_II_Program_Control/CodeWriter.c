#include "CodeWriter.h"

static inline int in_function(const Code_Writer *cw) {
    return cw->current_function[0] != '\0';
}

bool code_writer_init(Code_Writer *cw, const char *out_path, bool with_bootstrap) {
    cw->out = fopen(out_path, "w");
    if (!cw->out) return false;
    cw->label_counter = 0;
    cw->file_name[0]        = '\0';
    cw->current_function[0] = '\0';
    if (with_bootstrap) {
        // SP = 256 and call Sys.init
        fprintf(cw->out,
            "@256\n"
            "D=A\n"
            "@SP\n"
            "M=D\n"
        );
        write_call(cw, sv_from_cstr("Sys.init"), 0);
    } else {
        // SP = 256
        fprintf(cw->out,
            "@256\n"
            "D=A\n"
            "@SP\n"
            "M=D\n"
        );
    }
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

void write_label(Code_Writer *cw, String_View label) {
    if (in_function(cw)) {
        fprintf(cw->out, "(%s$%.*s)\n",
                cw->current_function, (int)label.count, label.data);
    } else {
        fprintf(cw->out, "(%.*s)\n", (int)label.count, label.data);
    }
}

void write_goto(Code_Writer *cw, String_View label) {
    if (in_function(cw)) {
        fprintf(cw->out, "@%s$%.*s\n0;JMP\n",
                cw->current_function, (int)label.count, label.data);
    } else {
        fprintf(cw->out, "@%.*s\n0;JMP\n", (int)label.count, label.data);
    }
}

void write_if(Code_Writer *cw, String_View label) {
    fprintf(cw->out,
        "@SP\n"
        "AM=M-1\n"
        "D=M\n");
    if (in_function(cw)) {
        fprintf(cw->out, "@%s$%.*s\nD;JNE\n",
                cw->current_function, (int)label.count, label.data);
    } else {
        fprintf(cw->out, "@%.*s\nD;JNE\n",
                (int)label.count, label.data);
    }
}

void write_call(Code_Writer *cw, String_View f_name, uint16_t num_args) {
    int id = cw->label_counter++;

    char return_label[64];
    snprintf(return_label, sizeof(return_label),
         "RETURN_%.*s_%d", (int)f_name.count, f_name.data, id);

    fprintf(cw->out,
        // push return-address
        "@%s\n"
        "D=A\n"
        "@SP\n"
        "A=M\n"
        "M=D\n"
        "@SP\n"
        "M=M+1\n"

        // push LCL
        "@LCL\n"
        "D=M\n"
        "@SP\n"
        "A=M\n"
        "M=D\n"
        "@SP\n"
        "M=M+1\n"

        // push ARG
        "@ARG\n"
        "D=M\n"
        "@SP\n"
        "A=M\n"
        "M=D\n"
        "@SP\n"
        "M=M+1\n"

        // push THIS
        "@THIS\n"
        "D=M\n"
        "@SP\n"
        "A=M\n"
        "M=D\n"
        "@SP\n"
        "M=M+1\n"

        // push THAT
        "@THAT\n"
        "D=M\n"
        "@SP\n"
        "A=M\n"
        "M=D\n"
        "@SP\n"
        "M=M+1\n"

        // ARG = SP - n - 5
        "@SP\n"
        "D=M\n"
        "@%u\n"
        "D=D-A\n"
        "@5\n"
        "D=D-A\n"
        "@ARG\n"
        "M=D\n"

        // LCL = SP
        "@SP\n"
        "D=M\n"
        "@LCL\n"
        "M=D\n"

        // goto f
        "@%.*s\n"
        "0;JMP\n"

        // (return-address)
        "(%s)\n",

        return_label,
        num_args,
        (int)f_name.count, f_name.data,
        return_label
    );
}

void write_return(Code_Writer *cw) {
    fprintf(cw->out,
        // Store the base address of the current function's frame
        "@LCL\n"
        "D=M\n"
        "@R13\n"
        "M=D\n"  // R13 is used as FRAME temporary variable

        // Retrieve the return address from FRAME-5
        "@R13\n"
        "D=M\n"
        "@5\n"
        "A=D-A\n"       // Address of return location: FRAME - 5
        "D=M\n"         // D = *(FRAME-5)
        "@R14\n"
        "M=D\n"         // R14 stores RET temporarily

        // Reposition the return value for the caller
        "@SP\n"
        "AM=M-1\n"      // Decrement SP and point to top of stack
        "D=M\n"         // D = pop()
        "@ARG\n"
        "A=M\n"
        "M=D\n"         // *ARG = return value

        // Restore the caller's SP
        "@ARG\n"
        "D=M+1\n"
        "@SP\n"
        "M=D\n"

        // Restore THAT of the caller
        "@R13\n"
        "D=M\n"
        "@1\n"
        "A=D-A\n"       // Address of THAT in the caller's frame
        "D=M\n"
        "@THAT\n"
        "M=D\n"

        // Restore THIS of the caller
        "@R13\n"
        "D=M\n"
        "@2\n"
        "A=D-A\n"       // Address of THIS in the caller's frame
        "D=M\n"
        "@THIS\n"
        "M=D\n"

        // Restore ARG of the caller
        "@R13\n"
        "D=M\n"
        "@3\n"
        "A=D-A\n"       // Address of ARG in the caller's frame
        "D=M\n"
        "@ARG\n"
        "M=D\n"

        // Restore LCL of the caller
        "@R13\n"
        "D=M\n"
        "@4\n"
        "A=D-A\n"       // Address of LCL in the caller's frame
        "D=M\n"
        "@LCL\n"
        "M=D\n"

        // Transfer control to the return address
        "@R14\n"
        "A=M\n"
        "0;JMP\n"
    );
}

void write_function(Code_Writer *cw, String_View f_name, uint16_t num_locals) {
    int id = cw->label_counter++;
    char loop[32], loop_end[32];
    snprintf(loop,     sizeof(loop),     "LOOP_%d",     id);
    snprintf(loop_end, sizeof(loop_end), "LOOP_%d_END", id);

    fprintf(cw->out,
        "(%.*s)\n"   // Function entry label
        "@%u\n"      // num_locals
        "D=A\n"
        "@R13\n"
        "M=D\n"      // Initialize count down: M[R13] = num_locals
        "(%s)\n"     // Loop start
        "@R13\n"
        "D=M\n"
        "@%s\n"
        "D;JEQ\n"    // If M[R13] == 0 then jump to loop end, else PUSH 0
        "@0\n"       // PUSH 0 start
        "D=A\n"      // .
        "@SP\n"      // .
        "A=M\n"      // .
        "M=D\n"      // .
        "@SP\n"      // .
        "M=M+1\n"    // PUSH 0 end
        "@R13\n"
        "M=M-1\n"    // decrement M[R13]
        "@%s\n"
        "0;JMP\n"    // next iteration
        "(%s)\n",    // Loop end
        (int)f_name.count, f_name.data,
        num_locals,
        loop,
        loop_end,
        loop,
        loop_end
    );

    // update current function
    size_t n = f_name.count;
    if (n >= sizeof(cw->current_function)) n = sizeof(cw->current_function) - 1;
    memcpy(cw->current_function, f_name.data, n);
    cw->current_function[n] = '\0';
}

void code_writer_close(Code_Writer *cw) {
    fprintf(cw->out, 
        "(_END_)\n"
        "@_END_\n"
        "0;JMP"
    );
    if (cw->out) fclose(cw->out);
}