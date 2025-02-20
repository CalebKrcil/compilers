#include "tree.h"
#include <stdarg.h>
#include <string.h>

extern int yylineno;
extern char *current_filename;

/**
 * Allocates a token and initializes it based on its category.
 */
struct token *alctoken(int category, char *text) {
    struct token *tok = malloc(sizeof(struct token));
    if (!tok) {
        fprintf(stderr, "Memory allocation failed for token\n");
        exit(EXIT_FAILURE);
    }

    tok->category = category;
    tok->text = strdup(text);
    tok->lineno = yylineno;
    tok->filename = strdup(current_filename);

    if (category == IntegerLiteral) {
        tok->value.ival = atoi(text);
    } else if (category == RealLiteral) {
        tok->value.dval = atof(text);
    } else if (category == CharacterLiteral || category == StringLiteral) {
        tok->value.sval = strdup(text);
    } else {
        tok->value.sval = NULL;
    }

    return tok;
}

/**
 * Frees memory allocated for a token.
 */
void freetoken(struct token *t) {
    if (!t) return;
    free(t->text);
    free(t->filename);
    if (t->category == CharacterLiteral || t->category == StringLiteral) {
        free(t->value.sval);
    }
    free(t);
}

/**
 * Allocates a new tree node with the given production rule, symbol name,
 * and child nodes.
 */
struct tree *alctree(int prodrule, char *symbolname, int nkids, ...) {
    struct tree *t = malloc(sizeof(struct tree));
    if (!t) {
        fprintf(stderr, "Memory allocation failed for tree node\n");
        exit(EXIT_FAILURE);
    }

    t->prodrule = prodrule;
    t->symbolname = symbolname;
    t->nkids = nkids;
    t->leaf = NULL;

    va_list args;
    va_start(args, nkids);
    for (int i = 0; i < nkids; i++) {
        t->kids[i] = va_arg(args, struct tree *);
    }
    va_end(args);

    return t;
}

/**
 * Frees memory allocated for a syntax tree.
 */
void freetree(struct tree *t) {
    if (!t) return;
    for (int i = 0; i < t->nkids; i++) {
        freetree(t->kids[i]);
    }
    if (t->leaf) {
        freetoken(t->leaf);
    }
    free(t);
}

/**
 * Recursively prints the syntax tree.
 */
void printtree(struct tree *t, int depth) {
    if (!t) return;
    for (int i = 0; i < depth; i++) printf("  ");
    
    if (t->leaf) {
        printf("Leaf: %s (Line %d)\n", t->leaf->text, t->leaf->lineno);
    } else {
        printf("Node: %s\n", t->symbolname);
        for (int i = 0; i < t->nkids; i++) {
            printtree(t->kids[i], depth + 1);
        }
    }
}
