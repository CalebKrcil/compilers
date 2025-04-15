#ifndef CODEGEN_H
#define CODEGEN_H

#include "tree.h"

void generate_code(struct tree *t);
void write_ic_file(const char *input_filename, struct instr *code);

#endif