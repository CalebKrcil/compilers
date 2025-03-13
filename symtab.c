#include "symtab.h"

extern int error_count;

SymbolTable mksymtab(int nBuckets, SymbolTable parent) {
    SymbolTable st = malloc(sizeof(struct sym_table));
    if (!st) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table\n");
        exit(EXIT_FAILURE);
    }
    st->nBuckets = nBuckets;
    st->nEntries = 0;
    st->parent = parent;
    st->scope_name = NULL;  // Will be set later
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
    printf("Inserting symbol: %s, Type: %s\n", s, type);
    if (lookup_symbol_current_scope(st, s)) {
        fprintf(stderr, "Error: Redeclaration of variable '%s'\n", s);
        error_count++;  // Increment error count instead of exiting
        return;  // Don't insert the symbol again
    }
    
    int index = hash(st, s);
    SymbolTableEntry newEntry = malloc(sizeof(struct sym_entry));
    if (!newEntry) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table entry\n");
        exit(EXIT_FAILURE);
    }
    
    newEntry->s = strdup(s);
    newEntry->kind = kind;
    newEntry->type = type ? strdup(type) : strdup("unknown");
    newEntry->table = st;
    newEntry->next = st->tbl[index];
    st->tbl[index] = newEntry;
    st->nEntries++;
}

SymbolTableEntry lookup_symbol(SymbolTable st, char *s) {
    if (!st) return NULL;
    
    int index = hash(st, s);
    SymbolTableEntry entry = st->tbl[index];
    while (entry) {
        if (strcmp(entry->s, s) == 0) return entry;
        entry = entry->next;
    }
    return st->parent ? lookup_symbol(st->parent, s) : NULL;
}

SymbolTableEntry lookup_symbol_current_scope(SymbolTable st, char *s) {
    if (!st) return NULL;
    
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
        error_count++;  // Increment the error count instead of exiting
    }
}

void print_symbols(SymbolTable st) {
    if (!st || !st->scope_name) return;
    
    // Symbol printing was already handled in the caller (process_file)
    // Just print the symbols without the header
    
    // Count total symbols for consistent ordering
    int symbol_count = 0;
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            symbol_count++;
            entry = entry->next;
        }
    }
    
    // If no symbols, just print nothing and return
    if (symbol_count == 0) {
        printf("---\n\n");
        return;
    }
    
    // Collect all symbols for alphabetical sorting
    char **symbols = malloc(symbol_count * sizeof(char*));
    if (!symbols) {
        fprintf(stderr, "Error: Memory allocation failed for symbol sorting\n");
        return;
    }
    
    int idx = 0;
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            symbols[idx++] = entry->s;
            entry = entry->next;
        }
    }
    
    // Simple insertion sort for minimal dependencies
    for (int i = 1; i < symbol_count; i++) {
        char *key = symbols[i];
        int j = i - 1;
        
        while (j >= 0 && strcmp(symbols[j], key) > 0) {
            symbols[j + 1] = symbols[j];
            j--;
        }
        symbols[j + 1] = key;
    }
    
    // Print sorted symbols
    for (int i = 0; i < symbol_count; i++) {
        printf("    %s\n", symbols[i]);
    }
    
    printf("---\n\n");
    free(symbols);
}

void free_symbol_table(SymbolTable st) {
    if (!st) return;
    
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
    free(st->scope_name);
    free(st);
}

SymbolTable create_function_scope(SymbolTable parent, char *func_name) {
    SymbolTable st = mksymtab(50, parent);
    st->scope_name = malloc(strlen(func_name) + 10); // "func " + name + null
    if (st->scope_name) {
        sprintf(st->scope_name, "func %s", func_name);
    }
    return st;
}

void set_package_scope_name(SymbolTable st, char *package_name) {
    if (st->scope_name) {
        free(st->scope_name);
    }
    st->scope_name = malloc(strlen(package_name) + 15); // "package " + name + null
    if (st->scope_name) {
        sprintf(st->scope_name, "package %s", package_name);
    }
}

// Add a function to populate predefined symbols
void add_predefined_symbols(SymbolTable st) {
    // Add predefined types
    insert_symbol(st, "Int", FUNCTION, "Type");
    insert_symbol(st, "Boolean", FUNCTION, "Type");
    insert_symbol(st, "String", FUNCTION, "Type");
    insert_symbol(st, "Unit", FUNCTION, "Type");
    
    // Add predefined functions
    insert_symbol(st, "println", FUNCTION, "Unit");
    insert_symbol(st, "print", FUNCTION, "Unit");
    insert_symbol(st, "readLine", FUNCTION, "String");
}
