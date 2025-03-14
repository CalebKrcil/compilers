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
    // printf("Inserting symbol: %s, Type: %s\n", s, type);
    // printf("[DEBUG] Storing symbol: %s of type %s in %s\n", s, type, 
           // st->scope_name ? st->scope_name : "(null)");
           
    // Check for duplicates in current scope only
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

    // Print the header for the scope
    printf("--- symbol table for: %s ---\n", st->scope_name);

    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            // printf("[BUCKET %d] %s : %s\n", i, entry->s, entry->type);
            entry = entry->next;
        }
    }

    // Count total symbols
    int symbol_count = 0;
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            symbol_count++;
            entry = entry->next;
        }
    }

    // If no symbols, print an empty message and return
    if (symbol_count == 0) {
        printf("    (empty)\n---\n\n");
        return;
    }

    // Allocate memory for sorting symbols
    char **symbols = malloc(symbol_count * sizeof(char *));
    char **types = malloc(symbol_count * sizeof(char *));
    
    if (!symbols || !types) {
        fprintf(stderr, "Error: Memory allocation failed for symbol sorting\n");
        free(symbols);
        free(types);
        return;
    }

    // Collect symbols and types
    int idx = 0;
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            symbols[idx] = entry->s;
            types[idx] = entry->type;
            idx++;
            entry = entry->next;
        }
    }

    // Sort symbols alphabetically using insertion sort
    for (int i = 1; i < symbol_count; i++) {
        char *key_sym = symbols[i];
        char *key_type = types[i];
        int j = i - 1;

        while (j >= 0 && strcmp(symbols[j], key_sym) > 0) {
            symbols[j + 1] = symbols[j];
            types[j + 1] = types[j];
            j--;
        }
        symbols[j + 1] = key_sym;
        types[j + 1] = key_type;
    }

    // Print sorted symbols with types
    for (int i = 0; i < symbol_count; i++) {
        printf("    %s : %s\n", symbols[i], types[i]);
    }

    printf("---\n\n");

    // Free allocated memory
    free(symbols);
    free(types);
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
            
            // Free parameter types
            if (temp->param_count > 0 && temp->param_types) {
                for (int j = 0; j < temp->param_count; j++) {
                    free(temp->param_types[j]);
                }
                free(temp->param_types);
            }
            
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

void add_predefined_symbols(SymbolTable st) {
    // Basic types
    insert_symbol(st, "Int", CLASS_TYPE, "Type");
    insert_symbol(st, "Double", CLASS_TYPE, "Type");
    insert_symbol(st, "Boolean", CLASS_TYPE, "Type");
    insert_symbol(st, "String", CLASS_TYPE, "Type");
    insert_symbol(st, "Char", CLASS_TYPE, "Type");
    insert_symbol(st, "Unit", CLASS_TYPE, "Type");
    
    // Global functions - using new helper function for consistency
    char *print_params[] = {"String"};
    insert_method_symbol(st, "", "print", "Unit", 1, print_params);
    
    char *println_params[] = {"String"};
    insert_method_symbol(st, "", "println", "Unit", 1, println_params);
    
    // readln with no parameters
    insert_method_symbol(st, "", "readln", "String", 0, NULL);
    
    // String methods
    char *get_params[] = {"Int"};
    insert_method_symbol(st, "String", "get", "Char", 1, get_params);
    
    char *equals_params[] = {"String"};
    insert_method_symbol(st, "String", "equals", "Boolean", 1, equals_params);
    
    // No parameters for length
    insert_method_symbol(st, "String", "length", "Int", 0, NULL);
    
    char *toString_params[] = {"Int"};  // Assuming it converts an int to string
    insert_method_symbol(st, "String", "toString", "String", 1, toString_params);
    
    // Assuming valueOf converts various types to String
    char *valueOf_params[] = {"Any"};
    insert_method_symbol(st, "String", "valueOf", "String", 1, valueOf_params);
    
    char *substring_params[] = {"Int", "Int"};
    insert_method_symbol(st, "String", "substring", "String", 2, substring_params);
    
    // java.util.Random methods
    insert_symbol(st, "java.util.Random", CLASS_TYPE, "Type");
    insert_method_symbol(st, "java.util.Random", "nextInt", "Int", 0, NULL);
    
    // java.lang.Math methods
    insert_symbol(st, "java.lang.Math", CLASS_TYPE, "Type");
    
    char *abs_params[] = {"Double"};
    insert_method_symbol(st, "java.lang.Math", "abs", "Double", 1, abs_params);
    
    char *max_params[] = {"Double", "Double"};
    insert_method_symbol(st, "java.lang.Math", "max", "Double", 2, max_params);
    
    char *min_params[] = {"Double", "Double"};
    insert_method_symbol(st, "java.lang.Math", "min", "Double", 2, min_params);
    
    char *pow_params[] = {"Double", "Double"};
    insert_method_symbol(st, "java.lang.Math", "pow", "Double", 2, pow_params);
    
    char *trig_params[] = {"Double"};
    insert_method_symbol(st, "java.lang.Math", "cos", "Double", 1, trig_params);
    insert_method_symbol(st, "java.lang.Math", "sin", "Double", 1, trig_params);
    insert_method_symbol(st, "java.lang.Math", "tan", "Double", 1, trig_params);
}

void insert_method_symbol(SymbolTable st, char *class_name, char *method_name, 
    char *return_type, int param_count, char **param_types) {
    char full_name[256];
    sprintf(full_name, "%s.%s", class_name, method_name);

    int index = hash(st, full_name);
    SymbolTableEntry newEntry = malloc(sizeof(struct sym_entry));
    if (!newEntry) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table entry\n");
        exit(EXIT_FAILURE);
    }

    newEntry->s = strdup(full_name);
    newEntry->kind = METHOD;
    newEntry->type = return_type ? strdup(return_type) : strdup("unknown");
    newEntry->param_count = param_count;

    // Allocate and copy parameter types
    if (param_count > 0) {
        newEntry->param_types = malloc(param_count * sizeof(char*));
        for (int i = 0; i < param_count; i++) {
            newEntry->param_types[i] = strdup(param_types[i]);
        }
    } else {
        newEntry->param_types = NULL;
    }

    newEntry->table = st;
    newEntry->next = st->tbl[index];
    st->tbl[index] = newEntry;
    st->nEntries++;
}