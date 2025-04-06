#include "symtab.h"
#include "type.h"
#include "tree.h"

extern int error_count;

/* Define the global current function symbol table.
   This will be updated when a new function scope is created.
*/
SymbolTable currentFunctionSymtab = NULL;

SymbolTable mksymtab(int nBuckets, SymbolTable parent) {
    SymbolTable st = malloc(sizeof(struct sym_table));
    if (!st) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table\n");
        exit(EXIT_FAILURE);
    }
    st->nBuckets = nBuckets;
    st->nEntries = 0;
    st->parent = parent;
    st->scope_name = NULL; 
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

typeptr typeptr_name(char *type_name) {
    if (!type_name) return null_typeptr;
    if (strcmp(type_name, "Int") == 0 ||
        strcmp(type_name, "Short") == 0 ||
        strcmp(type_name, "Long") == 0 ||
        strcmp(type_name, "Byte") == 0)
        return integer_typeptr;
    else if (strcmp(type_name, "Double") == 0 ||
             strcmp(type_name, "Float") == 0)
        return double_typeptr;
    else if (strcmp(type_name, "Boolean") == 0)
        return boolean_typeptr;
    else if (strcmp(type_name, "String") == 0)
        return string_typeptr;
    else if (strcmp(type_name, "Type") == 0)
        return alctype(CLASSTYPE);
    else if (strcmp(type_name, "Unit") == 0)
        return null_typeptr;
    else
        return alctype(ANY_TYPE);
}

void insert_symbol(SymbolTable st, char *s, SymbolKind kind, typeptr type, int is_mutable, int is_nullable) {
    if (lookup_symbol_current_scope(st, s)) {
        fprintf(stderr, "Error: Redeclaration of variable '%s'\n", s);
        error_count++;  
        return;  
    }
    int index = hash(st, s);
    SymbolTableEntry newEntry = malloc(sizeof(struct sym_entry));
    if (!newEntry) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table entry\n");
        exit(EXIT_FAILURE);
    }
    
    newEntry->s = strdup(s);
    newEntry->kind = kind;
    newEntry->type = type;
    newEntry->table = st;
    newEntry->next = st->tbl[index];
    newEntry->mutable = is_mutable;
    newEntry->nullable = is_nullable;
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
        error_count++; 
    }
}

void print_symbols(SymbolTable st) {
    if (!st || !st->scope_name) return;
    printf("--- symbol table for: %s ---\n", st->scope_name);

    int symbol_count = 0;
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            symbol_count++;
            entry = entry->next;
        }
    }

    if (symbol_count == 0) {
        printf("    (empty)\n---\n\n");
        return;
    }

    char **symbols = malloc(symbol_count * sizeof(char *));
    char **info = malloc(symbol_count * sizeof(char *));  // now holds full info string

    if (!symbols || !info) {
        fprintf(stderr, "Error: Memory allocation failed for symbol sorting\n");
        free(symbols);
        free(info);
        return;
    }

    int idx = 0;
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            symbols[idx] = entry->s;
            char *type_str = (entry->type ? typename(entry->type) : "(none)");
            int total_len = strlen(type_str) + 50;
            info[idx] = malloc(total_len);
            snprintf(info[idx], total_len, "%s (mutable: %s, nullable: %s)",
                     type_str,
                     entry->mutable ? "yes" : "no",
                     entry->nullable ? "yes" : "no");

            idx++;
            entry = entry->next;
        }
    }

    // Insertion sort
    for (int i = 1; i < symbol_count; i++) {
        char *key_sym = symbols[i];
        char *key_info = info[i];
        int j = i - 1;
        while (j >= 0 && strcmp(symbols[j], key_sym) > 0) {
            symbols[j + 1] = symbols[j];
            info[j + 1] = info[j];
            j--;
        }
        symbols[j + 1] = key_sym;
        info[j + 1] = key_info;
    }

    for (int i = 0; i < symbol_count; i++) {
        printf("    %s : %s\n", symbols[i], info[i]);
        free(info[i]);  // free each individual string
    }

    printf("---\n\n");

    free(symbols);
    free(info);
}

static int shared_type(typeptr t) {
    if (t == integer_typeptr ||
        t == double_typeptr ||
        t == boolean_typeptr ||
        t == string_typeptr ||
        t == null_typeptr)
        return 1;
    return 0;
}

void free_symbol_table(SymbolTable st) {
    if (!st) return;
    
    for (int i = 0; i < st->nBuckets; i++) {
        SymbolTableEntry entry = st->tbl[i];
        while (entry) {
            SymbolTableEntry temp = entry;
            entry = entry->next;
            free(temp->s);
            if (!shared_type(temp->type))
                free(temp->type);
            
            if (temp->param_count > 0 && temp->param_types) {
                for (int j = 0; j < temp->param_count; j++) {
                    if (!shared_type(temp->param_types[j]))
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
    st->scope_name = malloc(strlen(func_name) + 10); 
    if (st->scope_name) {
        sprintf(st->scope_name, "func %s", func_name);
    }
    /* Set the global current function symbol table */
    currentFunctionSymtab = st;
    return st;
}

void set_package_scope_name(SymbolTable st, char *package_name) {
    if (st->scope_name) {
        free(st->scope_name);
    }
    st->scope_name = malloc(strlen(package_name) + 15); 
    if (st->scope_name) {
        sprintf(st->scope_name, "package %s", package_name);
    }
}

void add_predefined_symbols(SymbolTable st) {
    insert_symbol(st, "Int", CLASS_TYPE, integer_typeptr, 0, 0);
    insert_symbol(st, "Double", CLASS_TYPE, double_typeptr, 0, 0);
    insert_symbol(st, "Boolean", CLASS_TYPE, boolean_typeptr, 0, 0);
    insert_symbol(st, "String", CLASS_TYPE, string_typeptr, 0, 0);
    insert_symbol(st, "Unit", CLASS_TYPE, null_typeptr, 0, 1);
    
    char *print_params[] = {"String"};
    insert_method_symbol(st, "", "print", typeptr_name("Unit"), 1, print_params);
    
    char *println_params[] = {"String"};
    insert_method_symbol(st, "", "println", typeptr_name("Unit"), 1, println_params);
    
    insert_method_symbol(st, "", "readln", typeptr_name("String"), 0, NULL);
    
    char *get_params[] = {"Int"};
    insert_method_symbol(st, "String", "get", typeptr_name("Char"), 1, get_params);
    
    char *equals_params[] = {"String"};
    insert_method_symbol(st, "String", "equals", typeptr_name("Boolean"), 1, equals_params);
    
    insert_method_symbol(st, "String", "length", typeptr_name("Int"), 0, NULL);
    
    char *toString_params[] = {"Int"};  
    insert_method_symbol(st, "String", "toString", typeptr_name("String"), 1, toString_params);
    
    char *valueOf_params[] = {"Any"};
    insert_method_symbol(st, "String", "valueOf", typeptr_name("String"), 1, valueOf_params);
    
    char *substring_params[] = {"Int", "Int"};
    insert_method_symbol(st, "String", "substring", typeptr_name("String"), 2, substring_params);
    
    insert_symbol(st, "java.util.Random", CLASS_TYPE, typeptr_name("Type"), 0, 0);
    insert_method_symbol(st, "java.util.Random", "nextInt", typeptr_name("Int"), 0, NULL);
    
    insert_symbol(st, "java.lang.Math", CLASS_TYPE, typeptr_name("Type"), 0, 0);
    
    char *abs_params[] = {"Double"};
    insert_method_symbol(st, "java.lang.Math", "abs", typeptr_name("Double"), 1, abs_params);
    
    char *max_params[] = {"Double", "Double"};
    insert_method_symbol(st, "java.lang.Math", "max", typeptr_name("Double"), 2, max_params);
    
    char *min_params[] = {"Double", "Double"};
    insert_method_symbol(st, "java.lang.Math", "min", typeptr_name("Double"), 2, min_params);
    
    char *pow_params[] = {"Double", "Double"};
    insert_method_symbol(st, "java.lang.Math", "pow", typeptr_name("Double"), 2, pow_params);
    
    char *trig_params[] = {"Double"};
    insert_method_symbol(st, "java.lang.Math", "cos", typeptr_name("Double"), 1, trig_params);
    insert_method_symbol(st, "java.lang.Math", "sin", typeptr_name("Double"), 1, trig_params);
    insert_method_symbol(st, "java.lang.Math", "tan", typeptr_name("Double"), 1, trig_params);
}

void insert_method_symbol(SymbolTable st, char *class_name, char *method_name, 
    typeptr return_type, int param_count, char **param_types) {
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

    typeptr func_type = alctype(FUNC_TYPE);
    func_type->u.f.returntype = return_type;
    func_type->u.f.nparams = param_count;

    newEntry->type = func_type;
    newEntry->param_count = param_count;

    if (param_count > 0) {
        newEntry->param_types = malloc(param_count * sizeof(typeptr));
        for (int i = 0; i < param_count; i++) {
            newEntry->param_types[i] = typeptr_name(param_types[i]);
        }
    } else {
        newEntry->param_types = NULL;
    }

    newEntry->table = st;
    newEntry->next = st->tbl[index];
    st->tbl[index] = newEntry;
    st->nEntries++;
}
