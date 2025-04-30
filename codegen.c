 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdarg.h>
 
 #include "tree.h"
 #include "tac.h"
 #include "k0gram.tab.h"
 #include "type.h"
 #include "codegen.h"
 #include "symtab.h"
 
 #define NULL_ADDR ((struct addr){R_NONE, {.offset = 0}})
 #define DEBUG_OUTPUT 0  // Set to 1 to enable debug output, 0 to disable
 
 extern SymbolTable currentFunctionSymtab;
 extern SymbolTable globalSymtab;

 extern struct addr new_temp(void);
 extern struct addr *genlabel(void);
 extern struct instr *gen(int op, struct addr a1, struct addr a2, struct addr a3);
 extern struct instr *concat(struct instr *l1, struct instr *l2);
static struct addr *current_break_label = NULL;

 extern char *regionname(int i);
 extern char *opcodename(int i);
 extern char *pseudoname(int i);

 typedef struct { char *label, *text; } StrLit;
 static StrLit strtab[128];
 static int   strcount = 0;
 
 static void add_string_literal(const char *label, const char *text) {
     strtab[strcount].label = strdup(label);
     strtab[strcount].text  = strdup(text);
     strcount++;
 }

 static void flattenExprList(struct tree *elist, struct tree ***outArgs, int *outCount) {
    if (!elist) return;
    if (elist->symbolname && strcmp(elist->symbolname, "expressionList") == 0) {
        if (elist->nkids == 1) {
            flattenExprList(elist->kids[0], outArgs, outCount);
        } else {
            flattenExprList(elist->kids[0], outArgs, outCount);
            flattenExprList(elist->kids[1], outArgs, outCount);
        }
    } else {
        *outArgs = realloc(*outArgs, sizeof(struct tree*) * (*outCount + 1));
        (*outArgs)[*outCount] = elist;
        (*outCount)++;
    }
}
 
 static void debug_print(const char *format, ...) {
     if (DEBUG_OUTPUT) {
         va_list args;
         va_start(args, format);
         vfprintf(stderr, format, args);
         va_end(args);
     }
 }
 

 static void generate_output_filename(const char *input_filename, char *output_filename, size_t size) {
     if (!input_filename || !output_filename || size == 0) {
         fprintf(stderr, "ERROR: Invalid parameters to generate_output_filename\n");
         return;
     }
     
     strncpy(output_filename, input_filename, size - 1);
     output_filename[size - 1] = '\0';
     char *dot = strrchr(output_filename, '.');
     if (dot != NULL) {
         strcpy(dot, ".ic");
     } else {
         strncat(output_filename, ".ic", size - strlen(output_filename) - 1);
     }
 }
 

static void format_operand(struct addr a, char *buf, size_t sz) {
    if (a.region == R_NAME && a.u.name) {
        snprintf(buf, sz, "%s", a.u.name);
    }
    else if (a.region == R_NONE) {
        snprintf(buf, sz, "none:%d", a.u.offset);
    }
    else if (a.region < R_GLOBAL || a.region > R_RET) {
        snprintf(buf, sz, "invalid:%d", a.u.offset);
    }
    else {
        snprintf(buf, sz, "%s:%d",
                 regionname(a.region),
                 a.u.offset);
    }
}


 static void output_instruction(FILE *f, struct instr *instr) {
    if (!f || !instr) return;

    char destbuf[32], src1buf[32], src2buf[32];

    format_operand(instr->dest,  destbuf, sizeof destbuf);
    format_operand(instr->src1, src1buf, sizeof src1buf);
    format_operand(instr->src2, src2buf, sizeof src2buf);

    fprintf(f, "%-8s  %-16s  %-16s  %-16s\n",
            opcodename(instr->opcode),
            destbuf,
            src1buf,
            src2buf);
}
 

 void write_ic_file(const char *input_filename, struct instr *code) {
     if (!input_filename) {
         fprintf(stderr, "ERROR: NULL input filename provided to write_ic_file\n");
         return;
     }
     
     char output_filename[256];
     generate_output_filename(input_filename, output_filename, sizeof(output_filename));
     
     FILE *f = fopen(output_filename, "w");
     if (!f) {
         fprintf(stderr, "ERROR: Could not open output file %s for writing\n", output_filename);
         return;
     }
     
     debug_print("DEBUG: Writing intermediate code to file %s\n", output_filename);
     printf("Intermediate code will be written to %s\n", output_filename);
     
     fprintf(f, ".string\n");
    for (int i = 0; i < strcount; i++) {
        fprintf(f, "%s\t\"%s\"\n", strtab[i].label, strtab[i].text);
    }
     
     fprintf(f, ".data\n");
     fprintf(f, "/* global variable declarations */\n\n");
     
     fprintf(f, ".code\n");
     
     if (code == NULL) {
         debug_print("DEBUG: write_ic_file: TAC code list is NULL!\n");
         fprintf(f, "/* No code generated */\n");
     } else {
         struct instr *curr = code;
         int instrCount = 0;
         while (curr != NULL) {
             debug_print("DEBUG: Writing instruction %d at address %p\n", instrCount, (void *)curr);
             output_instruction(f, curr);
             curr = curr->next;
             instrCount++;
         }
         if (instrCount == 0) {
             fprintf(f, "/* No instructions generated */\n");
         }
         debug_print("DEBUG: Finished writing %d instructions to %s\n", instrCount, output_filename);
     }
     
     fclose(f);
 }
 

 void generate_code(struct tree *t) {
    if (!t) return;

    t->returned = 0;

    debug_print("Processing node: %s\n", t->symbolname?t->symbolname:"unnamed");
    t->code  = NULL;
    t->place = (struct addr){ R_NONE, { .offset = 0 } };

    if (!t->leaf && t->nkids == 1 && t->kids[0] && t->kids[0]->leaf) {
        generate_code(t->kids[0]);
        t->place = t->kids[0]->place;
        t->code  = t->kids[0]->code;
        return;
    }

    if (t->leaf) {
        debug_print("Leaf node: category=%d, text=%s\n", 
                   t->leaf->category,
                   t->leaf->text ? t->leaf->text : "null");
    } else {
        debug_print("Interior node with %d kids\n", t->nkids);
    }
    /*literals & identifiers*/
    if (t->leaf) {
        // printf("Category: %d\n", t->leaf->category);
        switch (t->leaf->category) {
            case IntegerLiteral: {
                // printf("int literal\n");
                t->place = new_temp();
                struct addr imm = { .region = R_IMMED,
                                    .u.offset = t->leaf->value.ival };
                t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
                debug_print("LIT int %d -> %s:%d\n",
                            t->leaf->value.ival,
                            regionname(t->place.region),
                            t->place.u.offset);
                return;
            }
            case RealLiteral: {
                // printf("real literal\n");
                t->place = new_temp();
                struct addr imm = { .region = R_IMMED,
                                    .u.offset = (int)t->leaf->value.dval };
                t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
                debug_print("LIT real %f -> %s:%d\n",
                            t->leaf->value.dval,
                            regionname(t->place.region),
                            t->place.u.offset);
                return;
            }
            case StringLiteral: {
                static int stringLabelCounter = 0;
            
                for (int i = 0; i < strcount; i++) {
                    if (strcmp(strtab[i].text, t->leaf->text) == 0) {
                        t->place.region = R_GLOBAL;
                        t->place.u.offset = i;
                        return;
                    }
                }
            
                char label[32];
                snprintf(label, sizeof(label), "S%d", stringLabelCounter);
                t->place.region = R_GLOBAL;
                t->place.u.offset = stringLabelCounter;
                add_string_literal(label, t->leaf->text);
                stringLabelCounter++;
                return;
            }
            
            case BooleanLiteral: {
                // printf("boolean literal\n");
                t->place = new_temp();
                int val = strcmp(t->leaf->text,"true")==0 ? 1 : 0;
                struct addr imm = { .region = R_IMMED, .u.offset = val };
                t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
                debug_print("LIT bool %s -> %s:%d\n",
                            t->leaf->text,
                            regionname(t->place.region),
                            t->place.u.offset);
                return;
            }
            case Identifier: {
                // fprintf(stderr,
                //     "DEBUG: Identifier node: text=\"%s\"\n",
                //     t->leaf->text);
                SymbolTableEntry e = lookup_symbol(currentFunctionSymtab, t->leaf->text);
                if (e) {
                    t->place = e->location;
                    debug_print("ID '%s' -> %s:%d\n",
                                t->leaf->text,
                                regionname(t->place.region),
                                t->place.u.offset);
                } else {
                    t->place = new_temp();
                    debug_print("ID '%s' missing -> temp %s:%d\n",
                                t->leaf->text,
                                regionname(t->place.region),
                                t->place.u.offset);
                }
                return;
            }
            default:
                break;
        }
    }

    /*Special‑cases*/
    if (t->symbolname) {
        /*variableDeclaration */
        // fprintf(stderr,"NODE: symbolname=%s, prodrule=%d\n",
        //     t->symbolname? t->symbolname:"(null)",
        //     t->prodrule);

        /*assignment*/
        if (strcmp(t->symbolname, "assignment")==0 && t->nkids == 2) {
            struct tree *lhs = t->kids[0]; 
            struct tree *rhs = t->kids[1];
            
            if (lhs->leaf && lhs->leaf->category == Identifier) {
                SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, lhs->leaf->text);
                
                if (!entry) {
                    fprintf(stderr, "ERROR: Variable '%s' not found in symbol table\n", 
                            lhs->leaf->text);
                    t->place = new_temp(); 
                    return;
                }
                lhs->place = entry->location;
            } else {
                if (lhs) generate_code(lhs);
            }
            
            if (rhs) generate_code(rhs);
            
            if (lhs->place.region == R_NONE || rhs->place.region == R_NONE) {
                fprintf(stderr, "ERROR: Invalid places in assignment\n");
                t->place = lhs->place; 
                return;
            }
            
            t->place = lhs->place;
            t->code = concat(rhs->code, 
                            gen(O_ASN, 
                                lhs->place, 
                                rhs->place, 
                                NULL_ADDR));
            
            debug_print("DEBUG: Generated assignment: %s:%d = %s:%d\n",
                       regionname(lhs->place.region), lhs->place.u.offset,
                       regionname(rhs->place.region), rhs->place.u.offset);
            return;
        }

        else if (strcmp(t->symbolname, "additive_expression") == 0 && t->nkids == 2) {
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);

            int opcode;
            if      (t->prodrule == ADD) opcode = O_IADD;
            else if (t->prodrule == SUB) opcode = O_ISUB;
            else                         opcode = O_IADD;  

            t->place = new_temp();
            t->code  = concat(
                          concat(t->kids[0]->code, t->kids[1]->code),
                          gen(opcode,
                              t->place,
                              t->kids[0]->place,
                              t->kids[1]->place)
                       );
            return;
        }

        

        /*multiplicative_expression*/
        else if (strcmp(t->symbolname, "multiplicative_expression") == 0 && t->nkids == 2) {
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);
        
            int opcode;
            if (t->type == double_typeptr) {
                if (t->prodrule == MULT) opcode = O_DMUL;
                else if (t->prodrule == DIV) opcode = O_DDIV;
                else opcode = O_DMOD; 
            } else {
                if (t->prodrule == MULT) opcode = O_IMUL;
                else if (t->prodrule == DIV) opcode = O_IDIV;
                else opcode = O_IMOD;
            }
        
            t->place = new_temp();
            t->code = concat(
                concat(t->kids[0]->code, t->kids[1]->code),
                gen(opcode, t->place, t->kids[0]->place, t->kids[1]->place)
            );
            return;
        }
        


        /*comparison*/
        else if (strcmp(t->symbolname, "comparison")==0 && t->nkids==2) {
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);
        
            int opcode;
            if      (t->prodrule == RANGLE) opcode = O_IGT;
            else if (t->prodrule == LANGLE) opcode = O_ILT;
            else if (t->prodrule == GE) opcode = O_IGE;
            else if (t->prodrule == LE) opcode = O_ILE;
            else if (t->prodrule == EQEQ) opcode = O_IEQ;
            else if (t->prodrule == EXCL_EQ) opcode = O_INE;
            else                                    opcode = O_IGT; 
        
            t->place = new_temp();
            t->code  = concat(
                          concat(t->kids[0]->code, t->kids[1]->code),
                          gen(opcode,
                              t->place,
                              t->kids[0]->place,
                              t->kids[1]->place)
                       );
            return;
        }

        /*unary negation*/
        else if (strcmp(t->symbolname, "negation")==0 && t->nkids>=1) {
            generate_code(t->kids[0]);
            t->place = new_temp();
            t->code  = gen(O_NEG,
                           t->place,
                           t->kids[0]->place,
                           NULL_ADDR);
            return;
        }

        /*logical_not*/
        else if (strcmp(t->symbolname, "logical_not")==0 && t->nkids>=1) {
            generate_code(t->kids[0]);
            t->place = new_temp();
            t->code  = gen(O_NOT,
                           t->place,
                           t->kids[0]->place,
                           NULL_ADDR);
            return;
        }

        else if (strcmp(t->symbolname, "while_statement") == 0 && t->nkids >= 2) {
            struct tree *condition = t->kids[0];
            struct tree *body = t->kids[1];
            
            if (!t->first_used) {
                struct addr *start_label = genlabel();
                t->first = *start_label;
                t->first_used = 1;
                free(start_label);
            }
            
            if (!condition->follow_used) {
                struct addr *cond_false_label = genlabel();
                condition->follow = *cond_false_label;
                condition->follow_used = 1;
                free(cond_false_label);
            }
            
            generate_code(condition);
            
            struct instr *loop_code = gen(D_LABEL, t->first, NULL_ADDR, NULL_ADDR);
            
            loop_code = concat(loop_code, condition->code);
            
            struct instr *branch_exit = gen(O_BZ, condition->follow, condition->place, NULL_ADDR);
            loop_code = concat(loop_code, branch_exit);
            
            generate_code(body);
            loop_code = concat(loop_code, body->code);
            
            struct instr *jump_back = gen(O_BR, t->first, NULL_ADDR, NULL_ADDR);
            loop_code = concat(loop_code, jump_back);
            
            struct instr *exit_label = gen(D_LABEL, condition->follow, NULL_ADDR, NULL_ADDR);
            loop_code = concat(loop_code, exit_label);
            
            t->code = loop_code;
            return;
        }
        
        /*if statement*/
        else if (strcmp(t->symbolname, "ifStatement") == 0 && t->nkids == 2) {
            struct tree *cond = t->kids[0];
            struct tree *then_stmt = t->kids[1];
        
            struct addr *end_label = genlabel();
            struct instr *code = NULL;
        
            generate_code(cond);
            code = concat(code, cond->code);
        
            code = concat(code, gen(O_BZ, *end_label, cond->place, NULL_ADDR));
        
            generate_code(then_stmt);
            code = concat(code, then_stmt->code);
        
            code = concat(code, gen(D_LABEL, *end_label, NULL_ADDR, NULL_ADDR));
        
            t->code = code;
            return;
        }
        else if (strcmp(t->symbolname, "ifElseStatement") == 0 && t->nkids == 3) {
            // printf("DEBUG: Entering ifElseStatement\n");

            struct tree *cond = t->kids[0];
            struct tree *then_branch = t->kids[1];
            struct tree *else_branch = t->kids[2];
        
            struct addr *else_label = genlabel();
            struct addr *end_label = genlabel();
        
            struct instr *code = NULL;
        
            generate_code(cond);
            code = concat(code, cond->code);
            code = concat(code, gen(O_BZ, *else_label, cond->place, NULL_ADDR));
        
            generate_code(then_branch);
            code = concat(code, then_branch->code);
            code = concat(code, gen(O_BR, *end_label, NULL_ADDR, NULL_ADDR));
        
            code = concat(code, gen(D_LABEL, *else_label, NULL_ADDR, NULL_ADDR));
            generate_code(else_branch);
            code = concat(code, else_branch->code);
        
            code = concat(code, gen(D_LABEL, *end_label, NULL_ADDR, NULL_ADDR));
        
            t->code = code;
            return;
        }
        
        /*logical_and expression*/
        else if (strcmp(t->symbolname, "logical_and") == 0 && t->nkids == 2) {
            t->place = new_temp();
            
            struct addr *false_label = genlabel();
            struct addr *end_label = genlabel();
            
            generate_code(t->kids[0]);
            
            struct instr *and_code = t->kids[0]->code;
            
            struct instr *branch_false = gen(O_BZ, *false_label, t->kids[0]->place, NULL_ADDR);
            and_code = concat(and_code, branch_false);
            
            generate_code(t->kids[1]);
            and_code = concat(and_code, t->kids[1]->code);
            
            struct instr *copy_result = gen(O_ASN, t->place, t->kids[1]->place, NULL_ADDR);
            and_code = concat(and_code, copy_result);
            
            struct instr *jump_end = gen(O_BR, *end_label, NULL_ADDR, NULL_ADDR);
            and_code = concat(and_code, jump_end);
            
            struct instr *false_instr = gen(D_LABEL, *false_label, NULL_ADDR, NULL_ADDR);
            and_code = concat(and_code, false_instr);
            
            struct addr zero = { .region = R_IMMED, .u.offset = 0 };
            struct instr *set_false = gen(O_ASN, t->place, zero, NULL_ADDR);
            and_code = concat(and_code, set_false);
            
            struct instr *end_instr = gen(D_LABEL, *end_label, NULL_ADDR, NULL_ADDR);
            and_code = concat(and_code, end_instr);
            
            t->code = and_code;
            free(false_label);
            free(end_label);
            return;
        }
        
        /*logical_or expression*/
        else if (strcmp(t->symbolname, "logical_or") == 0 && t->nkids == 2) {
            t->place = new_temp();
            
            struct addr *true_label = genlabel();
            struct addr *end_label = genlabel();
            
            generate_code(t->kids[0]);
            
            struct instr *or_code = t->kids[0]->code;
            
            struct addr one = { .region = R_IMMED, .u.offset = 1 };
            struct instr *comp_true = gen(O_INE, t->place, t->kids[0]->place, one);
            or_code = concat(or_code, comp_true);
            
            struct instr *branch_true = gen(O_BNZ, *true_label, t->place, NULL_ADDR);
            or_code = concat(or_code, branch_true);
            
            generate_code(t->kids[1]);
            or_code = concat(or_code, t->kids[1]->code);
            
            struct instr *copy_result = gen(O_ASN, t->place, t->kids[1]->place, NULL_ADDR);
            or_code = concat(or_code, copy_result);
            
            struct instr *jump_end = gen(O_BR, *end_label, NULL_ADDR, NULL_ADDR);
            or_code = concat(or_code, jump_end);
            
            struct instr *true_instr = gen(D_LABEL, *true_label, NULL_ADDR, NULL_ADDR);
            or_code = concat(or_code, true_instr);
            
            struct instr *set_true = gen(O_ASN, t->place, one, NULL_ADDR);
            or_code = concat(or_code, set_true);
            
            struct instr *end_instr = gen(D_LABEL, *end_label, NULL_ADDR, NULL_ADDR);
            or_code = concat(or_code, end_instr);
            
            t->code = or_code;
            free(true_label);
            free(end_label);
            return;
        }
        else if (strcmp(t->symbolname, "comparison") == 0 && t->nkids == 2) {
            struct tree *lhs = t->kids[0];
            struct tree *rhs = t->kids[1];
        
            generate_code(lhs);
            generate_code(rhs);
        
            struct instr *code = NULL;
            code = concat(lhs->code, rhs->code);
        
            struct addr tmp = new_temp();
        
            const char *op = t->leaf ? t->leaf->text : NULL;
        
            if (op) {
                if (strcmp(op, "<") == 0)
                    code = concat(code, gen(O_ILT, tmp, lhs->place, rhs->place));
                else if (strcmp(op, "<=") == 0)
                    code = concat(code, gen(O_ILE, tmp, lhs->place, rhs->place));
                else if (strcmp(op, ">") == 0)
                    code = concat(code, gen(O_IGT, tmp, lhs->place, rhs->place));
                else if (strcmp(op, ">=") == 0)
                    code = concat(code, gen(O_IGE, tmp, lhs->place, rhs->place));
                else {
                    fprintf(stderr, "ERROR: unknown comparison op %s\n", op);
                    return;
                }
            }
        
            t->code = code;
            t->place = tmp;
            return;
        }
        else if (strcmp(t->symbolname, "equality") == 0 && t->nkids == 2) {
            struct tree *lhs = t->kids[0];
            struct tree *rhs = t->kids[1];
        
            generate_code(lhs);
            generate_code(rhs);
        
            struct instr *code = NULL;
            code = concat(lhs->code, rhs->code);
        
            struct addr tmp = new_temp();
        
            const char *op = t->leaf ? t->leaf->text : NULL;
            // printf("Equality operator: [%s]\n", op ? op : "NULL");
                    
            if (op) {
                if (strcmp(op, "==") == 0 || strcmp(op, "===") == 0)
                    code = concat(code, gen(O_IEQ, tmp, lhs->place, rhs->place));
                else if (strcmp(op, "!=") == 0)
                    code = concat(code, gen(O_INE, tmp, lhs->place, rhs->place));
                else {
                    fprintf(stderr, "ERROR: unknown equality op: %s\n", op);
                    return;
                }
            }
        
            t->code = code;
            t->place = tmp;
            return;
        }       
        /*function Calls*/
        else if (strcmp(t->symbolname, "functionCall")==0) {
            struct tree *fnNode = t->kids[0];
            SymbolTableEntry fentry = lookup_symbol(globalSymtab, fnNode->leaf->text);
            if (!fentry)
                fentry = lookup_symbol(currentFunctionSymtab, fnNode->leaf->text);
            if (!fentry) {
                // fprintf(stderr, "DEBUG: lookup_symbol failed for function '%s'\n", fnNode->leaf->text);
            } else {
                // fprintf(stderr,
                //     "DEBUG: functionCall '%s' → fentry=%p addr=%s:%d\n",
                //     fnNode->leaf->text,
                //     (void*)fentry,
                //     regionname(fentry->location.region),
                //     fentry->location.u.offset
                // );
            }
            struct addr func_addr = fentry->location;
        
            struct instr *code = NULL;
            struct tree **args = NULL;
            int argc = 0;
            for (int i = 1; i < t->nkids; i++) {
                flattenExprList(t->kids[i], &args, &argc);
            }
        
            for (int i = 0; i < argc; i++) {
                generate_code(args[i]);
                code = concat(code, args[i]->code);
                code = concat(code, gen(O_PARM, NULL_ADDR, args[i]->place, NULL_ADDR));
            }
            free(args);
        
            t->place = new_temp();
        
            code = concat(code, gen(O_CALL, t->place, func_addr, NULL_ADDR));
            t->code = code;
            return;
        }
        

        /*functionDeclaration */
        else if (strcmp(t->symbolname, "functionDeclaration") == 0) {
            SymbolTable oldSymtab = currentFunctionSymtab;
            currentFunctionSymtab = t->scope;
        
            struct addr label_addr = *genlabel();
            SymbolTableEntry fentry = lookup_symbol(globalSymtab, t->kids[0]->leaf->text);
            if (fentry) fentry->location = label_addr;
        
            t->code = gen(D_LABEL, label_addr, NULL_ADDR, NULL_ADDR);
            t->code = concat(t->code,
                             gen(O_PUSH, NULL_ADDR,
                                 (struct addr){ .region=R_FP, .u.offset=0 }, NULL_ADDR));
            t->code = concat(t->code,
                             gen(O_ASN, (struct addr){ .region=R_FP, .u.offset=0 },
                                 (struct addr){ .region=R_SP, .u.offset=0 }, NULL_ADDR));
            t->code = concat(t->code,
                             gen(O_ALLOC, NULL_ADDR,
                                 (struct addr){ .region=R_IMMED, .u.offset=currentFunctionSymtab->nextOffset }, NULL_ADDR));
        
            struct tree *body = NULL;
            for (int i = 0; i < t->nkids; i++) {
                if (t->kids[i] && strcmp(t->kids[i]->symbolname, "block") == 0) {
                    body = t->kids[i];
                    break;
                }
            }
            if (!body) {
                currentFunctionSymtab = oldSymtab;
                return;
            }
        
            generate_code(body);
            t->code = concat(t->code, body->code);
        
            struct instr *last = body->code;
            while (last && last->next) last = last->next;
            if (!last || last->opcode != O_RET) {
                t->code = concat(t->code,
                                 gen(O_DEALLOC, NULL_ADDR,
                                     (struct addr){ .region=R_IMMED, .u.offset=currentFunctionSymtab->nextOffset }, NULL_ADDR));
                t->code = concat(t->code,
                                 gen(O_RET, NULL_ADDR, NULL_ADDR, NULL_ADDR));
            }
        
            currentFunctionSymtab = oldSymtab;
            return;
        }
        
        
    
       /*return Statement*/
        else if (strcmp(t->symbolname, "returnStatement") == 0 && t->nkids == 1) {
            int frameSize = currentFunctionSymtab->nextOffset;
            if (frameSize == 0) frameSize = 8;
            generate_code(t->kids[0]);
            t->place = t->kids[0]->place;
        
            t->code = concat(t->kids[0]->code,
                             gen(O_DEALLOC, NULL_ADDR,
                                 (struct addr){ .region = R_IMMED, .u.offset = frameSize },
                                 NULL_ADDR));
            t->code = concat(t->code,
                             gen(O_RET, NULL_ADDR, t->kids[0]->place, NULL_ADDR));
            return;
        }
        
        else if (strcmp(t->symbolname, "returnStatement") == 0 && t->nkids == 0) {
            int frameSize = currentFunctionSymtab->nextOffset;
            if (frameSize == 0) frameSize = 8;
            t->code = gen(O_DEALLOC, NULL_ADDR,
                          (struct addr){ .region = R_IMMED, .u.offset = frameSize },
                          NULL_ADDR);
            t->code = concat(t->code,
                             gen(O_RET, NULL_ADDR, NULL_ADDR, NULL_ADDR));
            return;
        }
        
        else if ((strcmp(t->symbolname, "variableDeclaration") == 0 ||
                strcmp(t->symbolname, "constVariableDeclaration") == 0)
                && t->nkids >= 3)
        {
            struct tree *id   = t->kids[0];
            struct tree *init = t->kids[2];

            SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, id->leaf->text);
            if (!entry) {
                fprintf(stderr, "ERROR: Unknown variable '%s' in codegen\n", id->leaf->text);
                return;
            }
            id->place = entry->location;

            generate_code(init);
            t->code = init->code;

            t->code = concat(t->code,
                            gen(O_ASN, id->place, init->place, NULL_ADDR));
            t->place = id->place;
            return;
        }
        /*for statement*/
        else if (strcmp(t->symbolname, "forStatementKotlinRange") == 0 && t->nkids == 4) {
            struct tree *loopVar = t->kids[0];     
            struct tree *startExpr = t->kids[1];   
            struct tree *endExpr = t->kids[2];     
            struct tree *body = t->kids[3];        
        
            SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, loopVar->leaf->text);
            if (!entry) {
                fprintf(stderr, "ERROR: loop variable %s not found\n", loopVar->leaf->text);
                return;
            }
        
            struct addr i_addr = entry->location;
        
            generate_code(startExpr);
            generate_code(endExpr);
        
            struct instr *all_code = NULL;
            all_code = concat(all_code, startExpr->code);
            all_code = concat(all_code, endExpr->code);
        
            all_code = concat(all_code, gen(O_ASN, i_addr, startExpr->place, NULL_ADDR));
        
            struct addr *loop_start = genlabel();
            struct addr *loop_end = genlabel();
        
            struct instr *label_loop = gen(D_LABEL, *loop_start, NULL_ADDR, NULL_ADDR);
            all_code = concat(all_code, label_loop);
        
            struct addr cond_result = new_temp();
            all_code = concat(all_code, gen(O_ILE, cond_result, i_addr, endExpr->place));
            all_code = concat(all_code, gen(O_BZ, *loop_end, cond_result, NULL_ADDR));
        
            generate_code(body);
            all_code = concat(all_code, body->code);
        
            struct addr one = { .region = R_IMMED, .u.offset = 1 };
            struct addr inc_result = new_temp();
            all_code = concat(all_code, gen(O_IADD, inc_result, i_addr, one));
            all_code = concat(all_code, gen(O_ASN, i_addr, inc_result, NULL_ADDR));
        
            all_code = concat(all_code, gen(O_BR, *loop_start, NULL_ADDR, NULL_ADDR));
            all_code = concat(all_code, gen(D_LABEL, *loop_end, NULL_ADDR, NULL_ADDR));
        
            t->code = all_code;
            return;
        }
        else if (strcmp(t->symbolname, "forStatement") == 0 && t->nkids == 4) {
            struct tree *init     = t->kids[0]; 
            struct tree *cond     = t->kids[1]; 
            struct tree *update   = t->kids[2]; 
            struct tree *body     = t->kids[3]; 
        
            generate_code(init);
            generate_code(cond);
            generate_code(update);
        
            struct instr *code = NULL;
            code = concat(code, init->code); 
        
            struct addr *loop_start = genlabel();
            struct addr *loop_end = genlabel();
        
            code = concat(code, gen(D_LABEL, *loop_start, NULL_ADDR, NULL_ADDR));
        
            code = concat(code, cond->code);
            code = concat(code, gen(O_BZ, *loop_end, cond->place, NULL_ADDR));
        
            struct addr *prev_break = current_break_label;
            current_break_label = loop_end;
        
            generate_code(body);
            code = concat(code, body->code);
        
            current_break_label = prev_break; 
        
            code = concat(code, update->code);
        
            code = concat(code, gen(O_BR, *loop_start, NULL_ADDR, NULL_ADDR));
            code = concat(code, gen(D_LABEL, *loop_end, NULL_ADDR, NULL_ADDR));
        
            t->code = code;
            return;
        }
        else if (strcmp(t->symbolname, "whileStatement") == 0 && t->nkids == 2) {
            struct tree *cond = t->kids[0];
            struct tree *body = t->kids[1];
        
            generate_code(cond);
        
            struct addr *loop_start = genlabel();
            struct addr *loop_end = genlabel();
        
            struct instr *code = NULL;
        
            code = concat(code, gen(D_LABEL, *loop_start, NULL_ADDR, NULL_ADDR));
        
            generate_code(cond);
            code = concat(code, cond->code);
        
            code = concat(code, gen(O_BZ, *loop_end, cond->place, NULL_ADDR));
        
            struct addr *prev_break_label = current_break_label;
            current_break_label = loop_end;
        
            generate_code(body);
            code = concat(code, body->code);
        
            current_break_label = prev_break_label;
        
            code = concat(code, gen(O_BR, *loop_start, NULL_ADDR, NULL_ADDR));
            code = concat(code, gen(D_LABEL, *loop_end, NULL_ADDR, NULL_ADDR));
        
            t->code = code;
            return;
        }
        /*break statement*/
        else if (strcmp(t->symbolname, "breakStatement") == 0) {
            if (!current_break_label) {
                fprintf(stderr, "ERROR: 'break' used outside of loop.\n");
                return;
            }
            t->code = gen(O_BR, *current_break_label, NULL_ADDR, NULL_ADDR);
            return;
        }
        
    }

    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i]) {
            debug_print("DEBUG: Processing kid %d of node %s\n", i, 
                        t->symbolname ? t->symbolname : "unknown");
            generate_code(t->kids[i]);
        }
    }
    
    struct instr *children_code = NULL;
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] && t->kids[i]->code) {
            children_code = concat(children_code, t->kids[i]->code);
        }
    }
    t->code = children_code;
    
}

static void asm_output_filename(const char *in, char *out, size_t sz) {
    strncpy(out, in, sz-1);
    out[sz-1] = '\0';
    char *dot = strrchr(out, '.');
    if (dot) strcpy(dot, ".s");
    else strncat(out, ".s", sz - strlen(out) - 1);
}

void write_asm_file(const char *input_filename, struct instr *code) {
    char outfn[256];
    asm_output_filename(input_filename, outfn, sizeof outfn);
    FILE *f = fopen(outfn, "w");
    if (!f) { perror(outfn); return; }

    // --- 1) file + .rodata ---
    fprintf(f, "\t.file\t\"%s\"\n", input_filename);
    fprintf(f, "\t.section\t.rodata\n\t.align\t8\n");
    for (int i = 0; i < strcount; i++) {
        fprintf(f,
                ".LC%d:\n"
                "\t.string\t\"%s\"\n",
                i, strtab[i].text);
    }
    fprintf(f, "\t.text\n");

    fprintf(f, "\t.globl\tmain\n");
    fprintf(f, "\t.type\tmain, @function\n");
    fprintf(f, "main:\n");
    fprintf(f, ".LFB0:\n");
    fprintf(f, "\t.cfi_startproc\n");
    fprintf(f, "\tpushq\t%%rbp\n");
    fprintf(f, "\t.cfi_def_cfa_offset 16\n");
    fprintf(f, "\t.cfi_offset 6, -16\n");
    fprintf(f, "\tmovq\t%%rsp, %%rbp\n");
    fprintf(f, "\t.cfi_def_cfa_register 6\n");
    fprintf(f, "\tsubq\t$%d, %%rsp\n",
            currentFunctionSymtab->nextOffset);

    // --- 2) prepare argument‐passing tables ---
    const char *ireg[6] = { "%edi","%esi","%edx","%ecx","%r8d","%r9d" };
    const char *qreg[6] = { "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    int argc = 0, args_off[6], args_is_ptr[6];

    // track when we’re inside a function and its stack frame size
    int inFunction = 0;
    int frameSize  = 0;

    for (struct instr *cur = code; cur; cur = cur->next) {
        switch (cur->opcode) {

          // ————————— PSEUDO‐OPS —————————

          case D_GLOB:
            fprintf(f, "\t.globl\t%s\n",
                    cur->src1.u.name);
            break;

          case D_PROC:
            // start a new function
            inFunction = 1;
            argc       = 0;
            frameSize  = 0;
            fprintf(f,
                "\t.type\t%s, @function\n"
                "%s:\n"
                ".LFB%d:\n"
                "\t.cfi_startproc\n"
                "\tpushq\t%%rbp\n"
                "\t.cfi_def_cfa_offset 16\n"
                "\t.cfi_offset 6, -16\n"
                "\tmovq\t%%rsp, %%rbp\n"
                "\t.cfi_def_cfa_register 6\n",
                cur->src1.u.name,
                cur->src1.u.name,
                cur->src1.u.offset);
            break;

          case D_LABEL:
            // basic‐block label inside current function
            fprintf(f, ".L%d:\n", cur->dest.u.offset);
            break;

          case D_END:
            // finish the current function with its epilogue
            fprintf(f,
                "\taddq\t$%d, %%rsp\n"
                "\tmovl\t$0, %%eax\n"
                "\tleave\n"
                "\t.cfi_def_cfa 7, 8\n"
                "\tret\n"
                ".LFE%d:\n"
                "\t.size\t%s, .-%s\n",
                frameSize,
                cur->src1.u.offset,
                cur->src2.u.name,
                cur->src2.u.name);
            inFunction = 0;
            break;

          // —————— STACK ALLOCATION ——————

          case O_ALLOC:
            // at function entry, this is the frame size
            if (inFunction && frameSize == 0) {
                frameSize = cur->src1.u.offset;
                fprintf(f, "\tsubq\t$%d, %%rsp\n", frameSize);
            } else {
                // a mid‐function alloc (rare)
                fprintf(f, "\tsubq\t$%d, %%rsp\n",
                        cur->src1.u.offset);
            }
            break;

          case O_DEALLOC:
            // ignore the dealloc at function‐end (handled in D_END),
            // but handle any mid‐function deallocs
            if (!inFunction) {
                fprintf(f, "\taddq\t$%d, %%rsp\n",
                        cur->src1.u.offset);
            }
            break;

          // ——————— PARAM / CALL ———————

          case O_PARM:
            // record up to six args
            if (argc < 6) {
                args_off[argc]    = cur->src1.u.offset;
                args_is_ptr[argc] = (cur->src1.region == R_GLOBAL);
                argc++;
            }
            break;

          case O_CALL:
            // move recorded args into registers
            for (int i = 0; i < argc; i++) {
                if (args_is_ptr[i]) {
                    fprintf(f, "\tleaq\t-%d(%%rbp), %s\n",
                            args_off[i], qreg[i]);
                } else {
                    fprintf(f, "\tmovl\t-%d(%%rbp), %s\n",
                            args_off[i], ireg[i]);
                }
            }
            // indirect call via label-number
            fprintf(f, "\tcall\t.L%d\n",
                    cur->src1.u.offset);
            // store return value
            fprintf(f, "\tmovl\t%%eax, -%d(%%rbp)\n",
                    cur->dest.u.offset);
            argc = 0;
            break;

        case O_RET:
            // load return value into %eax if present
            if (cur->src1.region == R_IMMED) {
                fprintf(f, "\tmovl\t$%d, %%eax\n",
                        cur->src1.u.offset);
            } else {
                fprintf(f, "\tmovl\t-%d(%%rbp), %%eax\n",
                        cur->src1.u.offset);
            }
            // fall through to function epilogue
            fprintf(f,
                "\tleave\n"
                "\t.cfi_def_cfa 7, 8\n"
                "\tret\n"
                ".LFE0:\n"
                "\t.size\tmain, .-main\n");
            break;

          // ———————— ARITHMETIC ————————

          case O_ASN:
            if (cur->src1.region == R_IMMED) {
                fprintf(f, "\tmovl\t$%d, %d(%%rbp)\n",
                        cur->src1.u.offset,
                       -cur->dest.u.offset);
            } else {
                fprintf(f,
                    "\tmovl\t%d(%%rbp), %%eax\n"
                    "\tmovl\t%%eax, %d(%%rbp)\n",
                    -cur->src1.u.offset,
                    -cur->dest.u.offset);
            }
            break;

          case O_ADDR:
            fprintf(f,
                "\tleaq\t%d(%%rbp), %%rax\n"
                "\tmovq\t%%rax, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->dest.u.offset);
            break;

          case O_LCONT:
            fprintf(f,
                "\tmovsd\t.LC%d(%%rip), %%xmm0\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                cur->src1.u.offset,
               -cur->dest.u.offset);
            break;

          case O_SCONT:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%%xmm0, .LC%d(%%rip)\n",
               -cur->src1.u.offset,
                cur->dest.u.offset);
            break;

          case O_IADD:
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\taddl\t%d(%%rbp), %%eax\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_ISUB:
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\tsubl\t%d(%%rbp), %%eax\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
               -cur->src2.u.offset,
               -cur->src1.u.offset,
               -cur->dest.u.offset);
            break;

          case O_IMUL:
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\timull\t%d(%%rbp), %%eax\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_IDIV:
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\tcltd\n"
                "\tidivl\t%d(%%rbp)\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_IMOD:
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\tcltd\n"
                "\tidivl\t%d(%%rbp)\n"
                "\tmovl\t%%edx, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_DADD:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\taddsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_DSUB:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tsubsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_DMUL:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tmulsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_DDIV:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tdivsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
               -cur->dest.u.offset);
            break;

          case O_DMOD:
            fprintf(f, "\t# double modulo not implemented\n");
            break;

          case O_NEG:
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\tnegl\t%%eax\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->dest.u.offset);
            break;

        case O_NOT:
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\tcmpl\t$0, %%eax\n"
                "\tsete\t%%al\n"
                "\tmovzbl\t%%al, %%eax\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->dest.u.offset);
            break;

          case O_IEQ: case O_INE:
          case O_ILT: case O_ILE:
          case O_IGT: case O_IGE: {
            const char *mn;
            if      (cur->opcode == O_IEQ) mn = "sete";
            else if (cur->opcode == O_INE) mn = "setne";
            else if (cur->opcode == O_ILT) mn = "setl";
            else if (cur->opcode == O_ILE) mn = "setle";
            else if (cur->opcode == O_IGT) mn = "setg";
            else                             mn = "setge";
            fprintf(f,
                "\tmovl\t%d(%%rbp), %%eax\n"
                "\tcmpl\t%d(%%rbp), %%eax\n"
                "\t%s\t%%al\n"
                "\tmovzbl\t%%al, %%eax\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
               -cur->src1.u.offset,
               -cur->src2.u.offset,
                mn,
               -cur->dest.u.offset);
            break;
          }

          case O_GOTO:
            fprintf(f, "\tjmp\t.L%d\n", cur->dest.u.offset);
            break;

          case O_BR:
            fprintf(f, "\tjmp\t.L%d\n", cur->dest.u.offset);
            break;

          case O_BZ:
            fprintf(f,
                "\tcmpl\t$0, %d(%%rbp)\n"
                "\tje\t.L%d\n",
               -cur->src1.u.offset,
                cur->dest.u.offset);
            break;

          case O_BNZ:
            fprintf(f,
                "\tcmpl\t$0, %d(%%rbp)\n"
                "\tjne\t.L%d\n",
               -cur->src1.u.offset,
                cur->dest.u.offset);
            break;

          case O_BLT: case O_BLE:
          case O_BGT: case O_BGE:
          case O_BEQ: case O_BNE:
          case O_BIF: case O_BNIF: {
            const char *jmn;
            if      (cur->opcode == O_BLT)  jmn = "jl";
            else if (cur->opcode == O_BLE)  jmn = "jle";
            else if (cur->opcode == O_BGT)  jmn = "jg";
            else if (cur->opcode == O_BGE)  jmn = "jge";
            else if (cur->opcode == O_BEQ)  jmn = "je";
            else if (cur->opcode == O_BNE)  jmn = "jne";
            else if (cur->opcode == O_BIF)  jmn = "jnz";
            else                             jmn = "jz";
            fprintf(f,
                "\tcmpl\t%d(%%rbp), %d(%%rbp)\n"
                "\t%s\t.L%d\n",
               -cur->src2.u.offset,
               -cur->src1.u.offset,
                jmn,
                cur->dest.u.offset);
            break;
          }

          case O_PUSH:
            fprintf(f, "\tpushq\t%%rbp\n");
            break;

          case O_POP:
            fprintf(f, "\tpopq\t%%rbp\n");
            break;

          default:
            // any unhandled opcode
            fprintf(f, "\t# unhandled opcode %s\n",
                    opcodename(cur->opcode));
            break;
        }
    }

    fclose(f);
}