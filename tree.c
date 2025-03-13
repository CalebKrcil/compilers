#include "tree.h"
#include "k0gram.tab.h"
#include "ytab.h"
#include "symtab.h"

#include <stdarg.h>
#include <string.h>

extern int yylineno;
extern char *current_filename;
extern YYSTYPE yylval;
extern int error_count;

/**
 * Allocates a token and initializes it based on its category.
 */
int alctoken(int category, char *text) {
    yylval.treeptr = malloc(sizeof(struct tree));
    if (!yylval.treeptr) {
        fprintf(stderr, "Memory allocation failed for tree node\n");
        exit(EXIT_FAILURE);
    }

    yylval.treeptr->prodrule = category;
    yylval.treeptr->symbolname = strdup(text);
    yylval.treeptr->nkids = 0;
    yylval.treeptr->leaf = malloc(sizeof(struct token));

    struct token *tok = yylval.treeptr->leaf;
    tok->category = category;
    tok->text = strdup(text);
    tok->lineno = yylineno;
    tok->filename = strdup(current_filename);

    if (category == IntegerLiteral) {
        tok->value.ival = atoi(text);
    } else if (category == RealLiteral) {
        tok->value.dval = atof(text);
    } else if (category == CharacterLiteral) {
        tok->value.sval = strdup(text);
    } else {
        tok->value.sval = NULL;
    }

    return category;
}

void printsymbol(char *s) {
    printf("%s\n", s);
    fflush(stdout);
}

// Helper function to determine variable type
char *get_type_name(struct tree *type_node) {
    if (!type_node) return "unknown";
    if (type_node->leaf) return type_node->leaf->text;
    // If it's not a direct leaf, try to get the type name from a child node
    if (type_node->nkids > 0 && type_node->kids[0] && type_node->kids[0]->leaf) {
        return type_node->kids[0]->leaf->text;
    }
    return "unknown";
}

FuncSymbolTableList printsyms(struct tree *t, SymbolTable st) {
    if (!t) return NULL;

    // Create a list to track function symbol tables
    static FuncSymbolTableList func_list_head = NULL;
    static FuncSymbolTableList func_list_tail = NULL;

    // Track the current scope (default is the given symbol table)
    SymbolTable current_scope = st;

    // Handle function declarations
    if (t->prodrule == 327) {  // Function declaration
        char *func_name = NULL;
        char *return_type = "Unit"; // Default return type

        // First child should be the function name
        if (t->nkids >= 1 && t->kids[0] && t->kids[0]->leaf) {
            func_name = t->kids[0]->leaf->text;
        }

        // Return type is usually the third child
        if (t->nkids >= 3) {
            return_type = get_type_name(t->kids[2]);
        }

        if (func_name) {
            //printf("Processing function: %s\n", func_name);
            insert_symbol(st, func_name, FUNCTION, return_type);
            //printf("Created function scope for: %s\n", func_name);

            // Create a new symbol table for this function scope
            current_scope = create_function_scope(st, func_name);
            
            // Add to our function symbol table list
            FuncSymbolTableList new_node = malloc(sizeof(struct func_symtab_list));
            if (new_node) {
                new_node->symtab = current_scope;
                new_node->next = NULL;
                
                if (func_list_tail) {
                    func_list_tail->next = new_node;
                    func_list_tail = new_node;
                } else {
                    func_list_head = func_list_tail = new_node;
                }
            }
        }

        // Process function parameters (parameters belong in function scope)
        if (t->nkids >= 2 && t->kids[1]) {
            struct tree *params_node = t->kids[1];
            for (int i = 0; i < params_node->nkids; i++) {
                struct tree *param_node = params_node->kids[i];
                if (param_node && param_node->nkids >= 2) {
                    char *param_name = param_node->kids[0]->leaf->text;
                    char *param_type = get_type_name(param_node->kids[1]);

                    // printf("Inserting function parameter: %s of type %s into %s\n",
                    //        param_name, param_type, current_scope->scope_name);
                    insert_symbol(current_scope, param_name, VARIABLE, param_type);
                }
            }
        }
    }

    // Handle variable declarations (in the correct scope)
    else if (t->prodrule == 329 || t->prodrule == 330) {  // VAR or VAL declaration
        if (t->nkids >= 1 && t->kids[0] && t->kids[0]->leaf) {
            char *var_name = t->kids[0]->leaf->text;
            char *var_type = "unknown";

            // Try to find type in siblings
            if (t->nkids >= 2) {
                for (int i = 1; i < t->nkids; i++) {
                    if (t->kids[i] && strcmp(t->kids[i]->symbolname, "type") == 0) {
                        var_type = get_type_name(t->kids[i]);
                        break;
                    }
                }
            }

            // Insert into the correct scope (function or global)
            // printf("Inserting variable: %s of type %s into %s\n",
            //        var_name, var_type, current_scope->scope_name);
            insert_symbol(current_scope, var_name, VARIABLE, var_type);
        }
    }

    // Handle assignments (only insert if variable is undeclared in the current scope)
    else if (t->prodrule == 280) {  // Assignment
        if (t->nkids >= 1 && t->kids[0] && t->kids[0]->leaf) {
            char *var_name = t->kids[0]->leaf->text;

            // If variable is not already declared in the current scope, insert it
            if (!lookup_symbol_current_scope(current_scope, var_name)) {
                // printf("Implicitly declaring variable: %s in %s\n",
                //        var_name, current_scope->scope_name);
                insert_symbol(current_scope, var_name, VARIABLE, "unknown");  // Type unknown at this stage
            }
        }
    }

    // Check for variable usage (identifier references)
    else if (t->leaf && t->leaf->category == Identifier) {
        // Ensure it's not a declaration
        if (t->prodrule != 329 && t->prodrule != 330 && t->prodrule != 327) {
            check_undeclared(current_scope, t->leaf->text);
        }
    }

    // Recursively visit child nodes using the correct scope
    for (int i = 0; i < t->nkids; i++) {
        printsyms(t->kids[i], current_scope);
    }
    
    return func_list_head;
}

// Add this helper function to free the function symbol table list
void free_func_symtab_list(FuncSymbolTableList list) {
    while (list) {
        FuncSymbolTableList next = list->next;
        // Note: We don't free the symbol tables themselves here,
        // as that's handled elsewhere
        free(list);
        list = next;
    }
}


/**
 * Frees memory allocated for a token.
 */
void freetoken(struct token *t) {
    if (!t) return;
    free(t->text);
    free(t->filename);
    if (t->category == CharacterLiteral) {
        free(t->value.sval);
    }
    free(t);
}

/**
 * Allocates a new tree node with the given production rule, symbol name,
 * and child nodes.
 */
static int serial = 0;
struct tree *alctree(int prodrule, char *symbolname, int nkids, ...) {
    struct tree *t = malloc(sizeof(struct tree));
    if (!t) {
        fprintf(stderr, "Memory allocation failed for tree node\n");
        exit(EXIT_FAILURE);
    }

    t->id = serial++;  // Ensure every node gets a unique ID
    t->prodrule = prodrule;
    t->symbolname = strdup(symbolname);
    t->nkids = nkids;
    t->leaf = NULL;

    va_list args;
    va_start(args, nkids);
    for (int i = 0; i < nkids; i++) {
        t->kids[i] = va_arg(args, struct tree *);
    }
    va_end(args);

    return t;
}

/**
 * Frees memory allocated for a syntax tree.
 */
void freetree(struct tree *t) {
    if (!t) return;

    // Free children recursively
    for (int i = 0; i < t->nkids; i++) {
        freetree(t->kids[i]);
        t->kids[i] = NULL;  // Avoid dangling pointers
    }

    // Free the token if it's a leaf
    if (t->leaf) {
        freetoken(t->leaf);
        t->leaf = NULL;
    }

    // Free symbol name (strdup'd string)
    if (t->symbolname) {
        free(t->symbolname);
        t->symbolname = NULL;
    }

    // Free the tree node itself
    free(t);
}

/**
 * Recursively prints the syntax tree.
 */
void printtree(struct tree *t, int depth) {
    if (!t) return;

    // Print indentation based on depth
    for (int i = 0; i < depth; i++) printf("  ");

    if (t->leaf) {
        // Print different representations for literals
        if (t->leaf->category == IntegerLiteral) {
            printf("Leaf: IntegerLiteral - %d, Integer code: %d\n", t->leaf->value.ival, t->leaf->category);
        } else if (t->leaf->category == RealLiteral) {
            printf("Leaf: RealLiteral - %lf, Integer code: %d\n", t->leaf->value.dval, t->leaf->category);
        } else if (t->leaf->category == CharacterLiteral) {
            printf("Leaf: CharacterLiteral - '%s', Integer code: %d\n", t->leaf->value.sval, t->leaf->category);
        } else {
            printf("Leaf: %s - %s, Integer code: %d\n", t->symbolname, t->leaf->text, t->leaf->category);
        }
    } else {
        // Print node names
        printf("Node: %s\n", t->symbolname);
        for (int i = 0; i < t->nkids; i++) {
            printtree(t->kids[i], depth + 1);
        }
    }
}

char *escape(char *s) {
    if (!s) return strdup("NULL");
    
    int len = strlen(s);
    char *s2 = malloc(len * 2 + 1);  // Allocate extra space for escaping
    if (!s2) {
        fprintf(stderr, "Memory allocation failed for escaped string\n");
        exit(EXIT_FAILURE);
    }
    
    char *p = s2;

    for (int i = 0; i < len; i++) {
        if (s[i] == '"' || s[i] == '\\') {  // Escape quotes and backslashes
            *p++ = '\\';
        }
        *p++ = s[i];
    }
    *p = '\0';  // Null-terminate the new string
    return s2;
}

char *pretty_print_name(struct tree *t) {
    if (!t) return strdup("NULL");
    
    char *s2 = malloc(256);  // More generous allocation
    if (!s2) {
        fprintf(stderr, "Memory allocation failed for node name\n");
        exit(EXIT_FAILURE);
    }
    
    if (t->leaf == NULL) {
        sprintf(s2, "%s#%d", t->symbolname, t->prodrule);
    } else {
        char *escaped_text = escape(t->leaf->text);
        sprintf(s2, "%s:%d", escaped_text, t->leaf->category);
        free(escaped_text);
    }
    return s2;
}
 
void print_branch(struct tree *t, FILE *f) {
    if (!t) return;
    
    char *name = pretty_print_name(t);
    fprintf(f, "N%d [shape=box label=\"%s\"];\n", t->id, name);
    free(name);
}
  
void print_leaf(struct tree *t, FILE *f) {
    if (!t || !t->leaf) return;
    
    char *escaped_text = escape(t->leaf->text);
    fprintf(f, "N%d [shape=box style=dotted label=\"%s\\n", t->id, "Leaf");
    fprintf(f, "text = %s \\l lineno = %d \\l\"];\n", escaped_text, t->leaf->lineno);
    free(escaped_text);
}

void print_graph2(struct tree *t, FILE *f) {
    if (!t) return;
    
    if (t->leaf != NULL) {
        print_leaf(t, f);
        return;
    }
    
    /* not a leaf ==> internal node */
    print_branch(t, f);
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] != NULL) {
            fprintf(f, "N%d -> N%d;\n", t->id, t->kids[i]->id);
            print_graph2(t->kids[i], f);
        } else { /* NULL kid, epsilon production or something */
            int temp_id = serial++;
            fprintf(f, "N%d -> N%d;\n", t->id, temp_id);
            fprintf(f, "N%d [label=\"%s\"];\n", temp_id, "Empty rule");
        }
    }
}
 
void print_graph(struct tree *t, char *filename) {
    if (!t) {
        fprintf(stderr, "Error: Cannot generate DOT file for NULL tree\n");
        return;
    }
    
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file %s for writing\n", filename);
        return;
    }
    
    fprintf(f, "digraph {\n");
    print_graph2(t, f);
    fprintf(f, "}\n");
    fclose(f);
}
