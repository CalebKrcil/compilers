#include "tree.h"
#include "k0gram.tab.h"
#include "ytab.h"
#include "symtab.h"

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

void printsymbol(char *s) {
    printf("%s\n", s);
    fflush(stdout);
}

// Recursive function to traverse and print symbols
void printsyms(struct tree *t) {
    if (!t) return;

    // printf("Visiting: %s\n", t->symbolname);

    // If this node is an identifier, print it
    if (t->leaf && t->leaf->category == 396) {
        printsymbol(t->leaf->text);
    }

    // Recursively visit all child nodes
    for (int i = 0; i < t->nkids; i++) {
        printsyms(t->kids[i]);
    }
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
static int serial = 0;
struct tree *alctree(int prodrule, char *symbolname, int nkids, ...) {
    struct tree *t = malloc(sizeof(struct tree));
    if (!t) {
        fprintf(stderr, "Memory allocation failed for tree node\n");
        exit(EXIT_FAILURE);
    }

    t->id = serial++;  // Ensure every node gets a unique ID
    t->prodrule = prodrule;
    t->symbolname = strdup(symbolname);
    t->nkids = nkids;
    t->leaf = NULL;

    va_list args;
    va_start(args, nkids);
    for (int i = 0; i < nkids; i++) {
        t->kids[i] = va_arg(args, struct tree *);
        if (t->kids[i]) t->kids[i]->id = serial++;  // Ensure unique IDs for children
    }
    va_end(args);

    return t;
}

/**
 * Frees memory allocated for a syntax tree.
 */
void freetree(struct tree *t) {
    if (!t) return;

    // Free children recursively
    for (int i = 0; i < t->nkids; i++) {
        freetree(t->kids[i]);
        t->kids[i] = NULL;  // Avoid dangling pointers
    }

    // Free the token if it's a leaf
    if (t->leaf) {
        freetoken(t->leaf);
        t->leaf = NULL;
    }

    // Free symbol name (strdup'd string)
    if (t->symbolname) {
        free(t->symbolname);
        t->symbolname = NULL;
    }

    // Free the tree node itself
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
            printf("Leaf: IntegerLiteral - %d, Integer code: %d\n", t->leaf->value.ival, t->leaf->category);
        } else if (t->leaf->category == RealLiteral) {
            printf("Leaf: RealLiteral - %lf, Integer code: %d\n", t->leaf->value.dval, t->leaf->category);
        } else if (t->leaf->category == CharacterLiteral) {
            printf("Leaf: CharacterLiteral - '%s', Integer code: %d\n", t->leaf->value.sval, t->leaf->category);
        } else {
            printf("Leaf: %s - %s, Integer code: %d\n", t->symbolname, t->leaf->text, t->leaf->category);
        }
    } else {
        // Print node names
        printf("Node: %s\n", t->symbolname);
        for (int i = 0; i < t->nkids; i++) {
            printtree(t->kids[i], depth + 1);
        }
    }
}

char *escape(char *s) {
    int len = strlen(s);
    char *s2 = malloc(len * 2 + 1);  // Allocate extra space for escaping
    char *p = s2;

    for (int i = 0; i < len; i++) {
        if (s[i] == '"' || s[i] == '\\') {  // Escape quotes and backslashes
            *p++ = '\\';
        }
        *p++ = s[i];
    }
    *p = '\0';  // Null-terminate the new string
    return s2;
}

 
 char *pretty_print_name(struct tree *t) {
    char *s2 = malloc(40);
    if (t->leaf == NULL) {
       sprintf(s2, "%s#%d", t->symbolname, t->prodrule);
       return s2;
       }
    else {
       sprintf(s2,"%s:%d", escape(t->leaf->text), t->leaf->category);
       return s2;
       }
 }
 
 void print_branch(struct tree *t, FILE *f) {
    fprintf(f, "N%d [shape=box label=\"%s\"];\n", t->id, pretty_print_name(t));
 }
  
 void print_leaf(struct tree *t, FILE *f) {
    if (!t || !t->leaf) return;  // Safety check
    fprintf(f, "N%d [shape=box style=dotted label=\"%s\\n", t->id, "Leaf");
    fprintf(f, "text = %s \\l lineno = %d \\l\"];\n", escape(t->leaf->text), t->leaf->lineno);
}

 
 void print_graph2(struct tree *t, FILE *f) {
    int i;
    if (t->leaf != NULL) {
       print_leaf(t, f);
       return;
       }
    /* not a leaf ==> internal node */
    print_branch(t, f);
    for(i=0; i < t->nkids; i++) {
       if (t->kids[i] != NULL) {
          fprintf(f, "N%d -> N%d;\n", t->id, t->kids[i]->id);
      print_graph2(t->kids[i], f);
      }
       else { /* NULL kid, epsilon production or something */
          fprintf(f, "N%d -> N%d%d;\n", t->id, t->id, serial);
      fprintf(f, "N%d%d [label=\"%s\"];\n", t->id, serial, "Empty rule");
      serial++;
      }
       }
 }
 
 void print_graph(struct tree *t, char *filename){
       FILE *f = fopen(filename, "w"); /* should check for NULL */
       fprintf(f, "digraph {\n");
       print_graph2(t, f);
       fprintf(f,"}\n");
       fclose(f);
 }