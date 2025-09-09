#ifndef JACKANALYZER_H_
#define JACKANALYZER_H_

#include "CompilationEngine.h"

typedef struct {
    const char* path;
} JackAnalyzer;

int jack_analyzer_run(const char* path);

#endif // JACKANALYZER_H_