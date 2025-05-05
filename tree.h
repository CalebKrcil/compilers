#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "symtab.h"
#include "tac.h"

struct sym_table;
typedef struct sym_table *SymbolTable;
struct typeinfo;
typedef struct typeinfo *typeptr;

struct token {
    int category;
    char *text;
    int lineno;
    char *filename;

    union {
        int ival;
        double dval;
        char *sval;
    } value;
};

struct tree {
    int id;
    int prodrule;
    char *symbolname;
    int nkids;
    struct tree *kids[10];
    struct token *leaf;
    typeptr type;
    int is_mutable;
    int is_nullable;
    int lineno;
    SymbolTable scope;
    struct addr place;
    struct instr *code;
    int returned;
    struct addr first;     // Entry label for this node
    struct addr follow;    // Exit label for this node
    struct addr onTrue;    // Branch target when condition is true
    struct addr onFalse;   // Branch target when condition is false
    int first_used;        // Flag indicating if first is used
    int follow_used;       // Flag indicating if follow is used
    int onTrue_used;       // Flag indicating if onTrue is used
    int onFalse_used;      // Flag indicating if onFalse is used
};

typedef struct func_symtab_list {
    SymbolTable symtab;
    struct func_symtab_list *next;
} *FuncSymbolTableList;

FuncSymbolTableList printsyms(struct tree *t, SymbolTable st);
void free_func_symtab_list(FuncSymbolTableList list);
int alctoken(int category, char *text);
struct tree *alctree(int prodrule, char *symbolname, int nkids, ...);
void freetree(struct tree *t);
void printtree(struct tree *t, int depth);
void print_graph(struct tree *t, char *filename);
char *get_type_name(struct tree *type_node);
void print_branch(struct tree *t, FILE *f);
void print_leaf(struct tree *t, FILE *f);
void print_graph2(struct tree *t, FILE *f);
char *pretty_print_name(struct tree *t);
char *escape(char *s);
void flattenParameterList(struct tree *node, struct tree ***params, int *count);
void print_graph_TAC(struct tree *t, char *filename);
void print_graph2_TAC(struct tree *t, FILE *f);
void format_instruction(char *buf, size_t bufsize, struct instr *i);

#endif
