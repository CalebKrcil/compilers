#include "tree.h"
#include "k0gram.tab.h"
#include "ytab.h"
#include "symtab.h"
#include "type.h"

#include <stdarg.h>
#include <string.h>

extern int yylineno;
extern char *current_filename;
extern YYSTYPE yylval;
extern int error_count;


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
    } else if (category == StringLiteral) {
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

char *get_type_name(struct tree *type_node) {
    if (!type_node) return "unknown";
    if (type_node->leaf) return type_node->leaf->text;
    if (type_node->nkids > 0 && type_node->kids[0] && type_node->kids[0]->leaf) {
        return type_node->kids[0]->leaf->text;
    }
    return "unknown";
}

void computeFunctionParameters(struct tree *fvp, int *count, typeptr **types) {
    *count = 0;
    *types = NULL;
    if (!fvp || fvp->nkids < 1 || !fvp->kids[0])
        return;
    struct tree *plist = fvp->kids[0];
    if (strcmp(plist->symbolname, "functionValueParameter") == 0) {
         *count = 1;
         *types = malloc(sizeof(typeptr));
         if (!(*types)) {
             fprintf(stderr, "Memory allocation failed in computeFunctionParameters\n");
             exit(1);
         }
         (*types)[0] = plist->type; // Assume the parameter node’s type was already set.
    } else if (strcmp(plist->symbolname, "functionParameterList") == 0) {
         *count = plist->nkids;
         *types = malloc(plist->nkids * sizeof(typeptr));
         if (!(*types)) {
             fprintf(stderr, "Memory allocation failed in computeFunctionParameters\n");
             exit(1);
         }
         for (int i = 0; i < plist->nkids; i++) {
             (*types)[i] = plist->kids[i]->type;
         }
    }
}

FuncSymbolTableList printsyms(struct tree *t, SymbolTable st) {
    if (!t) return NULL;

    static FuncSymbolTableList func_list_head = NULL;
    static FuncSymbolTableList func_list_tail = NULL;

    SymbolTable current_scope = st;

    if (t->prodrule == 327) {  
        char *func_name = NULL;
        char *return_type = "Unit"; 

    if (t->nkids >= 1 && t->kids[0] && t->kids[0]->leaf) {
        func_name = t->kids[0]->leaf->text;
    }

    if (t->nkids >= 3 && t->kids[2] != NULL) {
        return_type = get_type_name(t->kids[2]);
    }

    if (func_name) {
        // Insert the function symbol with a placeholder type (NULL for now).
        insert_symbol(st, func_name, FUNCTION, NULL, 0, 0);
    
        // Compute the function's parameter information from the functionValueParameters subtree.
        int paramCount = 0;
        typeptr *paramTypes = NULL;
        // t->kids[1] is the functionValueParameters node.
        computeFunctionParameters(t->kids[1], &paramCount, &paramTypes);
    
        // Use the already-set return_type variable.
        typeptr retType = typeptr_name(return_type);
    
        // Create a new function type.
        typeptr func_type = alctype(FUNC_TYPE);
        func_type->u.f.returntype = retType;
        func_type->u.f.nparams = paramCount;
    
        // Lookup the inserted function and update its entry with the parameter info and function type.
        SymbolTableEntry func_entry = lookup_symbol(st, func_name);
        if (func_entry) {
             func_entry->param_count = paramCount;
             func_entry->param_types = paramTypes;
             func_entry->type = func_type;
             // Debug output:
            printf("DEBUG: Function '%s' recorded with %d parameter(s), return type: %s\n",
                    func_name, paramCount, typename(func_type->u.f.returntype));
             for (int i = 0; i < paramCount; i++) {
                printf("DEBUG: Parameter %d has type %s\n", i, typename(paramTypes[i]));
             }
        }
    
        // Create a new function scope.
        current_scope = create_function_scope(st, func_name);
        t->scope = current_scope;
    
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

        if (t->nkids >= 2 && t->kids[1]) {
            struct tree *params_container = t->kids[1];  
            if (strcmp(params_container->symbolname, "functionValueParameters") == 0) {
                if (params_container->nkids > 0 && params_container->kids[0] &&
                    strcmp(params_container->kids[0]->symbolname, "functionValueParameter") == 0) {
                    
                    struct tree *param_node = params_container->kids[0];
                    if (param_node->nkids >= 2) {  
                        char *param_name = NULL;
                        if (param_node->kids[0] && param_node->kids[0]->leaf) {
                            param_name = param_node->kids[0]->leaf->text;
                        }
                        
                        char *param_type = "unknown";
                        if (param_node->kids[1] && strcmp(param_node->kids[1]->symbolname, "type") == 0) {
                            struct tree *type_node = param_node->kids[1];
                            if (type_node->nkids > 0 && type_node->kids[0] && type_node->kids[0]->leaf) {
                                param_type = type_node->kids[0]->leaf->text;
                            } else if (type_node->leaf) {
                                param_type = type_node->leaf->text;
                            }
                        }
                        
                        if (param_name) {
                            // printf("Inserting single function parameter: %s of type %s into %s\n",
                                // param_name, param_type, current_scope->scope_name);
                            insert_symbol(current_scope, param_name, VARIABLE, typeptr_name(param_type), 0, param_node->kids[1]->is_nullable);
                        }
                    }
                }
                else if (params_container->nkids > 0 && params_container->kids[0] &&
                        strcmp(params_container->kids[0]->symbolname, "functionParameterList") == 0) {
                    
                    struct tree *params_list = params_container->kids[0];
                    
                    for (int i = 0; i < params_list->nkids; i++) {
                        struct tree *param_node = params_list->kids[i];
                        if (!param_node) continue;
                        
                        if (strcmp(param_node->symbolname, "functionValueParameter") == 0) {
                            if (param_node->nkids >= 2) {  
                                char *param_name = NULL;
                                if (param_node->kids[0] && param_node->kids[0]->leaf) {
                                    param_name = param_node->kids[0]->leaf->text;
                                }
                                char *param_type = "unknown";
                                if (param_node->kids[1] && strcmp(param_node->kids[1]->symbolname, "type") == 0) {
                                    struct tree *type_node = param_node->kids[1];
                                    if (type_node->nkids > 0 && type_node->kids[0] && type_node->kids[0]->leaf) {
                                        param_type = type_node->kids[0]->leaf->text;
                                    } else if (type_node->leaf) {
                                        param_type = type_node->leaf->text;
                                    }
                                }
                                if (param_name) {
                                    // printf("Inserting multi function parameter: %s of type %s into %s\n",
                                        // param_name, param_type, current_scope->scope_name);
                                    insert_symbol(current_scope, param_name, VARIABLE, typeptr_name(param_type),0, param_node->kids[1]->is_nullable);
                                }
                            }
                        }
                    }
                }
                else {
                    for (int i = 0; i < params_container->nkids; i++) {
                        struct tree *child = params_container->kids[i];
                        if (!child) continue;
                        
                        if (strcmp(child->symbolname, "functionValueParameter") == 0) {
                            if (child->nkids >= 2) {
                                char *param_name = NULL;
                                if (child->kids[0] && child->kids[0]->leaf) {
                                    param_name = child->kids[0]->leaf->text;
                                }
                                
                                char *param_type = "unknown";
                                if (child->kids[1] && strcmp(child->kids[1]->symbolname, "type") == 0) {
                                    struct tree *type_node = child->kids[1];
                                    if (type_node->nkids > 0 && type_node->kids[0] && type_node->kids[0]->leaf) {
                                        param_type = type_node->kids[0]->leaf->text;
                                    } else if (type_node->leaf) {
                                        param_type = type_node->leaf->text;
                                    }
                                }
                                
                                if (param_name) {
                                    // printf("Inserting fallback function parameter: %s of type %s into %s\n",
                                        // param_name, param_type, current_scope->scope_name);
                                    insert_symbol(current_scope, param_name, VARIABLE, typeptr_name(param_type), 0, child->kids[1]->is_nullable);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    
    else if (strcmp(t->symbolname, "variableDeclaration") == 0 ||
         strcmp(t->symbolname, "constVariableDeclaration") == 0 ||
         strcmp(t->symbolname, "arrayDeclaration") == 0) {  // Add this line
        if (t->nkids >= 1 && t->kids[0] && t->kids[0]->leaf) {
            char *var_name = t->kids[0]->leaf->text;
            /* Use the type already set on the node, if available */
            typeptr var_type = t->type;
            /* Otherwise, search the children for a type node */
            if (!var_type) {
                // Try to find a genericType node for arrays
                for (int i = 1; i < t->nkids; i++) {
                    if (t->kids[i] && strcmp(t->kids[i]->symbolname, "type") == 0) {
                        var_type = typeptr_name(get_type_name(t->kids[i]));
                        break;
                    } else if (t->kids[i] && strcmp(t->kids[i]->symbolname, "genericType") == 0) {
                        // Handle array type differently - we already have the type set on the node itself
                        var_type = t->kids[i]->type;
                        break;
                    }
                }
            }
            /* If still missing, default to Any */
            if (!var_type) {
                var_type = typeptr_name("Any");
            }
            insert_symbol(current_scope, var_name, VARIABLE, var_type, t->is_mutable, t->is_nullable);
        }
    }

    else if (strcmp(t->symbolname, "arrayAssignmentDeclaration") == 0) {
        char *var_name = t->kids[0]->leaf->text;      // $2 = variable name
        typeptr var_type = t->type;                   // already set during parse
        int is_mutable = t->is_mutable;
        int is_nullable = t->is_nullable;
    
        insert_symbol(current_scope, var_name, VARIABLE, var_type, is_mutable, is_nullable);
    }

    else if (t->prodrule == 280) {  
        if (t->nkids >= 1 && t->kids[0] && t->kids[0]->leaf) {
            char *var_name = t->kids[0]->leaf->text;

            if (!lookup_symbol_current_scope(current_scope, var_name)) {
                // printf("Implicitly declaring variable: %s in %s\n",
                        // var_name, current_scope->scope_name);
                insert_symbol(current_scope, var_name, VARIABLE, alctype(ANY_TYPE), 1, 1);  
            }
        }
    }

    else if (t->leaf && t->leaf->category == Identifier) {
        if (t->prodrule != 329 && t->prodrule != 330 && t->prodrule != 327) {
            check_undeclared(current_scope, t->leaf->text);
        }
    }

    for (int i = 0; i < t->nkids; i++) {
        printsyms(t->kids[i], current_scope);
    }
    
    return func_list_head;
}

void free_func_symtab_list(FuncSymbolTableList list) {
    while (list) {
        FuncSymbolTableList next = list->next;
        free(list);
        list = next;
    }
}



void freetoken(struct token *t) {
    if (!t) return;
    free(t->text);
    free(t->filename);
    if (t->category == StringLiteral) {
        free(t->value.sval);
    }
    free(t);
}


static int serial = 0;
struct tree *alctree(int prodrule, char *symbolname, int nkids, ...) {
    struct tree *t = malloc(sizeof(struct tree));
    if (!t) {
        fprintf(stderr, "Memory allocation failed for tree node\n");
        exit(EXIT_FAILURE);
    }

    t->id = serial++;  
    t->prodrule = prodrule;
    t->symbolname = strdup(symbolname);
    t->nkids = nkids;
    t->leaf = NULL;
    t->type = NULL;
    t->is_mutable = 0;
    t->is_nullable = 0;
    t->lineno = yylineno;
    t->scope = NULL;

    va_list args;
    va_start(args, nkids);
    for (int i = 0; i < nkids; i++) {
        t->kids[i] = va_arg(args, struct tree *);
    }
    va_end(args);

    return t;
}


void freetree(struct tree *t) {
    if (!t) return;

    for (int i = 0; i < t->nkids; i++) {
        freetree(t->kids[i]);
        t->kids[i] = NULL;  
    }

    if (t->leaf) {
        freetoken(t->leaf);
        t->leaf = NULL;
    }

    if (t->symbolname) {
        free(t->symbolname);
        t->symbolname = NULL;
    }

    free(t);
}

void printtree(struct tree *t, int depth) {
    if (!t) return;

    for (int i = 0; i < depth; i++) printf("  ");

    if (t->leaf) {
        if (t->leaf->category == IntegerLiteral) {
            printf("Leaf: IntegerLiteral - %d, Integer code: %d", t->leaf->value.ival, t->leaf->category);
        } else if (t->leaf->category == RealLiteral) {
            printf("Leaf: RealLiteral - %lf, Integer code: %d", t->leaf->value.dval, t->leaf->category);
        } else if (t->leaf->category == StringLiteral) {
            printf("Leaf: StringLiteral - '%s', Integer code: %d", t->leaf->value.sval, t->leaf->category);
        } else {
            printf("Leaf: %s - %s, Integer code: %d", t->symbolname, t->leaf->text, t->leaf->category);
        }
    } else {
        printf("Node: %s (prod: %d)", t->symbolname, t->prodrule);
    }

    if (t->type) {
        printf(" type: %s", typename(t->type));
    }

    // Always print these flags for clarity
    printf(" [mutable=%d nullable=%d]\n", t->is_mutable, t->is_nullable);

    for (int i = 0; i < t->nkids; i++) {
        printtree(t->kids[i], depth + 1);
    }
}


char *escape(char *s) {
    if (!s) return strdup("NULL");
    
    int len = strlen(s);
    char *s2 = malloc(len * 2 + 1);  
    if (!s2) {
        fprintf(stderr, "Memory allocation failed for escaped string\n");
        exit(EXIT_FAILURE);
    }
    
    char *p = s2;

    for (int i = 0; i < len; i++) {
        if (s[i] == '"' || s[i] == '\\') {  
            *p++ = '\\';
        }
        *p++ = s[i];
    }
    *p = '\0';  
    return s2;
}

char *pretty_print_name(struct tree *t) {
    if (!t) return strdup("NULL");
    
    char *s2 = malloc(256);  
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
    
    print_branch(t, f);
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] != NULL) {
            fprintf(f, "N%d -> N%d;\n", t->id, t->kids[i]->id);
            print_graph2(t->kids[i], f);
        } else { 
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