#include "symtab.h"

SymbolTable mksymtab(int nBuckets, SymbolTable parent) {
    SymbolTable st = malloc(sizeof(struct sym_table));
    if (!st) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table\n");
        exit(EXIT_FAILURE);
    }
    st->nBuckets = nBuckets;
    st->nEntries = 0;
    st->parent = parent;
    st->tbl = calloc(nBuckets, sizeof(SymbolTableEntry));
    if (!st->tbl) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table buckets\n");
        free(st);
        exit(EXIT_FAILURE);
    }
    return st;
}

int hash(SymbolTable st, char *s) {
    int h = 0;
    while (*s) {
        h = (h * 37 + *s++) % st->nBuckets;
    }
    return h;
}

void insert_symbol(SymbolTable st, char *s, SymbolKind kind, char *type) {
    // if (lookup_symbol_current_scope(st, s)) {
    //     fprintf(stderr, "Error: Redeclaration of variable '%s'\n", s);
    //     exit(3);
    // }
    int index = hash(st, s);
    SymbolTableEntry newEntry = malloc(sizeof(struct sym_entry));
    newEntry->s = strdup(s);
    newEntry->kind = kind;
    newEntry->type = strdup(type);
    newEntry->table = st;
    newEntry->next = st->tbl[index];
    st->tbl[index] = newEntry;
    st->nEntries++;
}

SymbolTableEntry lookup_symbol(SymbolTable st, char *s) {
    int index = hash(st, s);
    SymbolTableEntry entry = st->tbl[index];
    while (entry) {
        if (strcmp(entry->s, s) == 0) return entry;
        entry = entry->next;
    }
    return st->parent ? lookup_symbol(st->parent, s) : NULL;
}

SymbolTableEntry lookup_symbol_current_scope(SymbolTable st, char *s) {
    int index = hash(st, s);
    SymbolTableEntry entry = st->tbl[index];
    while (entry) {
        if (strcmp(entry->s, s) == 0) return entry;
        entry = entry->next;
    }
    return NULL;
}

void check_undeclared(SymbolTable st, char *s) {
    if (!lookup_symbol(st, s)) {
        fprintf(stderr, "Error: Undeclared variable '%s'\n", s);
        exit(3);
    }
}

void print_symbols(SymbolTable st) {
    printf("\nSymbol Table:\n====================\n");
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            printf("%s (%s)\n", entry->s, (entry->kind == VARIABLE) ? "Variable" : "Function");
            entry = entry->next;
        }
    }
}

void free_symbol_table(SymbolTable st) {
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            SymbolTableEntry temp = entry;
            entry = entry->next;
            free(temp->s);
            free(temp->type);
            free(temp);
        }
    }
    free(st->tbl);
    free(st);
}

SymbolTable create_function_scope(SymbolTable parent, char *func_name) {
    return mksymtab(50, parent);
}
