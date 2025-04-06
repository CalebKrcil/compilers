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

/* Check control conditions for if, while, and for statements.
   Adjust child indexes as needed by your AST structure.
*/
void check_control_conditions(struct tree *t) {
    if (!t)
        return;

    /* For if and if-else statements, assume the condition is the first child */
    if (t->symbolname &&
       (strcmp(t->symbolname, "ifStatement") == 0 ||
        strcmp(t->symbolname, "ifElseStatement") == 0)) {
        if (t->nkids < 1 || t->kids[0] == NULL) {
            report_semantic_error("If-statement missing condition", t->lineno);
        } else {
            struct tree *cond = t->kids[0];
            // Only resolve if the condition node is an Identifier
            if (strcmp(cond->symbolname, "Identifier") == 0 && !cond->type && cond->leaf && cond->leaf->text) {
                SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, cond->leaf->text);
                if (entry && entry->type) {
                    cond->type = entry->type;
                    printf("DEBUG: Resolved identifier '%s' to type %s\n",
                           cond->leaf->text, typename(entry->type));
                } else {
                    printf("DEBUG: Lookup failed for identifier '%s'\n", cond->leaf->text);
                }
            }
            if (!cond->type) {
                report_semantic_error("If-statement condition has no type", cond->lineno);
            } else if (cond->type->basetype != BOOL_TYPE) {
                report_semantic_error("If-statement condition must be boolean", cond->lineno);
            }
        }
    }

    /* For while-statements, assume condition is the first child */
    if (t->symbolname && strcmp(t->symbolname, "whileStatement") == 0) {
        if (t->nkids < 1 || t->kids[0] == NULL) {
            report_semantic_error("While-statement missing condition", t->lineno);
        } else {
            struct tree *cond = t->kids[0];
            if (strcmp(cond->symbolname, "Identifier") == 0 && !cond->type && cond->leaf && cond->leaf->text) {
                SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, cond->leaf->text);
                if (entry && entry->type) {
                    cond->type = entry->type;
                    printf("DEBUG: Resolved identifier '%s' to type %s\n",
                           cond->leaf->text, typename(entry->type));
                } else {
                    printf("DEBUG: Lookup failed for identifier '%s'\n", cond->leaf->text);
                }
            }
            if (!cond->type) {
                report_semantic_error("While-statement condition has no type", cond->lineno);
            } else if (cond->type->basetype != BOOL_TYPE) {
                report_semantic_error("While-statement condition must be boolean", cond->lineno);
            }
        }
    }

    /* For for-statements (the boolean condition alternative), assume condition is child[1] */
    if (t->symbolname && strcmp(t->symbolname, "forStatement") == 0) {
        if (t->nkids < 2 || t->kids[1] == NULL) {
            report_semantic_error("For-statement missing condition", t->lineno);
        } else {
            struct tree *cond = t->kids[1];
            if (strcmp(cond->symbolname, "Identifier") == 0 && !cond->type && cond->leaf && cond->leaf->text) {
                SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, cond->leaf->text);
                if (entry && entry->type) {
                    cond->type = entry->type;
                    printf("DEBUG: Resolved identifier '%s' to type %s\n",
                           cond->leaf->text, typename(entry->type));
                } else {
                    printf("DEBUG: Lookup failed for identifier '%s'\n", cond->leaf->text);
                }
            }
            if (!cond->type) {
                report_semantic_error("For-statement condition has no type", cond->lineno);
            } else if (cond->type->basetype != BOOL_TYPE) {
                report_semantic_error("For-statement condition must be boolean", cond->lineno);
            }
        }
    }
}







void check_semantics(struct tree *t) {
    if (!t)
        return;

    /* --- Check variable declarations with initializers --- */
    if (t->symbolname && strcmp(t->symbolname, "variableDeclaration") == 0 && t->nkids == 3) {
        // Expected children: [0]: identifier, [1]: type node, [2]: initializer
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

    /* --- Check assignment nodes (re-assignments) --- */
    if (t->symbolname && strcmp(t->symbolname, "assignment") == 0 && t->nkids >= 2) {
        struct tree *lhs = t->kids[0];
        struct tree *rhs = t->kids[1];

        /* If lhs is not from a variableDeclaration initializer, enforce mutability */
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

    /* --- Check binary operators --- */
    if (is_operator(t->prodrule)) {
        if (t->nkids >= 2) {
            if (!check_type_compatibility(t->kids[0]->type, t->kids[1]->type)) {
                report_semantic_error("Operator operands have incompatible types", t->kids[0]->lineno);
            }
        }
    }

    /* --- Check function call nodes (prod: 122) --- */
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

    /* --- Check control structure conditions --- */
    check_control_conditions(t);

    /* Recursively check all children */
    for (int i = 0; i < t->nkids; i++) {
        check_semantics(t->kids[i]);
    }
}
