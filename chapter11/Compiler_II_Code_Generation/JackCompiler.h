#ifndef JACKCOMPILER_H_
#define JACKCOMPILER_H_

#include "CompilationEngine.h"

typedef struct {
    const char* path;
} JackCompiler;

int jack_compiler_run(const char* path);

#endif // JACKCOMPILER_H_