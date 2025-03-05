#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct sym_entry {
    char *s;
    struct sym_entry *next;
} *SymbolTableEntry;

typedef struct sym_table {
    int nBuckets;
    int nEntries;
    struct sym_entry **tbl;
} *SymbolTable;

SymbolTable mksymtab(int nBuckets);
void insert_symbol(SymbolTable st, char *s);
int lookup_symbol(SymbolTable st, char *s);
void print_symbols(SymbolTable st);
void free_symbol_table(SymbolTable st);
int hash(SymbolTable st, char *s);

#endif
