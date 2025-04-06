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
    
    // --- Early Propagation for Declarations ---
    // If this node is a variable or constant declaration, propagate its type
    // and flags (mutability, nullability) to the identifier child.
    if (t->symbolname &&
       (strcmp(t->symbolname, "variableDeclaration") == 0 ||
        strcmp(t->symbolname, "constVariableDeclaration") == 0)) {
        if (t->nkids > 0) {
            if (t->type)
                t->kids[0]->type = t->type;
            t->kids[0]->is_mutable = t->is_mutable;
            t->kids[0]->is_nullable = t->is_nullable;
            printf("DEBUG: Propagated declaration info: identifier '%s' gets type %s, mutable=%d, nullable=%d\n",
                   t->kids[0]->leaf ? t->kids[0]->leaf->text : "unknown",
                   t->type ? typename(t->type) : "none",
                   t->is_mutable, t->is_nullable);
        }
    }
    
    // --- Immediate Resolution for Identifier Nodes ---
    // If this node is produced by the "Identifier" rule and its type is not yet set,
    // try to resolve it now from the current scope.
    if (t->symbolname && strcmp(t->symbolname, "Identifier") == 0 && !t->type) {
        char *idText = NULL;
        if (t->leaf && t->leaf->text)
            idText = t->leaf->text;
        else if (t->nkids > 0 && t->kids[0] && t->kids[0]->leaf && t->kids[0]->leaf->text)
            idText = t->kids[0]->leaf->text;
        if (idText) {
            SymbolTableEntry entry = lookup_symbol(current_scope, idText);
            if (entry && entry->type) {
                t->type = entry->type;
                t->is_mutable = entry->mutable;
                t->is_nullable = entry->nullable;
                printf("DEBUG: Resolved Identifier '%s' to type %s, mutable=%d, nullable=%d\n",
                       idText, typename(entry->type), entry->mutable, entry->nullable);
            }
        }
    }
    
    // --- Print Current Node ---
    printf("DEBUG: Entering node: %s (prod: %d) at line %d, scope: %p\n", 
           t->symbolname ? t->symbolname : "NULL", t->prodrule, t->lineno, current_scope);
    
    // --- Function Declaration: Update Current Scope ---
    if (t->prodrule == 327 && t->scope != NULL) {
        printf("DEBUG: Function declaration encountered. Updating current scope.\n");
        if (t->kids[0] && t->kids[0]->leaf)
            printf("DEBUG: Function '%s' has scope pointer: %p\n", t->kids[0]->leaf->text, t->scope);
        current_scope = t->scope;
    }
    
    // --- Recurse into Children ---
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
            if (t->nkids >= 2)
                cond = t->kids[1];
        } else {
            cond = t->kids[0];
        }
        if (cond && strcmp(cond->symbolname, "Identifier") == 0 && !cond->type) {
            char *idText = NULL;
            if (cond->leaf && cond->leaf->text)
                idText = cond->leaf->text;
            else if (cond->nkids > 0 && cond->kids[0] && cond->kids[0]->leaf && cond->kids[0]->leaf->text)
                idText = cond->kids[0]->leaf->text;
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
    
    if (t->symbolname && strcmp(t->symbolname, "comparison") == 0 && t->nkids >= 2) {
        typeptr left = t->kids[0]->type;
        typeptr right = t->kids[1]->type;
        printf("DEBUG: In comparison operator at line %d. Left type: %s, Right type: %s\n",
               t->lineno,
               (left ? typename(left) : "none"),
               (right ? typename(right) : "none"));
        
        // Allow numeric comparisons or string comparisons.
        int leftIsNumeric = (check_type_compatibility(left, integer_typeptr) || check_type_compatibility(left, double_typeptr));
        int rightIsNumeric = (check_type_compatibility(right, integer_typeptr) || check_type_compatibility(right, double_typeptr));
        int bothString = (check_type_compatibility(left, string_typeptr) && check_type_compatibility(right, string_typeptr));
        
        if (!( (leftIsNumeric && rightIsNumeric) || bothString )) {
            report_semantic_error("Invalid operands for comparison operator", t->lineno);
        }
        
        // The type of any comparison is boolean.
        t->type = boolean_typeptr;
    }

    // --- Assignment Checks ---
    if (t->symbolname && strcmp(t->symbolname, "assignment") == 0 && t->nkids >= 2) {
        struct tree *lhs = t->kids[0];
        struct tree *rhs = t->kids[1];
    
        printf("DEBUG: Assignment at line %d. LHS token: '%s'\n",
               lhs->lineno,
               (lhs->leaf && lhs->leaf->text) ? lhs->leaf->text : "unknown");
    
        if (lhs->type) {
            printf("DEBUG: LHS type pointer: %p, basetype: %d (%s)\n",
                   lhs->type, lhs->type->basetype, typename(lhs->type));
        } else {
            printf("DEBUG: LHS type is NULL\n");
        }
        printf("DEBUG: LHS mutable flag: %d\n", lhs->is_mutable);
    
        // Force resolution if the type is NULL or has basetype NONE_TYPE.
        if (lhs->leaf) {
            if (lhs->type == NULL || lhs->type->basetype == NONE_TYPE) {
                char *idText = lhs->leaf->text;
                printf("DEBUG: Attempting to resolve identifier '%s'\n", idText);
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry) {
                    printf("DEBUG: Lookup for '%s': entry pointer=%p, entry->mutable=%d\n",
                           idText, entry, entry->mutable);
                    if (entry->type) {
                        printf("DEBUG: Entry type pointer=%p, basetype=%d (%s)\n",
                               entry->type, entry->type->basetype, typename(entry->type));
                        lhs->type = entry->type;
                        lhs->is_mutable = entry->mutable;
                        lhs->is_nullable = entry->nullable;
                        printf("DEBUG: Resolved '%s': new LHS type pointer=%p, basetype=%d (%s), mutable=%d\n",
                               idText, lhs->type, lhs->type->basetype, typename(lhs->type), lhs->is_mutable);
                    } else {
                        printf("DEBUG: Entry for '%s' has no type\n", idText);
                    }
                } else {
                    printf("DEBUG: Lookup for '%s' returned NULL\n", idText);
                }
            } else {
                printf("DEBUG: Skipping resolution because LHS type is not NONE_TYPE (basetype=%d, %s)\n",
                       lhs->type->basetype, typename(lhs->type));
            }
        }
    
        printf("DEBUG: Final LHS state: token='%s', mutable=%d, type pointer=%p, basetype=%d (%s), lineno=%d\n",
               (lhs->leaf && lhs->leaf->text) ? lhs->leaf->text : "unknown",
               lhs->is_mutable,
               lhs->type,
               lhs->type ? lhs->type->basetype : -1,
               lhs->type ? typename(lhs->type) : "none",
               lhs->lineno);
    
        // Check for immutability.
        if (!(lhs->symbolname && (strcmp(lhs->symbolname, "variableDeclaration") == 0 ||
                                   strcmp(lhs->symbolname, "constVariableDeclaration") == 0))) {
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

    // --- Binary Operator Type Checking ---
    if (is_operator(t->prodrule)) {
        // Force-resolve left operand if it's an Identifier lacking a type.
        if (t->kids[0]->symbolname && strcmp(t->kids[0]->symbolname, "Identifier") == 0 && !t->kids[0]->type) {
            char *idText = (t->kids[0]->leaf && t->kids[0]->leaf->text) ? t->kids[0]->leaf->text : NULL;
            if (idText) {
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry && entry->type) {
                    t->kids[0]->type = entry->type;
                    printf("DEBUG: (Operator left) Resolved '%s' to type %s\n", idText, typename(entry->type));
                }
            }
        }
        // Force-resolve right operand if it's an Identifier lacking a type.
        if (t->kids[1]->symbolname && strcmp(t->kids[1]->symbolname, "Identifier") == 0 && !t->kids[1]->type) {
            char *idText = (t->kids[1]->leaf && t->kids[1]->leaf->text) ? t->kids[1]->leaf->text : NULL;
            if (idText) {
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry && entry->type) {
                    t->kids[1]->type = entry->type;
                    printf("DEBUG: (Operator right) Resolved '%s' to type %s\n", idText, typename(entry->type));
                }
            }
        }
        
        typeptr left = t->kids[0]->type;
        typeptr right = t->kids[1]->type;
        int prod = t->prodrule;
        
        // For addition (prod 114) and subtraction (prod 115)
        if (prod == 114 || prod == 115) {
            if (check_type_compatibility(left, integer_typeptr) &&
                check_type_compatibility(right, integer_typeptr)) {
                t->type = integer_typeptr;
            }
            else if ((check_type_compatibility(left, double_typeptr) || check_type_compatibility(right, double_typeptr)) &&
                     ((check_type_compatibility(left, integer_typeptr) || check_type_compatibility(left, double_typeptr)) &&
                      (check_type_compatibility(right, integer_typeptr) || check_type_compatibility(right, double_typeptr)))) {
                t->type = double_typeptr;
            }
            else if (prod == 114 && check_type_compatibility(left, string_typeptr) &&
                     (check_type_compatibility(right, integer_typeptr) || check_type_compatibility(right, double_typeptr))) {
                t->type = string_typeptr;
            }
            else {
                report_semantic_error("Invalid operands for addition/subtraction", t->lineno);
            }
        }
        // For multiplicative operators (prod 118, 119, 120)
        else if (prod == 118 || prod == 119 || prod == 120) {
            if (check_type_compatibility(left, integer_typeptr) &&
                check_type_compatibility(right, integer_typeptr)) {
                t->type = double_typeptr;
            }
            else if ((check_type_compatibility(left, integer_typeptr) || check_type_compatibility(left, double_typeptr)) &&
                     (check_type_compatibility(right, integer_typeptr) || check_type_compatibility(right, double_typeptr))) {
                t->type = double_typeptr;
            }
            else {
                report_semantic_error("Invalid operands for multiplicative operator", t->lineno);
            }
        }
        else {
            if (!check_type_compatibility(left, right)) {
                report_semantic_error("Operator operands have incompatible types", t->kids[0]->lineno);
            }
            t->type = left;
        }
    }
    
    // --- Function Call Checks (production 122) ---
    if (t->prodrule == 122) {
        char *funcName = t->kids[0]->leaf->text;
        printf("DEBUG: Checking function call for '%s' at line %d\n", funcName, t->kids[0]->lineno);
        SymbolTableEntry func_entry = lookup_symbol(current_scope, funcName);
        
        if (!func_entry) {
            printf("DEBUG: Undefined function '%s' in current scope chain starting at %p\n", funcName, current_scope);
            report_semantic_error("Undefined function", t->kids[0]->lineno);
        } else {
            int expected = func_entry->param_count;
            int actual = (t->kids[1] != NULL) ? t->kids[1]->nkids : 0;
            printf("DEBUG: Function '%s' expects %d parameter(s), but call has %d argument(s).\n",
                   funcName, expected, actual);
            
            if (expected != actual) {
                report_semantic_error("Function call argument count mismatch", t->kids[0]->lineno);
            }
            // Check parameter types if they are available
            for (int i = 0; i < actual; i++) {
                if (!check_type_compatibility(func_entry->param_types[i], t->kids[1]->kids[i]->type)) {
                    report_semantic_error("Function call argument type mismatch", t->kids[1]->kids[i]->lineno);
                }
            }
            // Set the type of the function call to the function's return type.
            if (func_entry->type && func_entry->type->u.f.returntype) {
                t->type = func_entry->type->u.f.returntype;
            } else {
                printf("DEBUG: Function '%s' has no recorded return type.\n", funcName);
                t->type = null_typeptr;
            }
        }
    }
    
    // --- Unresolved Identifier Resolution ---
    if (!t->type && (t->leaf && t->leaf->category == Identifier)) {
        char *idText = t->leaf->text;
        if (!idText && t->nkids > 0 && t->kids[0] && t->kids[0]->leaf)
            idText = t->kids[0]->leaf->text;
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