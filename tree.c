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

// Recursive function to traverse and print symbols
void printsyms(struct tree *t, SymbolTable st) {
    if (!t) return;

    // Handle variable declarations
    if (t->prodrule == VAR || t->prodrule == VAL) {
        if (t->nkids >= 1) {
            // The variable name should be in the first child
            struct tree *var_name_node = t->kids[0];
            if (var_name_node && var_name_node->leaf) {
                // Get the variable name
                char *var_name = var_name_node->leaf->text;
                // Default type is "unknown"
                char *var_type = "unknown";
                
                // Try to get type if available (usually second child)
                if (t->nkids >= 2 && t->kids[1]) {
                    var_type = get_type_name(t->kids[1]);
                }
                
                // Insert the variable into the symbol table
                printf("Found variable: %s of type %s\n", var_name, var_type);
                insert_symbol(st, var_name, VARIABLE, var_type);
            }
        }
    }
    // Handle function declarations
    else if (t->prodrule == FUN) {
        char *funcName = NULL;
        char *returnType = "Unit";  // Default return type is Unit
        
        // First child should be the function name
        if (t->nkids >= 1 && t->kids[0] && t->kids[0]->leaf) {
            funcName = t->kids[0]->leaf->text;
        }
        
        // Return type is usually the third child
        if (t->nkids >= 3) {
            returnType = get_type_name(t->kids[2]);
        }

        if (funcName) {
            insert_symbol(st, funcName, FUNCTION, returnType);
        }
        
        // Process function parameters
        if (t->nkids >= 2 && t->kids[1]) {
            struct tree *params = t->kids[1];
            for (int i = 0; i < params->nkids; i++) {
                struct tree *param = params->kids[i];
                if (param->nkids >= 2) {
                    char *paramName = param->kids[0]->leaf->text;
                    char *paramType = get_type_name(param->kids[1]);
                    insert_symbol(st, paramName, VARIABLE, paramType);
                }
            }
        }
    }
    // Handle identifiers in expressions (check for undeclared variables)
    else if (t->prodrule == FUN) {
        if (t->nkids >= 1) {
            // The function name should be in the first child
            struct tree *func_name_node = t->kids[0];
            if (func_name_node && func_name_node->leaf) {
                char *func_name = func_name_node->leaf->text;
                // Default return type is "Unit"
                char *return_type = "Unit";
                
                // Try to get return type if available
                if (t->nkids >= 3 && t->kids[2]) {
                    return_type = get_type_name(t->kids[2]);
                }
                
                // Insert the function into the symbol table
                printf("Found function: %s returning %s\n", func_name, return_type);
                insert_symbol(st, func_name, FUNCTION, return_type);
                
                // Process parameters if available
                if (t->nkids >= 2 && t->kids[1]) {
                    struct tree *params_node = t->kids[1];
                    for (int i = 0; i < params_node->nkids; i++) {
                        struct tree *param_node = params_node->kids[i];
                        if (param_node && param_node->nkids >= 2) {
                            // Parameter name and type
                            char *param_name = param_node->kids[0]->leaf->text;
                            char *param_type = get_type_name(param_node->kids[1]);
                            
                            printf("Found parameter: %s of type %s\n", param_name, param_type);
                            insert_symbol(st, param_name, VARIABLE, param_type);
                        }
                    }
                }
            }
        }
    }
    // Check for variable usage (identifier references)
    else if (t->leaf && t->leaf->category == Identifier) {
        // Make sure this is not part of a declaration
        if (t->prodrule != VAR && t->prodrule != VAL && t->prodrule != FUN) {
            // Check if this identifier is declared
            check_undeclared(st, t->leaf->text);
        }
    }

    // Recursively visit child nodes
    for (int i = 0; i < t->nkids; i++) {
        printsyms(t->kids[i], st);
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
