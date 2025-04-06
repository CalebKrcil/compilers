#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "ytab.h"
#include "type.h"  // for BOOL_TYPE and our base types

extern int error_count;
extern SymbolTable globalSymtab;
extern SymbolTable currentFunctionSymtab;

void report_semantic_error(const char *msg, int lineno) {
    fprintf(stderr, "Semantic Error at line %d: %s\n", lineno, msg);
    error_count++;
}

/* Check for exact type match by pointer or by comparing basetype values.
   (Ensure that your type pointers for Int, Float/Double, Boolean, etc. are set up correctly.)
*/
int check_type_compatibility(typeptr expected, typeptr actual) {
    if (!expected || !actual)
        return 0;
    if (expected == actual)
        return 1;
    if (expected->basetype == actual->basetype)
        return 1;
    return 0;
}

int is_operator(int prodrule) {
    switch (prodrule) {
        case 114: /* additive_expression with ADD */
        case 115: /* additive_expression with SUB */
        case 118: /* multiplicative_expression with MULT */
        case 119: /* multiplicative_expression with DIV */
        case 120: /* multiplicative_expression with MOD */
            return 1;
        default:
            return 0;
    }
}

int is_null_literal(struct tree *t) {
    if (!t || !t->leaf)
        return 0;
    return (t->leaf->category == NullLiteral);
}

void check_semantics_helper(struct tree *t, SymbolTable current_scope) {
    if (!t)
        return;
    
    // Print basic info for the current node.
    printf("DEBUG: Entering node: %s (prod: %d) at line %d, scope: %p\n", 
           t->symbolname ? t->symbolname : "NULL", t->prodrule, t->lineno, current_scope);

    // If this node is a function declaration (prod 327) and has an attached scope, update current_scope.
    if (t->prodrule == 327 && t->scope != NULL) {
        printf("DEBUG: Function declaration encountered. Updating current scope.\n");
        if (t->kids[0] && t->kids[0]->leaf)
            printf("DEBUG: Function '%s' has scope pointer: %p\n", t->kids[0]->leaf->text, t->scope);
        current_scope = t->scope;
    }
    
    // Recursively process all children first.
    for (int i = 0; i < t->nkids; i++) {
        check_semantics_helper(t->kids[i], current_scope);
    }
    
    // --- Control Structures ---
    if (t->symbolname &&
       (strcmp(t->symbolname, "ifStatement") == 0 ||
        strcmp(t->symbolname, "ifElseStatement") == 0 ||
        strcmp(t->symbolname, "whileStatement") == 0 ||
        strcmp(t->symbolname, "forStatement") == 0)) {
        
        struct tree *cond = NULL;
        if (strcmp(t->symbolname, "forStatement") == 0) {
            // For for-statements, assume the condition is the second child.
            if (t->nkids >= 2)
                cond = t->kids[1];
        } else {
            // For if and while, assume the condition is the first child.
            cond = t->kids[0];
        }
        if (cond && strcmp(cond->symbolname, "Identifier") == 0 && !cond->type) {
            // Try to obtain the identifier text. Sometimes the Identifier node itself is not a leaf.
            char *idText = NULL;
            if (cond->leaf && cond->leaf->text) {
                idText = cond->leaf->text;
            } else if (cond->nkids > 0 && cond->kids[0] && cond->kids[0]->leaf && cond->kids[0]->leaf->text) {
                idText = cond->kids[0]->leaf->text;
            }
            if (idText) {
                printf("DEBUG: Attempting to resolve identifier '%s' in control structure condition.\n", idText);
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry && entry->type) {
                    cond->type = entry->type;
                    printf("DEBUG: Resolved '%s' to type %s\n", idText, typename(entry->type));
                } else {
                    printf("DEBUG: Lookup failed for identifier '%s' in scope %p\n", idText, current_scope);
                }
            } else {
                printf("DEBUG: Could not retrieve identifier text from node '%s'\n", cond->symbolname);
            }
        }
        if (!cond || !cond->type) {
            report_semantic_error("Control structure condition has no type", t->lineno);
        } else if (cond->type->basetype != BOOL_TYPE) {
            report_semantic_error("Control structure condition must be boolean", cond->lineno);
        }
    }
    
    // --- Variable Declarations with Initializers ---
    if (t->symbolname && strcmp(t->symbolname, "variableDeclaration") == 0 && t->nkids == 3) {
        struct tree *declTypeNode = t->kids[1];
        struct tree *initializer = t->kids[2];
        if (!check_type_compatibility(declTypeNode->type, initializer->type)) {
            char errMsg[256];
            snprintf(errMsg, sizeof(errMsg),
                     "Type mismatch in declaration: cannot assign value of type %s to variable of type %s",
                     (initializer->type ? typename(initializer->type) : "none"),
                     (declTypeNode->type ? typename(declTypeNode->type) : "none"));
            report_semantic_error(errMsg, initializer->lineno);
        }
    }
    
    // --- Assignment Checks ---
    if (t->symbolname && strcmp(t->symbolname, "assignment") == 0 && t->nkids >= 2) {
        struct tree *lhs = t->kids[0];
        struct tree *rhs = t->kids[1];
        
        /* If lhs is not directly a variableDeclaration node, enforce immutability */
        if (!(lhs->symbolname && strcmp(lhs->symbolname, "variableDeclaration") == 0)) {
            if (!lhs->is_mutable) {
                report_semantic_error("Assignment to immutable variable", lhs->lineno);
            }
        }
        if (!lhs->is_nullable && is_null_literal(rhs)) {
            report_semantic_error("Assignment of null to non-nullable variable", lhs->lineno);
        }
        if (!check_type_compatibility(lhs->type, rhs->type)) {
            char errMsg[256];
            snprintf(errMsg, sizeof(errMsg),
                     "Type mismatch in assignment: cannot assign value of type %s to variable of type %s",
                     (rhs->type ? typename(rhs->type) : "none"),
                     (lhs->type ? typename(lhs->type) : "none"));
            report_semantic_error(errMsg, rhs->lineno);
        }
    }
    
    // --- Binary Operators ---
    if (is_operator(t->prodrule)) {
        if (t->nkids >= 2) {
            if (!check_type_compatibility(t->kids[0]->type, t->kids[1]->type)) {
                report_semantic_error("Operator operands have incompatible types", t->kids[0]->lineno);
            }
        }
    }
    
    // --- Function Calls (production 122) ---
    if (t->prodrule == 122) {
        SymbolTableEntry func_entry = lookup_symbol(globalSymtab, t->kids[0]->leaf->text);
        if (func_entry) {
            int expected = func_entry->param_count;
            int actual = (t->kids[1] != NULL) ? t->kids[1]->nkids : 0;
            if (expected != actual) {
                report_semantic_error("Function call argument count mismatch", t->kids[0]->lineno);
            }
            for (int i = 0; i < actual; i++) {
                if (!check_type_compatibility(func_entry->param_types[i], t->kids[1]->kids[i]->type)) {
                    report_semantic_error("Function call argument type mismatch", t->kids[1]->kids[i]->lineno);
                }
            }
        }
    }
    
    // --- Unresolved Identifier Resolution ---
    // If this node is a leaf identifier and its type is not set, attempt resolution.
    if (!t->type && (t->leaf && t->leaf->category == Identifier)) {
        char *idText = t->leaf->text;
        if (!idText && t->nkids > 0 && t->kids[0] && t->kids[0]->leaf) {
            idText = t->kids[0]->leaf->text;
        }
        if (idText) {
            SymbolTableEntry entry = lookup_symbol(current_scope, idText);
            if (entry && entry->type) {
                t->type = entry->type;
                printf("DEBUG: Resolved leaf identifier '%s' to type %s\n", idText, typename(entry->type));
            } else {
                report_semantic_error("Undeclared identifier", t->lineno);
            }
        }
    }
}



void check_semantics(struct tree *t) {
    // Start semantic checking from the package or global scope.
    // You may choose to pass in globalSymtab, packageSymtab, or currentFunctionSymtab,
    // depending on how your symbol tables are organized.
    SymbolTable starting_scope = currentFunctionSymtab;
    check_semantics_helper(t, starting_scope);
}