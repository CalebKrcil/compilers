#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "tree.h"

// Allocate and initialize an internal node
struct tree* alctree(int prodrule, const char* symbolname, int nkids, ...) {
    struct tree* node = malloc(sizeof(struct tree));
    if (!node) return NULL;
    
    node->prodrule = prodrule;
    node->symbolname = strdup(symbolname);
    node->nkids = nkids;
    node->leaf = NULL;
    
    va_list ap;
    va_start(ap, nkids);
    for (int i = 0; i < nkids; i++) {
        node->kids[i] = va_arg(ap, struct tree*);
    }
    va_end(ap);
    
    return node;
}

// Allocate and initialize a leaf node
struct tree* alcleaf(int category, const char* text, int lineno) {
    struct tree* node = malloc(sizeof(struct tree));
    if (!node) return NULL;
    
    node->prodrule = category;
    node->symbolname = NULL;
    node->nkids = 0;
    
    node->leaf = malloc(sizeof(struct token));
    if (!node->leaf) {
        free(node);
        return NULL;
    }
    
    node->leaf->category = category;
    node->leaf->text = strdup(text);
    node->leaf->lineno = lineno;
    
    return node;
}

// Print information about a single node
void printnode(struct tree* node) {
    if (!node) return;
    
    if (node->nkids == 0 && node->leaf) {
        printf("LEAF: category=%d, text='%s', line=%d\n",
               node->leaf->category,
               node->leaf->text,
               node->leaf->lineno);
    } else {
        printf("INTERNAL: rule=%d, symbol='%s', children=%d\n",
               node->prodrule,
               node->symbolname,
               node->nkids);
    }
}

// Print entire tree (simple pre-order traversal)
void tree_print(struct tree* root) {
    if (!root) return;
    
    printnode(root);
    for (int i = 0; i < root->nkids; i++) {
        tree_print(root->kids[i]);
    }
}