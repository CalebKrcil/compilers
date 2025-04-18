/*
 * codegen.c - Code Generation Module for k0 Intermediate Code
 *
 * This module traverses the AST (from tree.h) and synthesizes three-address
 * code (TAC) for the subset of Kotlin supported by k0.
 *
 * It provides functionality such as generating the intermediate code file,
 * printing operands/instructions, and traversing the AST for code generation.
 * The generate_code() function below performs a complete postorder traversal
 * so that every node in the AST is visited and all instructions are generated.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdarg.h>
 
 #include "tree.h"
 #include "tac.h"
 // #include "ytab.h"   // For token categories
 #include "k0gram.tab.h"
 #include "type.h"
 #include "codegen.h"
 #include "symtab.h"
 
 #define NULL_ADDR ((struct addr){R_NONE, {.offset = 0}})
 #define DEBUG_OUTPUT 1  // Set to 1 to enable debug output, 0 to disable
 
 extern SymbolTable currentFunctionSymtab;

 /* External function prototypes from tac.c */
 extern struct addr new_temp(void);
 extern struct addr *genlabel(void);
 extern struct instr *gen(int op, struct addr a1, struct addr a2, struct addr a3);
 extern struct instr *concat(struct instr *l1, struct instr *l2);
 
 /* External functions from tac.c */
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
 
 /* Debug print function */
 static void debug_print(const char *format, ...) {
     if (DEBUG_OUTPUT) {
         va_list args;
         va_start(args, format);
         vfprintf(stderr, format, args);
         va_end(args);
     }
 }
 
 /*
  * generate_output_filename
  *   Given an input filename (e.g. "foo.kt"), produce an output filename
  *   (e.g. "foo.ic") by replacing its extension with ".ic".
  */
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
 
 /*
  * output_operand
  *   Writes a single operand to the file in the format: region:offset.
  */
static void format_operand(struct addr a, char *buf, size_t sz) {
    if (a.region == R_NAME && a.u.name) {
        // print the function name (or any name region)
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

 /*
  * output_instruction
  *   Writes a single TAC instruction to the file.
  *   The format is: <opcode> <dest>, <src1>, <src2>
  */
 static void output_instruction(FILE *f, struct instr *instr) {
    if (!f || !instr) return;

    char destbuf[32], src1buf[32], src2buf[32];

    // render each operand into a string
    format_operand(instr->dest,  destbuf, sizeof destbuf);
    format_operand(instr->src1, src1buf, sizeof src1buf);
    format_operand(instr->src2, src2buf, sizeof src2buf);

    // fixed‑width columns: opcode (8 cols), then each operand (16 cols)
    fprintf(f, "%-8s  %-16s  %-16s  %-16s\n",
            opcodename(instr->opcode),
            destbuf,
            src1buf,
            src2buf);
}
 
 /*
  * write_ic_file
  *   Writes out the intermediate code file (.ic) for the program.
  *   The file is partitioned into three sections: .string, .data, and .code.
  */
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
        // print each literal label and its text
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
 
 /*
  * generate_code
  *
  * This enhanced implementation traverses the AST in postorder and handles a comprehensive
  * set of language constructs:
  * 
  * Expressions:
  *   • Literals (IntegerLiteral, RealLiteral, StringLiteral)
  *   • Identifiers
  *   • Binary arithmetic operators (additive, subtractive, multiplicative, division)
  *   • Comparison operators
  *   • Logical operators (AND, OR)
  *   • Unary operations (negation, NOT)
  *
  * Statements:
  *   • Variable declarations and assignments
  *   • Function calls and declarations
  *   • Control flow statements (if-else, while loops, for loops)
  *   • Return statements
  *
  * Array operations:
  *   • Array access
  *   • Array creation
  */
 void generate_code(struct tree *t) {
    if (!t) return;

    debug_print("Processing node: %s\n", t->symbolname?t->symbolname:"unnamed");
    t->code  = NULL;
    t->place = (struct addr){ R_NONE, { .offset = 0 } };

    /* --- unwrap any single‐child literal wrapper nodes so leaf‐case fires --- */
    if (!t->leaf && t->nkids == 1 && t->kids[0] && t->kids[0]->leaf) {
        generate_code(t->kids[0]);
        t->place = t->kids[0]->place;
        t->code  = t->kids[0]->code;
        return;
    }

    /* Print detailed node info for debugging */
    if (t->leaf) {
        debug_print("Leaf node: category=%d, text=%s\n", 
                   t->leaf->category,
                   t->leaf->text ? t->leaf->text : "null");
    } else {
        debug_print("Interior node with %d kids\n", t->nkids);
    }
    /* 1) Leaf nodes (literals & identifiers) */
    if (t->leaf) {
        printf("Category: %d\n", t->leaf->category);
        switch (t->leaf->category) {
            case IntegerLiteral: {
                printf("int literal\n");
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
                printf("real literal\n");
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
                char label[32];
                snprintf(label, sizeof(label), "S%d", stringLabelCounter++);
                add_string_literal(label, t->leaf->text);
                t->place.region   = R_GLOBAL;
                t->place.u.offset = stringLabelCounter - 1;
                return;
            }
            case BooleanLiteral: {
                printf("boolean literal\n");
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
                fprintf(stderr,
                    "DEBUG: Identifier node: text=\"%s\"\n",
                    t->leaf->text);
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

    /* 2) Special‑cases (in order) */
    if (t->symbolname) {
        /* -- variableDeclaration -- */
        fprintf(stderr,"NODE: symbolname=%s, prodrule=%d\n",
            t->symbolname? t->symbolname:"(null)",
            t->prodrule);

        /* -- assignment -- */
        if (strcmp(t->symbolname, "assignment")==0 && t->nkids == 2) {
            struct tree *lhs = t->kids[0]; 
            struct tree *rhs = t->kids[1];
            
            // Handle direct identifier on left side
            if (lhs->leaf && lhs->leaf->category == Identifier) {
                SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, lhs->leaf->text);
                if (!entry) {
                    fprintf(stderr, "ERROR: Variable '%s' not found in symbol table\n", 
                            lhs->leaf->text);
                    t->place = new_temp(); // Assign place to avoid cascading errors
                    return;
                }
                lhs->place = entry->location;
            } else {
                // For more complex LHS expressions
                if (lhs) generate_code(lhs);
            }
            
            // Generate code for the right side
            if (rhs) generate_code(rhs);
            
            // Ensure both sides have valid places
            if (lhs->place.region == R_NONE || rhs->place.region == R_NONE) {
                fprintf(stderr, "ERROR: Invalid places in assignment\n");
                t->place = lhs->place; // Still use LHS as result 
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

        /* -- additive_expression -- */
        else if (strcmp(t->symbolname, "additive_expression")==0 && t->nkids>=2) {
            // Force evaluate both sides
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);
            
            // Debug what we have
            debug_print("Additive expression: left=%s:%d, right=%s:%d\n",
                       regionname(t->kids[0]->place.region), t->kids[0]->place.u.offset,
                       regionname(t->kids[1]->place.region), t->kids[1]->place.u.offset);
            
            // Create a temporary for result
            t->place = new_temp();
            
            // Generate the addition instruction directly
            struct instr *add_instr = gen(O_IADD, 
                                         t->place,
                                         t->kids[0]->place,
                                         t->kids[1]->place);
            
            // Explicitly print what we generated
            debug_print("Generated IADD: %s:%d = %s:%d + %s:%d\n",
                       regionname(t->place.region), t->place.u.offset,
                       regionname(t->kids[0]->place.region), t->kids[0]->place.u.offset,
                       regionname(t->kids[1]->place.region), t->kids[1]->place.u.offset);
            
            // Be very explicit about concatenation
            struct instr *left_code = t->kids[0]->code;
            struct instr *right_code = t->kids[1]->code;
            struct instr *combined = NULL;
            
            if (left_code) {
                combined = left_code;
                // Find the end of the left code list
                struct instr *end = left_code;
                while (end && end->next) end = end->next;
                
                // Manually attach right code
                if (end && right_code) end->next = right_code;
            } else {
                combined = right_code;
            }
            
            // Now manually attach add instruction
            if (combined) {
                struct instr *end = combined;
                while (end && end->next) end = end->next;
                if (end) end->next = add_instr;
                else combined = add_instr;
            } else {
                combined = add_instr;
            }
            
            t->code = combined;
            return;
        }
        
        /*
         * Similarly, fix the subtractive_expression, multiplicative_expression,
         * and division_expression handlers
         */
        
        /* -- subtractive_expression -- */
        else if (strcmp(t->symbolname, "subtractive_expression")==0 && t->nkids>=2) {
            if (t->kids[0]) generate_code(t->kids[0]);
            if (t->kids[1]) generate_code(t->kids[1]);
            
            if (!t->kids[0] || !t->kids[1] || 
                t->kids[0]->place.region == R_NONE || 
                t->kids[1]->place.region == R_NONE) {
                fprintf(stderr, "ERROR: Invalid operands in subtractive expression\n");
                t->place = new_temp();
                return;
            }
            
            struct instr *both = concat(t->kids[0]->code, t->kids[1]->code);
            t->place = new_temp();
            t->code = concat(both, 
                            gen(O_ISUB,
                                t->place,
                                t->kids[0]->place,
                                t->kids[1]->place));
            return;
        }

        /* -- multiplicative_expression -- */
        else if (strcmp(t->symbolname, "multiplicative_expression")==0 && t->nkids>=2) {
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);

            struct instr *both = concat(t->kids[0]->code,
                                        t->kids[1]->code);
            t->place = new_temp();
            t->code  = concat(both,
                        gen(O_IMUL,
                            t->place,
                            t->kids[0]->place,
                            t->kids[1]->place));
            return;
        }

        /* -- division_expression -- */
        else if (strcmp(t->symbolname, "division_expression")==0 && t->nkids>=2) {
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);

            struct instr *both = concat(t->kids[0]->code,
                                        t->kids[1]->code);
            t->place = new_temp();
            t->code  = concat(both,
                        gen(O_IDIV,
                            t->place,
                            t->kids[0]->place,
                            t->kids[1]->place));
            return;
        }

        /* -- comparison (<,>,<=,>=,==,!=) -- */
        else if (strcmp(t->symbolname, "comparison")==0 && t->nkids==2) {
            // generate code for both operands
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);
        
            // pick the right opcode based on the production (or symbolname/prodrule)
            int opcode;
            // if you have access to t->prodrule constants for '>' and '<':
            if      (t->prodrule == RANGLE) opcode = O_IGT;
            else if (t->prodrule == LANGLE) opcode = O_ILT;
            else if (t->prodrule == GE) opcode = O_IGE;
            else if (t->prodrule == LE) opcode = O_ILE;
            else if (t->prodrule == EQEQ) opcode = O_IEQ;
            else if (t->prodrule == EXCL_EQ) opcode = O_INE;
            else                                    opcode = O_IGT;  // fallback
        
            // stitch their code together, emit the compare, and return
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

        /* -- unary negation -- */
        else if (strcmp(t->symbolname, "negation")==0 && t->nkids>=1) {
            generate_code(t->kids[0]);
            t->place = new_temp();
            t->code  = gen(O_NEG,
                           t->place,
                           t->kids[0]->place,
                           NULL_ADDR);
            return;
        }

        /* -- logical_not -- */
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
            
            // Generate labels if not already assigned
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
            
            // Generate code for condition with branching
            generate_code(condition);
            
            // Start with the loop label
            struct instr *loop_code = gen(D_LABEL, t->first, NULL_ADDR, NULL_ADDR);
            
            // Add the condition code
            loop_code = concat(loop_code, condition->code);
            
            // Branch if condition is false (exit loop)
            struct instr *branch_exit = gen(O_BZ, condition->follow, condition->place, NULL_ADDR);
            loop_code = concat(loop_code, branch_exit);
            
            // Generate and add the loop body code
            generate_code(body);
            loop_code = concat(loop_code, body->code);
            
            // Add jump back to beginning of loop
            struct instr *jump_back = gen(O_BR, t->first, NULL_ADDR, NULL_ADDR);
            loop_code = concat(loop_code, jump_back);
            
            // Add the exit label
            struct instr *exit_label = gen(D_LABEL, condition->follow, NULL_ADDR, NULL_ADDR);
            loop_code = concat(loop_code, exit_label);
            
            t->code = loop_code;
            return;
        }
        
        /* -- if statement -- */
        else if (strcmp(t->symbolname, "if_statement") == 0 && t->nkids >= 2) {
            struct tree *condition = t->kids[0];
            struct tree *then_clause = t->kids[1];
            struct tree *else_clause = (t->nkids > 2) ? t->kids[2] : NULL;
            
            // Generate labels if not already assigned
            if (!then_clause->first_used) {
                struct addr *then_label = genlabel();
                then_clause->first = *then_label;
                then_clause->first_used = 1;
                free(then_label);
            }
            
            if (!t->follow_used) {
                struct addr *end_label = genlabel();
                t->follow = *end_label;
                t->follow_used = 1;
            }
            
            // Generate code for condition
            generate_code(condition);
            
            // Start with condition code
            struct instr *if_code = condition->code;
            
            if (else_clause) {
                // If we have an else clause, generate its label
                if (!else_clause->first_used) {
                    struct addr *else_label = genlabel();
                    else_clause->first = *else_label;
                    else_clause->first_used = 1;
                    free(else_label);
                }
                
                // Branch to else if condition is false
                struct instr *branch_else = gen(O_BZ, else_clause->first, condition->place, NULL_ADDR);
                if_code = concat(if_code, branch_else);
                
                // Generate then clause code
                generate_code(then_clause);
                if_code = concat(if_code, then_clause->code);
                
                // Add jump to end after then clause
                struct instr *jump_end = gen(O_BR, t->follow, NULL_ADDR, NULL_ADDR);
                if_code = concat(if_code, jump_end);
                
                // Add else label
                struct instr *else_label = gen(D_LABEL, else_clause->first, NULL_ADDR, NULL_ADDR);
                if_code = concat(if_code, else_label);
                
                // Generate and add else code
                generate_code(else_clause);
                if_code = concat(if_code, else_clause->code);
            } else {
                // No else clause - branch to end if condition is false
                struct instr *branch_end = gen(O_BZ, t->follow, condition->place, NULL_ADDR);
                if_code = concat(if_code, branch_end);
                
                // Generate and add then code
                generate_code(then_clause);
                if_code = concat(if_code, then_clause->code);
            }
            
            // Add end label
            struct instr *end_label = gen(D_LABEL, t->follow, NULL_ADDR, NULL_ADDR);
            if_code = concat(if_code, end_label);
            
            t->code = if_code;
            return;
        }
        
        /* -- logical_and expression -- */
        else if (strcmp(t->symbolname, "logical_and") == 0 && t->nkids == 2) {
            // Generate temporary for result
            t->place = new_temp();
            
            // Generate labels
            struct addr *false_label = genlabel();
            struct addr *end_label = genlabel();
            
            // Generate code for left operand
            generate_code(t->kids[0]);
            
            // Start building code
            struct instr *and_code = t->kids[0]->code;
            
            // If left operand is false, short-circuit
            struct instr *branch_false = gen(O_BZ, *false_label, t->kids[0]->place, NULL_ADDR);
            and_code = concat(and_code, branch_false);
            
            // Generate code for right operand
            generate_code(t->kids[1]);
            and_code = concat(and_code, t->kids[1]->code);
            
            // Copy right result to output
            struct instr *copy_result = gen(O_ASN, t->place, t->kids[1]->place, NULL_ADDR);
            and_code = concat(and_code, copy_result);
            
            // Jump to end
            struct instr *jump_end = gen(O_BR, *end_label, NULL_ADDR, NULL_ADDR);
            and_code = concat(and_code, jump_end);
            
            // False label - set result to 0
            struct instr *false_instr = gen(D_LABEL, *false_label, NULL_ADDR, NULL_ADDR);
            and_code = concat(and_code, false_instr);
            
            struct addr zero = { .region = R_IMMED, .u.offset = 0 };
            struct instr *set_false = gen(O_ASN, t->place, zero, NULL_ADDR);
            and_code = concat(and_code, set_false);
            
            // End label
            struct instr *end_instr = gen(D_LABEL, *end_label, NULL_ADDR, NULL_ADDR);
            and_code = concat(and_code, end_instr);
            
            t->code = and_code;
            free(false_label);
            free(end_label);
            return;
        }
        
        /* -- logical_or expression -- */
        else if (strcmp(t->symbolname, "logical_or") == 0 && t->nkids == 2) {
            // Generate temporary for result
            t->place = new_temp();
            
            // Generate labels
            struct addr *true_label = genlabel();
            struct addr *end_label = genlabel();
            
            // Generate code for left operand
            generate_code(t->kids[0]);
            
            // Start building code
            struct instr *or_code = t->kids[0]->code;
            
            // If left operand is true, short-circuit
            struct addr one = { .region = R_IMMED, .u.offset = 1 };
            struct instr *comp_true = gen(O_INE, t->place, t->kids[0]->place, one);
            or_code = concat(or_code, comp_true);
            
            struct instr *branch_true = gen(O_BNZ, *true_label, t->place, NULL_ADDR);
            or_code = concat(or_code, branch_true);
            
            // Generate code for right operand
            generate_code(t->kids[1]);
            or_code = concat(or_code, t->kids[1]->code);
            
            // Copy right result to output
            struct instr *copy_result = gen(O_ASN, t->place, t->kids[1]->place, NULL_ADDR);
            or_code = concat(or_code, copy_result);
            
            // Jump to end
            struct instr *jump_end = gen(O_BR, *end_label, NULL_ADDR, NULL_ADDR);
            or_code = concat(or_code, jump_end);
            
            // True label - set result to 1
            struct instr *true_instr = gen(D_LABEL, *true_label, NULL_ADDR, NULL_ADDR);
            or_code = concat(or_code, true_instr);
            
            struct instr *set_true = gen(O_ASN, t->place, one, NULL_ADDR);
            or_code = concat(or_code, set_true);
            
            // End label
            struct instr *end_instr = gen(D_LABEL, *end_label, NULL_ADDR, NULL_ADDR);
            or_code = concat(or_code, end_instr);
            
            t->code = or_code;
            free(true_label);
            free(end_label);
            return;
        }

        else if (strcmp(t->symbolname, "functionCall")==0 && t->nkids >= 1) {
            struct tree *func_name = t->kids[0];
            if (!func_name || !func_name->leaf) {
                fprintf(stderr, "Function call without function name\n");
                return;
            }
            
            debug_print("Function call to: %s\n", func_name->leaf->text);
            
            // Process all arguments first
            struct instr *all_code = NULL;
            for (int i = 1; i < t->nkids; i++) {
                if (t->kids[i]) {
                    generate_code(t->kids[i]);
                    all_code = concat(all_code, t->kids[i]->code);
                    
                    // Generate parameter passing code
                    if (t->kids[i]->place.region != R_NONE) {
                        struct instr *param = gen(O_PARM, 
                                                 NULL_ADDR, 
                                                 t->kids[i]->place, 
                                                 NULL_ADDR);
                        all_code = concat(all_code, param);
                        debug_print("Added parameter from %s:%d\n", 
                                   regionname(t->kids[i]->place.region),
                                   t->kids[i]->place.u.offset);
                    }
                }
            }
            
            // Create function call
            t->place = new_temp();
            struct addr func_addr = { .region = R_NAME, .u.name = strdup(func_name->leaf->text) };
            
            struct instr *call = gen(O_CALL, t->place, func_addr, NULL_ADDR);
            all_code = concat(all_code, call);
            
            t->code = all_code;
            return;
        }

        /* -- functionDeclaration (prologue/body/epilogue) -- */
        else if (strcmp(t->symbolname, "functionDeclaration")==0) {
            if (t->nkids < 3 || t->kids[2] == NULL) {
                fprintf(stderr, "ERROR: functionDeclaration node has only %d kids; expected 3\n",
                        t->nkids);
                return;
            }
            
            // Generate a proper label for the function
            struct addr label_addr = *genlabel();
            // Make sure the label is properly created
            struct instr *label_instr = gen(D_LABEL, label_addr, NULL_ADDR, NULL_ADDR);
            // Debug to verify label creation
            debug_print("Generated label: %d\n", label_addr.u.offset);
            t->code = label_instr;  // Start with the label
            
            // 1) Generate the body first
            struct tree *bodyNode = t->kids[2];
            generate_code(bodyNode);
            struct instr *bodyCode = bodyNode->code;
            
            // 2) Compute stack frame size
            int frameSize = currentFunctionSymtab->nextOffset;
            if (frameSize == 0) frameSize = 8; // Minimum frame size
            
            // 3) Prologue
            t->code = NULL;
            t->code = concat(t->code,
                           gen(D_LABEL, label_addr, NULL_ADDR, NULL_ADDR));
            t->code = concat(t->code,
                           gen(O_PUSH, NULL_ADDR, 
                              (struct addr){ .region=R_FP, .u.offset=0 }, 
                              NULL_ADDR));
            t->code = concat(t->code,
                           gen(O_ASN,
                              (struct addr){ .region=R_FP, .u.offset=0 },
                              (struct addr){ .region=R_SP, .u.offset=0 },
                              NULL_ADDR));
            t->code = concat(t->code,
                           gen(O_ALLOC,
                              NULL_ADDR,
                              (struct addr){ .region=R_IMMED, .u.offset=frameSize },
                              NULL_ADDR));
            
            // 4) The user's function body
            t->code = concat(t->code, bodyCode);
            
            // 5) Epilogue
            t->code = concat(t->code,
                           gen(O_DEALLOC,
                              NULL_ADDR,
                              (struct addr){ .region=R_IMMED, .u.offset=frameSize },
                              NULL_ADDR));
            t->code = concat(t->code,
                           gen(O_RET, NULL_ADDR, NULL_ADDR, NULL_ADDR));
            
            debug_print("DEBUG: Generated function '%s' with frame size %d\n", 
                       t->kids[0]->leaf->text, frameSize);
            return;
        }

        else if (strcmp(t->symbolname, "variableDeclaration")==0 && t->nkids>=3) {
            struct tree *id = t->kids[0];
            struct tree *init = t->kids[2];
            
            // Force generate code for initializer
            if (init) generate_code(init);
            
            // Debug output to see what's happening
            debug_print("Variable declaration: %s, initializer place: %s:%d\n", 
                        id->leaf->text, 
                        init ? regionname(init->place.region) : "none",
                        init ? init->place.u.offset : -1);
            
            // Look up variable in symbol table
            SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, id->leaf->text);
            if (!entry) {
                fprintf(stderr, "ERROR: Variable '%s' not in symbol table\n", id->leaf->text);
                return;
            }
            
            t->place = entry->location;
            
            // Generate assignment regardless of initializer (for debugging)
            if (init) {
                struct instr *assign = gen(O_ASN,
                                          entry->location,
                                          init->place,
                                          NULL_ADDR);
                debug_print("Generated variable assignment: %s:%d = %s:%d\n",
                           regionname(entry->location.region), entry->location.u.offset,
                           regionname(init->place.region), init->place.u.offset);
                t->code = concat(init->code, assign);
            } else {
                // Even without initializer, create an empty assignment for debugging
                struct addr zero = { .region = R_IMMED, .u.offset = 0 };
                t->code = gen(O_ASN, entry->location, zero, NULL_ADDR);
            }
            return;
        }
    }

    /* 3) Fallback: generic postorder traversal */
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i]) {
            debug_print("DEBUG: Processing kid %d of node %s\n", i, 
                        t->symbolname ? t->symbolname : "unknown");
            generate_code(t->kids[i]);
        }
    }
    
    // Safely concatenate code from all children
    struct instr *children_code = NULL;
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] && t->kids[i]->code) {
            children_code = concat(children_code, t->kids[i]->code);
        }
    }
    t->code = children_code;
}
