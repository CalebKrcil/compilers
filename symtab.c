#include "symtab.h"

SymbolTable mksymtab(int nBuckets) {
    SymbolTable st = malloc(sizeof(struct sym_table));
    st->nBuckets = nBuckets;
    st->nEntries = 0;
    st->tbl = calloc(nBuckets, sizeof(SymbolTableEntry));
    return st;
}

int hash(SymbolTable st, char *s) {
    int h = 0;
    char c;
    while ((c = *s++)) {
        h += c & 0xFF;
        h *= 37;
    }
    if (h < 0) h = -h;
    return h % st->nBuckets;
}

void insert_symbol(SymbolTable st, char *s) {
    int index = hash(st, s);
    SymbolTableEntry newEntry = malloc(sizeof(struct sym_entry));
    newEntry->s = strdup(s);
    newEntry->next = st->tbl[index];
    st->tbl[index] = newEntry;
    st->nEntries++;
}

int lookup_symbol(SymbolTable st, char *s) {
    int index = hash(st, s);
    SymbolTableEntry entry = st->tbl[index];
    while (entry) {
        if (strcmp(entry->s, s) == 0) return 1; // Found
        entry = entry->next;
    }
    return 0; // Not found
}

void print_symbols(SymbolTable st) {
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            printf("%s\n", entry->s);
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
            free(temp);
        }
    }
    free(st->tbl);
    free(st);
}
