#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { VARIABLE, FUNCTION} SymbolKind;

typedef struct sym_entry {
    char *s;
    SymbolKind kind;
    char *type;
    struct sym_table *table;
    struct sym_entry *next;
} *SymbolTableEntry;

typedef struct sym_table {
    int nBuckets;            // Number of buckets
    int nEntries;            // Number of symbols
    struct sym_table *parent; // Enclosing scope (NULL if global)
    struct sym_entry **tbl;  // Array of linked lists
} *SymbolTable;


SymbolTable mksymtab(int nBuckets, SymbolTable parent);
void insert_symbol(SymbolTable st, char *s, SymbolKind kind, char *type);
SymbolTableEntry lookup_symbol(SymbolTable st, char *s);
void print_symbols(SymbolTable st);
void free_symbol_table(SymbolTable st);
int hash(SymbolTable st, char *s);

#endif
