#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "tree.h"
#include "type.h"
#include "symtab.h"

void check_semantics(struct tree *t);
int check_type_compatibility(typeptr expected, typeptr actual);
void report_semantic_error(const char *msg, int lineno);
int is_operator(int prodrule);
int is_null_literal(struct tree *t);

#endif
