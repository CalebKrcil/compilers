#include "tree.h"
#include "k0gram.tab.h"
#include "ytab.h"
#include <stdarg.h>
#include <string.h>

extern int yylineno;
extern char *current_filename;
extern YYSTYPE yylval;

/**
 * Allocates a token and initializes it based on its category.
 */
int alctoken(int category, char *text) {
    yylval.treeptr = malloc(sizeof(struct tree));
    if (!yylval.treeptr) {
        fprintf(stderr, "Memory allocation failed for tree node\n");
        exit(EXIT_FAILURE);
    }

    yylval.treeptr->prodrule = category;
    yylval.treeptr->symbolname = strdup(text);
    yylval.treeptr->nkids = 0;
    yylval.treeptr->leaf = malloc(sizeof(struct token));

    struct token *tok = yylval.treeptr->leaf;
    tok->category = category;
    tok->text = strdup(text);
    tok->lineno = yylineno;
    tok->filename = strdup(current_filename);

    if (category == IntegerLiteral) {
        tok->value.ival = atoi(text);
    } else if (category == RealLiteral) {
        tok->value.dval = atof(text);
    } else if (category == CharacterLiteral) {
        tok->value.sval = strdup(text);
    } else {
        tok->value.sval = NULL;
    }

    return category;
}


/**
 * Frees memory allocated for a token.
 */
void freetoken(struct token *t) {
    if (!t) return;
    free(t->text);
    free(t->filename);
    if (t->category == CharacterLiteral) {
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

    // Print indentation based on depth
    for (int i = 0; i < depth; i++) printf("  ");

    if (t->leaf) {
        // Print different representations for literals
        if (t->leaf->category == IntegerLiteral) {
            printf("Leaf: IntegerLiteral - %d\n", t->leaf->value.ival);
        } else if (t->leaf->category == RealLiteral) {
            printf("Leaf: RealLiteral - %lf\n", t->leaf->value.dval);
        } else if (t->leaf->category == CharacterLiteral) {
            printf("Leaf: CharacterLiteral - '%s'\n", t->leaf->value.sval);
        } else {
            printf("Leaf: %s - %s\n", t->symbolname, t->leaf->text);
        }
    } else {
        // Print node names
        printf("Node: %s\n", t->symbolname);
        for (int i = 0; i < t->nkids; i++) {
            printtree(t->kids[i], depth + 1);
        }
    }
}
