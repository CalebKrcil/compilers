#include "symtab.h"

// Create a new symbol table with an optional parent (for scoping)
SymbolTable mksymtab(int nBuckets, SymbolTable parent) {
    SymbolTable st = malloc(sizeof(struct sym_table));
    if (!st) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table\n");
        exit(EXIT_FAILURE);
    }

    st->nBuckets = nBuckets;
    st->nEntries = 0;
    st->parent = parent;  // Store parent scope reference
    st->tbl = calloc(nBuckets, sizeof(SymbolTableEntry));

    if (!st->tbl) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table buckets\n");
        free(st);
        exit(EXIT_FAILURE);
    }

    return st;
}


// Hash function for symbols
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

// Insert a symbol with attributes
void insert_symbol(SymbolTable st, char *s, SymbolKind kind, char *type) {
    int index = hash(st, s);
    SymbolTableEntry newEntry = malloc(sizeof(struct sym_entry));

    newEntry->s = strdup(s);
    newEntry->kind = kind;
    newEntry->type = strdup(type);
    newEntry->table = st;  // Track which table this symbol belongs to
    newEntry->next = st->tbl[index]; // Handle collisions with linked list
    st->tbl[index] = newEntry;

    st->nEntries++;
}


// Look up a symbol in the current and parent scopes
SymbolTableEntry lookup_symbol(SymbolTable st, char *s) {
    int index = hash(st, s);
    SymbolTableEntry entry = st->tbl[index];

    // Search in current scope
    while (entry) {
        if (strcmp(entry->s, s) == 0) return entry;
        entry = entry->next;
    }

    // If not found, search in parent scope
    if (st->parent) return lookup_symbol(st->parent, s);
    
    return NULL; // Not found
}

// Print the symbol table with attributes
void print_symbols(SymbolTable st) {
    printf("\nSymbol Table (Scope Level: %p)\n", (void *)st);
    printf("=======================================================================\n");
    printf("| %-20s | %-10s | %-10s | %-10s | %-14s |\n", 
           "Symbol", "Kind", "Type", "Bucket", "Table Address");
    printf("=======================================================================\n");

    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            printf("| %-20s | %-10s | %-10s | %-10d | %-14p |\n", 
                   entry->s, 
                   (entry->kind == VARIABLE ? "Variable" : "Function"),
                   entry->type, i, (void *)entry->table);
            entry = entry->next;
        }
    }

    printf("=======================================================================\n");
    printf("Total Symbols: %d\n", st->nEntries);
}


// Free symbol table memory
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
