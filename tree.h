#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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
};

int alctoken(int category, char *text);
struct tree *alctree(int prodrule, char *symbolname, int nkids, ...);
void freetree(struct tree *t);
void printtree(struct tree *t, int depth);
void print_graph(struct tree *t, char *filename);
void printsyms(struct tree *t);

#endif
