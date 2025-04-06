#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "symtab.h"

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

#endif
