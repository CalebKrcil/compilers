#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tree;
struct typeinfo;
typedef struct typeinfo *typeptr;

typedef enum { VARIABLE, FUNCTION, METHOD, CLASS_TYPE } SymbolKind;

typedef struct sym_entry {
    char *s;
    SymbolKind kind;
    typeptr type;           
    int param_count;      
    typeptr *param_types;   
    struct sym_table *table;
    struct sym_entry *next;
} *SymbolTableEntry;

typedef struct sym_table {
    int nBuckets;
    int nEntries;
    struct sym_table *parent;
    struct sym_entry **tbl;
    char *scope_name;  
} *SymbolTable;

SymbolTable mksymtab(int nBuckets, SymbolTable parent);
void insert_symbol(SymbolTable st, char *s, SymbolKind kind, typeptr type);
SymbolTableEntry lookup_symbol(SymbolTable st, char *s);
SymbolTableEntry lookup_symbol_current_scope(SymbolTable st, char *s);
void check_undeclared(SymbolTable st, char *s);
void print_symbols(SymbolTable st);
void free_symbol_table(SymbolTable st);
int hash(SymbolTable st, char *s);
typeptr typeptr_name(char *type_name);
SymbolTable create_function_scope(SymbolTable parent, char *func_name);
void set_package_scope_name(SymbolTable st, char *package_name);
void add_predefined_symbols(SymbolTable st);
void insert_method_symbol(SymbolTable st, char *class_name, char *method_name, typeptr return_type, int param_count, char **param_types);

#endif
