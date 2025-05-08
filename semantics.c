#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "k0gram.tab.h"
#include "type.h"

extern int error_count;
extern SymbolTable globalSymtab;
extern SymbolTable currentFunctionSymtab;

void report_semantic_error(const char *msg, int lineno) {
    fprintf(stderr, "Semantic Error at line %d: %s\n", lineno, msg);
    error_count++;
}

int check_type_compatibility(typeptr expected, typeptr actual) {
    if (!expected || !actual)
        return 0;
    if (expected->basetype == ANY_TYPE || actual->basetype == ANY_TYPE)
        return 1;
    
    if (expected->basetype == DOUBLE_TYPE && actual->basetype == INT_TYPE)
        return 1;
    
    if (expected == actual)
        return 1;
    
    if (expected->basetype == actual->basetype)
        return 1;
    
    return 0;
}

int is_operator(int prodrule) {
    // printf("DEBUG: Checking if prodrule %d is an operator\n", prodrule);
    switch (prodrule) {
        case ADD: /* additive_expression with ADD */
        case SUB: /* additive_expression with SUB */
        case MULT: /* multiplicative_expression with MULT */
        case DIV: /* multiplicative_expression with DIV */
        case MOD: /* multiplicative_expression with MOD */
            return 1;
        default:
            return 0;
    }
}

void flattenExpressionList(struct tree *exprList, struct tree ***args, int *count) {
    if (!exprList) return;
    if (exprList->symbolname && strcmp(exprList->symbolname, "expressionList") == 0) {
        for (int i = 0; i < exprList->nkids; i++) {
            flattenExpressionList(exprList->kids[i], args, count);
        }
    } else {
        *args = realloc(*args, ((*count) + 1) * sizeof(struct tree *));
        if (!(*args)) {
            fprintf(stderr, "Memory allocation failed in flattenExpressionList\n");
            exit(1);
        }
        (*args)[*count] = exprList;
        (*count)++;
    }
}

char *resolve_qualified_name(struct tree *t) {
    if (!t)
        return NULL;

    char *result = NULL;
    if (t->symbolname) {
        // fprintf(stderr, "DEBUG: Resolving node with symbolname: %s\n", t->symbolname);
    }

    if (t->symbolname && strcmp(t->symbolname, "functionCall") == 0) {
        if (t->nkids > 0) {
            result = resolve_qualified_name(t->kids[0]);
            // fprintf(stderr, "DEBUG: functionCall node resolved to: %s\n", result);
            return result;
        }
    }

    if (t->leaf) {
        result = strdup(t->leaf->text);
        // fprintf(stderr, "DEBUG: Leaf node resolved to: %s\n", result);
        return result;
    }

    if (t->symbolname && strcmp(t->symbolname, "qualifiedName") == 0) {
        int first = 1;
        for (int i = 0; i < t->nkids; i++) {
            char *part = resolve_qualified_name(t->kids[i]);
            if (!part)
                continue;
            if (first) {
                result = part;
                first = 0;
            } else {
                int new_len = strlen(result) + strlen(part) + 2;
                char *tmp = malloc(new_len);
                if (!tmp) {
                    free(result);
                    free(part);
                    return NULL;
                }
                sprintf(tmp, "%s.%s", result, part);
                free(result);
                free(part);
                result = tmp;
            }
        }
        // fprintf(stderr, "DEBUG: qualifiedName node resolved to: %s\n", result);
        return result;
    }
    
    if (t->symbolname) {
        result = strdup(t->symbolname);
        // fprintf(stderr, "DEBUG: Fallback node resolved to: %s\n", result);
        return result;
    }
    
    return NULL;
}

int is_null_literal(struct tree *t) {
    if (!t) return 0;
    if (t->leaf && t->leaf->text && strcmp(t->leaf->text, "null") == 0)
        return 1;
    if (t->symbolname && strcmp(t->symbolname, "NullLiteral") == 0)
        return 1;
    return 0;
}

void check_semantics_helper(struct tree *t, SymbolTable current_scope) {
    if (!t)
        return;
    // fprintf(stderr, "DEBUG: check_semantics_helper called for node: %s\n", 
    //         t->symbolname ? t->symbolname : "NULL");

    printf("SEMANTIC-NODE: %-25s  nkids=%d\n",
        t->symbolname ? t->symbolname : "(null)", t->nkids);

    if (t->symbolname &&
        (strcmp(t->symbolname, "genericType") == 0 ||
        strcmp(t->symbolname, "nullableGenericType") == 0)) {
        printf("DEBUG: Generic type encountered\n");
        check_semantics_helper(t->kids[1], current_scope);
        return;
    }

    if (strcmp(t->symbolname, "assignment") == 0 &&
        t->nkids == 1 &&
        t->kids[0] &&
        strcmp(t->kids[0]->symbolname, "arrayAssignmentDeclaration") == 0)
    {
        printf("here\n");
        /* Delegate entirely to the arrayAssignmentDeclaration logic below */
        check_semantics_helper(t->kids[0], current_scope);
        /* Propagate the resultant type back onto the assignment node */
        t->type = t->kids[0]->type;
        return;
    }

    /* 2) var a : Array<T> = Array<T>(n){…} */
    if (strcmp(t->symbolname, "arrayAssignmentDeclaration") == 0) {
        printf("DEBUG: Array assignment declaration encountered\n");
        struct tree *varId    = t->kids[0];
        struct tree *declType = t->kids[1];   // the “: Array<T>” node
        struct tree *ctorType = t->kids[3];   // the “<T>” node in Array<T>(...)
        struct tree *initTree = t->kids[4];
        struct tree *sizeExpr = initTree->kids[0];
        struct tree *initExpr = initTree->kids[1];
    
        /*–– 1) first visit all children so their .type fields get set ––*/
        check_semantics_helper(varId,    current_scope);
        check_semantics_helper(declType, current_scope);
        check_semantics_helper(ctorType, current_scope);
        check_semantics_helper(sizeExpr, current_scope);
        check_semantics_helper(initExpr, current_scope);
    
        /*–– 2) now we can safely pull the array type out of declType ––*/
        t->type = declType->type;
    
        /*–– 3) run your checks ––*/
        if (!t->type
            || t->type->basetype != ARRAY_TYPE
            || t->type->u.a.elemtype != ctorType->type)
        {
            report_semantic_error(
              "mismatched Array<…> in declaration",
              t->lineno);
        }
    
        /* size must be an Int */
        if (!sizeExpr->type
            || sizeExpr->type->basetype != INT_TYPE)
        {
            report_semantic_error("Array size must be Int",
                                  sizeExpr->lineno);
        }
    
        /* init must match element type */
        if (!check_type_compatibility(initExpr->type,
                                      t->type->u.a.elemtype))
        {
            report_semantic_error("Array init type mismatch",
                                  initExpr->lineno);
        }
    
        /*–– 4) insert into symbol‐table ––*/
        if (! lookup_symbol(current_scope, varId->leaf->text)) {
            insert_symbol(current_scope,
                          varId->leaf->text,
                          VARIABLE,
                          t->type,
                          t->is_mutable,
                          t->is_nullable);
            /* propagate the new symbol info onto the varId node */
            varId->type      = t->type;
            varId->is_mutable= t->is_mutable;
            varId->is_nullable = t->is_nullable;
        }
    
        /*–– 5) DONE – stop here! ––*/
        printf("DEBUG: Array assignment declaration '%s' type %s\n",
               varId->leaf->text, typename(t->type));
        return;
    }    

    /* 3) var a : Array<T>(n){…} without the explicit “Array” constructor token*/
    if (strcmp(t->symbolname, "arrayDeclaration") == 0) {
        printf("DEBUG: Array declaration encountered\n");
        struct tree *varId    = t->kids[0];
        struct tree *declType = t->kids[1];
        struct tree *initTree = t->kids[2];
        struct tree *sizeExpr = initTree->kids[0];
        struct tree *initExpr = initTree->kids[1];

        check_semantics_helper(sizeExpr, current_scope);
        if (sizeExpr->type->basetype != INT_TYPE)
            report_semantic_error("Array size must be Int", sizeExpr->lineno);

        check_semantics_helper(initExpr, current_scope);
        if (!check_type_compatibility(initExpr->type,
              declType->type->u.a.elemtype))
            report_semantic_error("Array init type mismatch", initExpr->lineno);

        insert_symbol(current_scope,
                      varId->leaf->text,
                      VARIABLE,
                      declType->type,
                      t->is_mutable,
                      declType->is_nullable);

        t->type = declType->type;
        printf("DEBUG: Array declaration '%s' type %s\n",
               varId->leaf->text, typename(t->type));
        return;
    }

    if (t->symbolname &&
       (strcmp(t->symbolname, "variableDeclaration") == 0 ||
        strcmp(t->symbolname, "constVariableDeclaration") == 0)) {
        if (t->nkids > 0) {
            if (t->type)
                t->kids[0]->type = t->type;
            t->kids[0]->is_mutable = t->is_mutable;
            t->kids[0]->is_nullable = t->is_nullable;
            // printf("DEBUG: Propagated declaration info: identifier '%s' gets type %s, mutable=%d, nullable=%d\n",
            //        t->kids[0]->leaf ? t->kids[0]->leaf->text : "unknown",
            //        t->type ? typename(t->type) : "none",
            //        t->is_mutable, t->is_nullable);
        }
    }
    
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
                // printf("DEBUG: Resolved Identifier '%s' to type %s, mutable=%d, nullable=%d\n",
                //        idText, typename(entry->type), entry->mutable, entry->nullable);
            }
        }
    }

    // printf("DEBUG: Entering node: %s (prod: %d) at line %d, scope: %p\n", 
    //        t->symbolname ? t->symbolname : "NULL", t->prodrule, t->lineno, current_scope);
    
    if (t->symbolname && 
    (strcmp(t->symbolname, "forStatementKotlinRange") == 0 || 
        strcmp(t->symbolname, "forStatementKotlinRangeUntil") == 0)) {
    
        // printf("DEBUG: For loop encountered at line %d. Creating new scope for loop variable.\n", t->lineno);
        
        SymbolTable loop_scope = create_function_scope(current_scope, "forLoop");
        t->scope = loop_scope;
        
        if (t->kids[0] && t->kids[0]->leaf) {
            char *loopVarName = t->kids[0]->leaf->text;
            insert_symbol(loop_scope, loopVarName, VARIABLE, integer_typeptr, 0, 0);
            t->kids[0]->type = integer_typeptr;
            t->kids[0]->is_mutable = 0; // You can mark it immutable if desired.
            // printf("DEBUG: Loop variable '%s' inserted with type int in new scope %p.\n", loopVarName, loop_scope);
        }
        
        current_scope = loop_scope;
    }

    if (t->prodrule == 327 && t->scope != NULL) {
        // printf("DEBUG: Function declaration encountered. Updating current scope.\n");
        if (t->kids[0] && t->kids[0]->leaf)
            // printf("DEBUG: Function '%s' has scope pointer: %p\n", t->kids[0]->leaf->text, t->scope);
        current_scope = t->scope;
    }
    
    for (int i = 0; i < t->nkids; i++) {
        check_semantics_helper(t->kids[i], current_scope);
    }
    
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
                // printf("DEBUG: Attempting to resolve identifier '%s' in control structure condition.\n", idText);
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry && entry->type) {
                    cond->type = entry->type;
                    // printf("DEBUG: Resolved '%s' to type %s\n", idText, typename(entry->type));
                } else {
                    // printf("DEBUG: Lookup failed for identifier '%s' in scope %p\n", idText, current_scope);
                }
            } else {
                // printf("DEBUG: Could not retrieve identifier text from node '%s'\n", cond->symbolname);
            }
        }
        if (!cond || !cond->type) {
            report_semantic_error("Control structure condition has no type", t->lineno);
        } else if (cond->type->basetype != BOOL_TYPE) {
            report_semantic_error("Control structure condition must be boolean", cond->lineno);
        }
    }
    
    if (t->symbolname && strcmp(t->symbolname, "variableDeclaration") == 0 && t->nkids == 3) {
        struct tree *declTypeNode = t->kids[1];
        struct tree *initializer = t->kids[2];
        
        if (is_null_literal(initializer)) {
            if (!t->is_nullable) {
                report_semantic_error("Assignment of null to non-nullable variable", initializer->lineno);
            }
            goto after_type_check;
        }
        else if (!check_type_compatibility(declTypeNode->type, initializer->type)) {
            char errMsg[256];
            snprintf(errMsg, sizeof(errMsg),
                     "Type mismatch in declaration: cannot assign value of type %s to variable of type %s",
                     (initializer->type ? typename(initializer->type) : "none"),
                     (declTypeNode->type ? typename(declTypeNode->type) : "none"));
            report_semantic_error(errMsg, initializer->lineno);
        }
    }
    after_type_check:
    if (t->symbolname && strcmp(t->symbolname, "variableDeclaration") == 0 &&
        t->nkids == 3 &&
        t->kids[1] && t->kids[1]->type &&
        t->kids[2]) {

        typeptr declared = t->kids[1]->type;
        struct tree *initializer = t->kids[2];

        if (initializer->symbolname && strcmp(initializer->symbolname, "functionCall") == 0) {
            struct tree *callee = initializer->kids[0];
            if (callee && callee->leaf && strcmp(callee->leaf->text, "Array") == 0 &&
                declared->basetype == ARRAY_TYPE) {
                initializer->type = declared;
                // printf("DEBUG: Treating 'Array(...) { ... }' as array initializer\n");
            }
        }
    }

    if (t->symbolname && strcmp(t->symbolname, "arrayAccess") == 0) {
        if (t->kids[0]->symbolname && strcmp(t->kids[0]->symbolname, "Identifier") == 0 && !t->kids[0]->type) {
            char *idText = (t->kids[0]->leaf && t->kids[0]->leaf->text) ? t->kids[0]->leaf->text : NULL;
            if (idText) {
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry && entry->type) {
                    t->kids[0]->type = entry->type;
                    t->kids[0]->is_mutable = entry->mutable;
                    t->kids[0]->is_nullable = entry->nullable;
                    // printf("DEBUG: (Array access) Resolved array identifier '%s' to type %s, mutable=%d\n", 
                    //      idText, typename(entry->type), entry->mutable);
                }
            }
        }

        if (t->kids[1] && !check_type_compatibility(t->kids[1]->type, integer_typeptr)) {
            report_semantic_error("Array index must be of integer type", t->kids[1]->lineno);
        }
        
        if (t->kids[0]->type && t->kids[0]->type->basetype == ARRAY_TYPE) {
            t->type = t->kids[0]->type->u.a.elemtype;
            t->is_mutable = t->kids[0]->is_mutable;
            t->is_nullable = t->kids[0]->is_nullable;
            // printf("DEBUG: Array access resolved: element type is %s, mutable=%d\n", 
            //      typename(t->type), t->is_mutable);
        } else {
            report_semantic_error("Array access on a non-array type", t->lineno);
            t->type = null_typeptr;
        }
    }

    if (t->symbolname && strcmp(t->symbolname, "postIncrement") == 0 && t->nkids == 1) {
        struct tree *varNode = t->kids[0];
        // Type check: must be integer or double
        if (!check_type_compatibility(varNode->type, integer_typeptr) &&
            !check_type_compatibility(varNode->type, double_typeptr)) {
            report_semantic_error("Invalid operand type for '++'", t->lineno);
        }
        // The result type is the same as the operand
        t->type = varNode->type;
        return;
    }

    else if (t->symbolname && strcmp(t->symbolname, "postDecrement") == 0 && t->nkids == 1) {
        struct tree *varNode = t->kids[0];
        /* Operand must be Int or Double */
        if (varNode->type != integer_typeptr && varNode->type != double_typeptr) {
            report_semantic_error("Operand of -- must be Int or Double", t->lineno);
        }
        /* Result type is same as operand */
        t->type = varNode->type;
        return;
    }

    if (t->symbolname && strcmp(t->symbolname, "comparison") == 0 && t->nkids >= 2) {
        typeptr left = t->kids[0]->type;
        typeptr right = t->kids[1]->type;
        // printf("DEBUG: In comparison operator at line %d. Left type: %s, Right type: %s\n",
        //        t->lineno,
        //        (left ? typename(left) : "none"),
        //        (right ? typename(right) : "none"));
        
        int leftIsNumeric = (check_type_compatibility(left, integer_typeptr) || check_type_compatibility(left, double_typeptr));
        int rightIsNumeric = (check_type_compatibility(right, integer_typeptr) || check_type_compatibility(right, double_typeptr));
        int bothString = (check_type_compatibility(left, string_typeptr) && check_type_compatibility(right, string_typeptr));
        
        if (!( (leftIsNumeric && rightIsNumeric) || bothString )) {
            report_semantic_error("Invalid operands for comparison operator", t->lineno);
        }
        
        t->type = boolean_typeptr;
    }

    if (t->symbolname && strcmp(t->symbolname, "equality") == 0 && t->nkids == 2) {
        //typeptr left  = t->kids[0]->type;
        //typeptr right = t->kids[1]->type;
        t->type = boolean_typeptr;
    }

    if (t->symbolname && strcmp(t->symbolname, "assignment") == 0 && t->nkids >= 2) {
        struct tree *lhs = t->kids[0];
        struct tree *rhs = t->kids[1];
    
        // printf("DEBUG: Assignment at line %d. LHS token: '%s'\n",
        //        lhs->lineno,
        //        (lhs->leaf && lhs->leaf->text) ? lhs->leaf->text : "unknown");
    
        if (lhs->type) {
            // printf("DEBUG: LHS type pointer: %p, basetype: %d (%s)\n",
            //       lhs->type, lhs->type->basetype, typename(lhs->type));
        } else {
            // printf("DEBUG: LHS type is NULL\n");
        }
        // printf("DEBUG: LHS mutable flag: %d\n", lhs->is_mutable);
    
        if (lhs->leaf) {
            if (lhs->type == NULL || lhs->type->basetype == NONE_TYPE) {
                char *idText = lhs->leaf->text;
                // printf("DEBUG: Attempting to resolve identifier '%s'\n", idText);
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry) {
                    // printf("DEBUG: Lookup for '%s': entry pointer=%p, entry->mutable=%d\n",
                    //        idText, entry, entry->mutable);
                    if (entry->type) {
                        // printf("DEBUG: Entry type pointer=%p, basetype=%d (%s)\n",
                        //         entry->type, entry->type->basetype, typename(entry->type));
                        lhs->type = entry->type;
                        lhs->is_mutable = entry->mutable;
                        lhs->is_nullable = entry->nullable;
                        // printf("DEBUG: Resolved '%s': new LHS type pointer=%p, basetype=%d (%s), mutable=%d\n",
                        //       idText, lhs->type, lhs->type->basetype, typename(lhs->type), lhs->is_mutable);
                    } else {
                        // printf("DEBUG: Entry for '%s' has no type\n", idText);
                    }
                } else {
                    // printf("DEBUG: Lookup for '%s' returned NULL\n", idText);
                }
            } else {
                // printf("DEBUG: Skipping resolution because LHS type is not NONE_TYPE (basetype=%d, %s)\n",
                //       lhs->type->basetype, typename(lhs->type));
            }
        }
        
        if (lhs->symbolname && strcmp(lhs->symbolname, "arrayAccess") == 0) {
            struct tree *arrayVar = lhs->kids[0];
            
            if (arrayVar->symbolname && strcmp(arrayVar->symbolname, "Identifier") == 0 && !arrayVar->type) {
                char *idText = (arrayVar->leaf && arrayVar->leaf->text) ? arrayVar->leaf->text : NULL;
                if (idText) {
                    SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                    if (entry && entry->type) {
                        arrayVar->type = entry->type;
                        arrayVar->is_mutable = entry->mutable;
                        arrayVar->is_nullable = entry->nullable;
                        // printf("DEBUG: (Assignment) Resolved array identifier '%s' to type %s, mutable=%d\n", 
                        //      idText, typename(entry->type), entry->mutable);
                        
                        if (arrayVar->type->basetype == ARRAY_TYPE) {
                            lhs->type = arrayVar->type->u.a.elemtype;
                            lhs->is_mutable = arrayVar->is_mutable;
                            // printf("DEBUG: Updated array access element type to %s, mutable=%d\n", 
                            //      typename(lhs->type), lhs->is_mutable);
                        }
                    }
                }
            }
            
            // printf("DEBUG: Array assignment: array var mutable=%d\n", arrayVar->is_mutable);
            if (!arrayVar->is_mutable) {
                report_semantic_error("Assignment to element of immutable array", lhs->lineno);
            }
            
            lhs->is_mutable = arrayVar->is_mutable;
        }

        // printf("DEBUG: Final LHS state: token='%s', mutable=%d, type pointer=%p, basetype=%d (%s), lineno=%d\n",
        //        (lhs->leaf && lhs->leaf->text) ? lhs->leaf->text : "unknown",
        //        lhs->is_mutable,
        //        lhs->type,
        //        lhs->type ? lhs->type->basetype : -1,
        //        lhs->type ? typename(lhs->type) : "none",
        //        lhs->lineno);
    
        if (lhs->symbolname
            && strcmp(lhs->symbolname, "Identifier") == 0)
        {
            /* now .leaf is valid */
            SymbolTableEntry entry =
                lookup_symbol(currentFunctionSymtab,
                              lhs->leaf->text);
            if (entry && !entry->mutable) {
                report_semantic_error(
                    "Assignment to immutable variable",
                    lhs->lineno);
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

    if (is_operator(t->prodrule)) {
        // printf("DEBUG: Operator node at line %d, prodrule %d\n", t->lineno, t->prodrule);
        if (t->kids[0]->symbolname && strcmp(t->kids[0]->symbolname, "Identifier") == 0 && !t->kids[0]->type) {
            char *idText = (t->kids[0]->leaf && t->kids[0]->leaf->text) ? t->kids[0]->leaf->text : NULL;
            if (idText) {
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry && entry->type) {
                    t->kids[0]->type = entry->type;
                    // printf("DEBUG: (Operator left) Resolved '%s' to type %s\n", idText, typename(entry->type));
                }
            }
        }
        if (t->kids[1]->symbolname && strcmp(t->kids[1]->symbolname, "Identifier") == 0 && !t->kids[1]->type) {
            char *idText = (t->kids[1]->leaf && t->kids[1]->leaf->text) ? t->kids[1]->leaf->text : NULL;
            if (idText) {
                SymbolTableEntry entry = lookup_symbol(current_scope, idText);
                if (entry && entry->type) {
                    t->kids[1]->type = entry->type;
                    // printf("DEBUG: (Operator right) Resolved '%s' to type %s\n", idText, typename(entry->type));
                }
            }
        }
        
        typeptr left = t->kids[0]->type;
        typeptr right = t->kids[1]->type;
        int prod = t->prodrule;

        // fprintf(stderr,
        //     "DEBUG[op=%d,line=%d]: left=%s(%d), right=%s(%d)\n",
        //     prod,
        //     t->lineno,
        //     left  ? typename(left)  : "NULL",  left  ? left->basetype  : -1,
        //     right ? typename(right) : "NULL",  right ? right->basetype : -1
        // );
        
        if (prod == ADD || prod == SUB) {
            if (prod == ADD && (check_type_compatibility(left, string_typeptr) ||
                check_type_compatibility(right, string_typeptr))) {
                t->type = string_typeptr;
            }
            else if (check_type_compatibility(left, integer_typeptr) &&
                     check_type_compatibility(right, integer_typeptr)) {
                t->type = integer_typeptr;
            }
            else if ((check_type_compatibility(left, double_typeptr) || check_type_compatibility(right, double_typeptr)) &&
                     ((check_type_compatibility(left, integer_typeptr) || check_type_compatibility(left, double_typeptr)) &&
                      (check_type_compatibility(right, integer_typeptr) || check_type_compatibility(right, double_typeptr)))) {
                t->type = double_typeptr;
            }
            else {
                report_semantic_error("Invalid operands for addition", t->lineno);
            }
        }
        else if (prod == MULT || prod == DIV || prod == MOD) {
            if (check_type_compatibility(left, integer_typeptr) &&
                check_type_compatibility(right, integer_typeptr)) {
                t->type = integer_typeptr;
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
    
    if (t->prodrule == 122) {
        char *funcName = resolve_qualified_name(t->kids[0]);
        if (!funcName && t->kids[0] && t->kids[0]->leaf) {
            funcName = strdup(t->kids[0]->leaf->text);
        }
        // fprintf(stderr, "DEBUG: Resolved function name: %s\n", funcName);

        // printf("DEBUG: Checking function call for '%s' at line %d\n", funcName, t->kids[0]->lineno);
        SymbolTableEntry func_entry = lookup_symbol(current_scope, funcName);
        
        if (!func_entry) {
            // printf("DEBUG: Undefined function '%s' in current scope chain starting at %p\n", funcName, current_scope);
            report_semantic_error("Undefined function", t->kids[0]->lineno);
        } else {
            struct tree **args = NULL;
            int actual = 0;
            flattenExpressionList(t->kids[1], &args, &actual);
            // printf("DEBUG: Function '%s' expects %d parameter(s), call has %d argument(s).\n",
            //        funcName, func_entry->param_count, actual);
            
            if (func_entry->param_count != actual) {
                report_semantic_error("Function call argument count mismatch", t->kids[0]->lineno);
            }
            for (int i = 0; i < actual; i++) {
                if (!check_type_compatibility(func_entry->param_types[i], args[i]->type)) {
                    char errMsg[256];
                    snprintf(errMsg, sizeof(errMsg),
                             "Function call argument type mismatch for parameter %d: expected %s, got %s",
                             i, (func_entry->param_types[i] ? typename(func_entry->param_types[i]) : "none"),
                             (args[i]->type ? typename(args[i]->type) : "none"));
                    report_semantic_error(errMsg, args[i]->lineno);
                }
            }
            if (func_entry->type && func_entry->type->u.f.returntype) {
                t->type = func_entry->type->u.f.returntype;
            } else {
                // printf("DEBUG: Function '%s' has no recorded return type.\n", funcName);
                t->type = null_typeptr;
            }
            free(args);
        }
    }
    
    if (!t->type && (t->leaf && t->leaf->category == Identifier)) {
        char *idText = t->leaf->text;
        if (!idText && t->nkids > 0 && t->kids[0] && t->kids[0]->leaf)
            idText = t->kids[0]->leaf->text;
        if (idText) {
            SymbolTableEntry entry = lookup_symbol(current_scope, idText);
            if (entry && entry->type) {
                t->type = entry->type;
                // printf("DEBUG: Resolved leaf identifier '%s' to type %s\n", idText, typename(entry->type));
            } else {
                report_semantic_error("Undeclared identifier", t->lineno);
            }
        }
    }
}


void check_semantics(struct tree *t) {
    SymbolTable starting_scope = currentFunctionSymtab;
    check_semantics_helper(t, starting_scope);
}
