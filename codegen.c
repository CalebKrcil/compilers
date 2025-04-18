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
 static void output_operand(FILE *f, struct addr a) {
    if (!f) return;

    // 1) uninitialized
    if (a.region == R_NONE) {
        fprintf(f, "none:%d", a.u.offset);
    }
    // 2) out‑of‑range -> definitely bogus
    else if (a.region < R_GLOBAL || a.region > R_RET) {
        fprintf(f, "invalid:%d", a.u.offset);
    }
    // 3) valid region name lookup
    else {
        fprintf(f, "%s:%d", regionname(a.region), a.u.offset);
    }
}
 
 /*
  * output_instruction
  *   Writes a single TAC instruction to the file.
  *   The format is: <opcode> <dest>, <src1>, <src2>
  */
 static void output_instruction(FILE *f, struct instr *instr) {
     if (!f || !instr) {
         debug_print("DEBUG: output_instruction called with NULL parameter\n");
         return;
     }
     
     debug_print("DEBUG: output_instruction: opcode=%d, dest.region=%d, src1.region=%d, src2.region=%d\n",
                 instr->opcode, instr->dest.region, instr->src1.region, instr->src2.region);
     
     if (instr->opcode < 0) {
         debug_print("WARNING: Invalid opcode %d\n", instr->opcode);
         return;
     }
     
     fprintf(f, "%s\t", opcodename(instr->opcode));
     output_operand(f, instr->dest);
     fprintf(f, ", ");
     output_operand(f, instr->src1);
     fprintf(f, ", ");
     output_operand(f, instr->src2);
     fprintf(f, "\n");
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
    fprintf(stderr,
        "DEBUG: generate_code(%p): symbol=\"%s\" id=%d leaf=\"%s\" currentFunctionSymtab=%p\n",
        t,
        t && t->symbolname    ? t->symbolname    : "(none)",
        t                        ? t->id            : -1,
        t && t->leaf && t->leaf->text ? t->leaf->text : "(none)",
        currentFunctionSymtab
    );
    if (currentFunctionSymtab == NULL) {
        fprintf(stderr, "ERROR: no function symbol table – cannot generate code\n");
        return;
    }
    if (t == NULL) {
        fprintf(stderr, "Warning: generate_code called with NULL tree\n");
        return;
    }
    
    t->code  = NULL;
    t->place = (struct addr){ R_NONE, { .offset = 0 } };

    if (t->leaf) {
        printf("Category: %d\n", t->leaf->category);
        switch (t->leaf->category) {
            case IntegerLiteral: {
                printf("int literal\n");
                // allocate a new temp for this literal
                t->place = new_temp();
                // build an immediate operand whose offset is the *actual* literal value
                struct addr imm;
                imm.region       = R_IMMED;
                imm.u.offset     = t->leaf->value.ival;     // e.g. 1, 2, etc.
                // emit the ASN temp, imm
                struct instr *lit = gen(O_ASN, t->place, imm, NULL_ADDR);
                t->code = concat(t->code, lit);
                debug_print("LIT int %d -> %s:%d\n",
                            t->leaf->value.ival,
                            regionname(t->place.region), t->place.u.offset);
                return;
            }
            case RealLiteral: {
                printf("real literal\n");
                t->place    = new_temp();
                struct addr imm = { .region = R_IMMED, .u = { .offset = (int)t->leaf->value.dval } };
                t->code = concat(t->code, gen(O_ASN, t->place, imm, NULL_ADDR));
                debug_print("LIT real %f -> %s:%d\n",
                            t->leaf->value.dval,
                            regionname(t->place.region), t->place.u.offset);
                return;
            }
            case StringLiteral: {
                // uniquify a label
                static int stringLabelCounter = 0;
                char label[32];
                snprintf(label, sizeof(label), "S%d", stringLabelCounter++);
            
                // record it in a small table for write_ic_file
                add_string_literal(label, t->leaf->text);
            
                // this node’s “place” is that global string label
                t->place.region   = R_GLOBAL;
                t->place.u.offset = stringLabelCounter - 1;  // or store labelIndex
            
                // NO immediate assignment here
                return;
            }
            case BooleanLiteral: {
                printf("boolean literal\n");
                t->place    = new_temp();
                int val      = strcmp(t->leaf->text, "true")==0 ? 1 : 0;
                struct addr imm = { .region = R_IMMED, .u = { .offset = val } };
                t->code = concat(t->code, gen(O_ASN, t->place, imm, NULL_ADDR));
                debug_print("LIT bool %s -> %s:%d\n",
                            t->leaf->text,
                            regionname(t->place.region), t->place.u.offset);
                return;
            }
            case Identifier: {
                fprintf(stderr,
                    "DEBUG: Identifier node: text=\"%s\", currentFunctionSymtab=%p\n",
                    t->leaf->text,
                    currentFunctionSymtab
                );
                printf("identifier\n");
                SymbolTableEntry e = lookup_symbol(currentFunctionSymtab, t->leaf->text);
                printf("after lookup\n");
                if (e) {
                    t->place = e->location;
                    debug_print("ID '%s' -> %s:%d\n",
                                t->leaf->text,
                                regionname(t->place.region), t->place.u.offset);
                } else {
                    t->place = new_temp();
                    debug_print("ID '%s' missing -> temp %s:%d\n",
                                t->leaf->text,
                                regionname(t->place.region), t->place.u.offset);
                }
                printf("after if else\n");
                return;
            }
            default:
                printf("butt\n");
                break;
        }
    }
    
    // Process all children first (postorder traversal)
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] != NULL) {
            generate_code(t->kids[i]);
        }
    }
    
    // Concatenate TAC code from all children - carefully
    struct instr *children_code = NULL;
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] != NULL && t->kids[i]->code != NULL) {
            // Safe concatenation
            if (children_code == NULL) {
                children_code = t->kids[i]->code;
            } else {
                children_code = concat(children_code, t->kids[i]->code);
            }
        }
    }
    
    // Set the node's code to the concatenated children's code
    t->code = children_code;
    
    // Handle non-leaf nodes
    if (t->symbolname != NULL) {
        // Variable Declaration
        /* 1) Variable Declaration */
        if (strcmp(t->symbolname, "variableDeclaration") == 0 && t->nkids >= 3) {
            struct tree *id   = t->kids[0];
            struct tree *init = t->kids[2];
        
            // a) Generate the initializer’s code (preserves child code)
            if (init->place.region == R_NONE) {
                generate_code(init);
            }
            t->code = concat(t->code, init->code);
        
            // b) Do the store into the variable’s slot
            SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, id->leaf->text);
            if (!entry) {
                fprintf(stderr, "Error: variable '%s' not in symtab\n", id->leaf->text);
                return;
            }
            t->place = entry->location;
            t->code  = concat(t->code,
                        gen(O_ASN,
                            t->place,
                            init->place,
                            NULL_ADDR));
            return;
        }

        /* 2) Assignment */
        else if (strcmp(t->symbolname, "assignment") == 0 && t->nkids >= 2) {
            struct tree *lhs = t->kids[0];
            struct tree *rhs = t->kids[1];
        
            // generate RHS code first
            if (rhs->place.region == R_NONE) {
                generate_code(rhs);
            }
            // then reuse the LHS place if needed
            // (lhs is typically a variableReference, whose .place was set during its leaf)
            t->place = lhs->place;
        
            // concatenate: <rhs code> ; ASN lhs, rhs
            t->code = concat(t->code, rhs->code);
            t->code = concat(t->code,
                        gen(O_ASN,
                            lhs->place,
                            rhs->place,
                            NULL_ADDR));
            return;
        }

        /* 3) Binary arithmetic (add, sub, mul, div) */
        else if (strcmp(t->symbolname, "additive_expression") == 0 ||
                strcmp(t->symbolname, "subtractive_expression") == 0 ||
                strcmp(t->symbolname, "multiplicative_expression")== 0 ||
                strcmp(t->symbolname, "division_expression") == 0) {
            // left = kids[0], right = kids[1]
            if (t->kids[0]->place.region == R_NONE) generate_code(t->kids[0]);
            if (t->kids[1]->place.region == R_NONE) generate_code(t->kids[1]);

            int opcode;
            if      (strcmp(t->symbolname, "additive_expression")==0)       opcode = O_IADD;
            else if (strcmp(t->symbolname, "subtractive_expression")==0)    opcode = O_ISUB;
            else if (strcmp(t->symbolname, "multiplicative_expression")==0) opcode = O_IMUL;
            else                                                            opcode = O_IDIV;

            t->place = new_temp();
            t->code  = concat(t->code,
                        gen(opcode,
                            t->place,
                            t->kids[0]->place,
                            t->kids[1]->place));
            return;
        }

        /* 4) Comparison (<, >, <=, >=, ==, !=) */
        else if (strcmp(t->symbolname, "comparison") == 0 && t->nkids >= 3) {
            if (t->kids[0]->place.region == R_NONE) generate_code(t->kids[0]);
            if (t->kids[2]->place.region == R_NONE) generate_code(t->kids[2]);

            int opcode = O_IEQ;
            if (t->kids[1] && t->kids[1]->leaf) {
                char *op = t->kids[1]->leaf->text;
                if      (strcmp(op, "<")==0)  opcode = O_ILT;
                else if (strcmp(op, ">")==0)  opcode = O_IGT;
                else if (strcmp(op, "<=")==0) opcode = O_ILE;
                else if (strcmp(op, ">=")==0) opcode = O_IGE;
                else if (strcmp(op, "!=")==0) opcode = O_INE;
            }

            t->place = new_temp();
            t->code  = concat(t->code,
                        gen(opcode, t->place,
                            t->kids[0]->place,
                            t->kids[2]->place));
            return;
        }

        /* 5) Unary negation */
        else if (strcmp(t->symbolname, "negation") == 0 && t->nkids >= 1) {
            if (t->kids[0]->place.region == R_NONE) generate_code(t->kids[0]);
            t->place = new_temp();
            t->code  = concat(t->code,
                        gen(O_NEG, t->place,
                            t->kids[0]->place,
                            NULL_ADDR));
            return;
        }

        /* 6) Logical NOT */
        else if (strcmp(t->symbolname, "logical_not") == 0 && t->nkids >= 1) {
            if (t->kids[0]->place.region == R_NONE) generate_code(t->kids[0]);
            t->place = new_temp();
            t->code  = concat(t->code,
                        gen(O_NOT, t->place,
                            t->kids[0]->place,
                            NULL_ADDR));
            return;
        }

        /* 7) Logical AND (short‑circuit) */
        else if (strcmp(t->symbolname, "logical_and") == 0 && t->nkids >= 2) {
            if (t->kids[0]->place.region == R_NONE) generate_code(t->kids[0]);
            if (t->kids[1]->place.region == R_NONE) generate_code(t->kids[1]);

            struct addr L_false = *genlabel();
            struct addr L_end   = *genlabel();

            t->place = new_temp();
            /* init false */
            t->code = concat(t->code,
                    gen(O_ASN, t->place,
                        (struct addr){R_IMMED, .u.offset=0},
                        NULL_ADDR));
            /* if first==0 goto L_false */
            t->code = concat(t->code,
                    gen(O_BZ, L_false,
                        t->kids[0]->place,
                        NULL_ADDR));
            /* if second==0 goto L_false */
            t->code = concat(t->code,
                    gen(O_BZ, L_false,
                        t->kids[1]->place,
                        NULL_ADDR));
            /* both true -> result=1 */
            t->code = concat(t->code,
                    gen(O_ASN, t->place,
                        (struct addr){R_IMMED, .u.offset=1},
                        NULL_ADDR));
            /* jump end */
            t->code = concat(t->code,
                    gen(O_BR, L_end, NULL_ADDR, NULL_ADDR));
            /* L_false: */
            t->code = concat(t->code,
                    gen(O_LBL, L_false, NULL_ADDR, NULL_ADDR));
            /* L_end: */
            t->code = concat(t->code,
                    gen(O_LBL, L_end,   NULL_ADDR, NULL_ADDR));
            return;
        }

        /* 8) Logical OR (short‑circuit) */
        else if (strcmp(t->symbolname, "logical_or") == 0 && t->nkids >= 2) {
            if (t->kids[0]->place.region == R_NONE) generate_code(t->kids[0]);
            if (t->kids[1]->place.region == R_NONE) generate_code(t->kids[1]);

            struct addr L_true = *genlabel();
            struct addr L_end  = *genlabel();

            t->place = new_temp();
            /* init false */
            t->code = concat(t->code,
                    gen(O_ASN, t->place,
                        (struct addr){R_IMMED, .u.offset=0},
                        NULL_ADDR));
            /* if first!=0 goto L_true */
            t->code = concat(t->code,
                    gen(O_BNZ, L_true,
                        t->kids[0]->place,
                        NULL_ADDR));
            /* if second!=0 goto L_true */
            t->code = concat(t->code,
                    gen(O_BNZ, L_true,
                        t->kids[1]->place,
                        NULL_ADDR));
            /* jump end (result already 0) */
            t->code = concat(t->code,
                    gen(O_BR, L_end, NULL_ADDR, NULL_ADDR));
            /* L_true: result=1 */
            t->code = concat(t->code,
                    gen(O_LBL, L_true, NULL_ADDR, NULL_ADDR));
            t->code = concat(t->code,
                    gen(O_ASN, t->place,
                        (struct addr){R_IMMED, .u.offset=1},
                        NULL_ADDR));
            /* L_end: */
            t->code = concat(t->code,
                    gen(O_LBL, L_end,  NULL_ADDR, NULL_ADDR));
            return;
        }

        /* 9) Function Call */
        else if (strcmp(t->symbolname, "functionCall") == 0 && t->nkids >= 2) {
            struct tree *callee = t->kids[0];
            struct tree *args   = t->kids[1];
        
            // gen code for the function expression (in case it's e.g. a field access)
            if (callee->place.region == R_NONE) generate_code(callee);
            t->code = concat(t->code, callee->code);
        
            // now for each argument
            for (int i = 0; i < args->nkids; i++) {
                struct tree *arg = args->kids[i];
                if (arg->place.region == R_NONE) {
                    generate_code(arg);
                }
                // push the temp into dest so it prints as local:XX
                t->code = concat(t->code,
                            gen(O_PARM,
                                arg->place,   // dest = the argument temp
                                NULL_ADDR,
                                NULL_ADDR));
            }
        
            // emit the call: dest=temp, src1=callee, src2=NULL
            t->place = new_temp();
            t->code  = concat(t->code,
                        gen(O_CALL,
                            t->place,
                            callee->place,
                            NULL_ADDR));
            return;
        }

        /* 10) Function Declaration */
        else if ((strcmp(t->symbolname, "functionDeclaration")==0 || t->prodrule==327)
                && t->nkids > 0) {
            struct addr entry = *genlabel();
            struct addr exit  = *genlabel();
            /* save old body */
            struct instr *body = t->code;
            t->code = NULL;
            /* prologue */
            t->code = concat(t->code, gen(O_LBL,   entry, NULL_ADDR, NULL_ADDR));
            t->code = concat(t->code, gen(O_PUSH,  NULL_ADDR,
                                        (struct addr){R_FP, .u.offset=0},
                                        NULL_ADDR));
            t->code = concat(t->code, gen(O_ASN,   (struct addr){R_FP, .u.offset=0},
                                        (struct addr){R_SP, .u.offset=0},
                                        NULL_ADDR));
            int sz = currentFunctionSymtab->nextOffset;
            t->code = concat(t->code, gen(O_ALLOC, NULL_ADDR,
                                        (struct addr){R_IMMED, .u.offset=sz},
                                        NULL_ADDR));
            /* body */
            t->code = concat(t->code, body);
            /* epilogue */
            t->code = concat(t->code, gen(O_LBL,   exit, NULL_ADDR, NULL_ADDR));
            t->code = concat(t->code, gen(O_DEALLOC,NULL_ADDR,
                                        (struct addr){R_IMMED, .u.offset=sz},
                                        NULL_ADDR));
            t->code = concat(t->code, gen(O_POP,   (struct addr){R_FP,.u.offset=0},
                                        NULL_ADDR,NULL_ADDR));
            t->code = concat(t->code, gen(O_RET,   NULL_ADDR,NULL_ADDR,NULL_ADDR));
            return;
        }

        /* 11) Return Statement */
        else if (strcmp(t->symbolname, "returnStatement") == 0) {
            if (t->nkids > 0 && t->kids[0]->place.region == R_NONE)
                generate_code(t->kids[0]);
            if (t->nkids > 0) {
                /* move into R_RET */
                t->code = concat(t->code,
                        gen(O_ASN,
                            (struct addr){R_RET,.u.offset=0},
                            t->kids[0]->place,
                            NULL_ADDR));
            }
            t->code = concat(t->code,
                    gen(O_RET, NULL_ADDR, NULL_ADDR, NULL_ADDR));
            return;
        }

        /* 12) Array Access */
        else if (strcmp(t->symbolname, "arrayAccess")==0 && t->nkids>=2) {
            if (t->kids[0]->place.region==R_NONE) generate_code(t->kids[0]);
            if (t->kids[1]->place.region==R_NONE) generate_code(t->kids[1]);

            /* offset = index * 8 */
            struct addr offset = new_temp();
            t->code = concat(t->code,
                    gen(O_IMUL, offset,
                        t->kids[1]->place,
                        (struct addr){R_IMMED,.u.offset=8}));
            /* addr = base + offset */
            t->place = new_temp();
            t->code  = concat(t->code,
                    gen(O_ADDR, t->place,
                        t->kids[0]->place,
                        offset));
            /* load */
            struct addr load = new_temp();
            t->code = concat(t->code,
                    gen(O_LCONT, load, t->place, NULL_ADDR));
            t->place = load;
            return;
        }
        // If-Else Statement
        else if (strcmp(t->symbolname, "ifStatement") == 0) {
            // Generate labels for branches
            struct addr if_false = *genlabel();
            struct addr if_end = *genlabel();
            
            // Ensure condition has a place
            if (t->nkids > 0 && t->kids[0] != NULL) {
                if (t->kids[0]->place.region == R_NONE) {
                    generate_code(t->kids[0]);  // Actually generate code for condition
                }
                
                // Branch if condition is false
                t->code = concat(t->code, gen(O_BZ, if_false, t->kids[0]->place, NULL_ADDR));
            } else {
                fprintf(stderr, "Error: ifStatement has no condition\n");
                return;
            }
            
            // Generate code for 'then' branch (kids[1])
            if (t->nkids > 1 && t->kids[1] != NULL) {
                // Ensure we have code for 'then' branch
                if (t->kids[1]->code == NULL) {
                    generate_code(t->kids[1]);
                }
                t->code = concat(t->code, t->kids[1]->code);
            }
            
            // Jump to end after 'then' branch
            t->code = concat(t->code, gen(O_BR, if_end, NULL_ADDR, NULL_ADDR));
            
            // Label for 'else' branch
            t->code = concat(t->code, gen(O_LBL, if_false, NULL_ADDR, NULL_ADDR));
            
            // Generate code for 'else' branch if it exists (kids[2])
            if (t->nkids > 2 && t->kids[2] != NULL) {
                // Ensure we have code for 'else' branch
                if (t->kids[2]->code == NULL) {
                    generate_code(t->kids[2]);
                }
                t->code = concat(t->code, t->kids[2]->code);
            }
            
            // Label for end of if-statement
            t->code = concat(t->code, gen(O_LBL, if_end, NULL_ADDR, NULL_ADDR));
        }
        // While Loop
        else if (strcmp(t->symbolname, "whileStatement") == 0) {
            // Generate labels for loop
            struct addr loop_start = *genlabel();
            struct addr loop_end = *genlabel();
            
            // Label for loop start
            t->code = concat(t->code, gen(O_LBL, loop_start, NULL_ADDR, NULL_ADDR));
            
            // Generate code for condition
            if (t->nkids > 0 && t->kids[0] != NULL) {
                if (t->kids[0]->place.region == R_NONE) {
                    generate_code(t->kids[0]);  // Actually generate code for condition
                }
                
                // Branch to end if condition is false
                t->code = concat(t->code, gen(O_BZ, loop_end, t->kids[0]->place, NULL_ADDR));
            } else {
                fprintf(stderr, "Error: whileStatement has no condition\n");
                return;
            }
            
            // Generate code for loop body
            if (t->nkids > 1 && t->kids[1] != NULL) {
                // Ensure we have code for body
                if (t->kids[1]->code == NULL) {
                    generate_code(t->kids[1]);
                }
                t->code = concat(t->code, t->kids[1]->code);
            }
            
            // Jump back to start
            t->code = concat(t->code, gen(O_BR, loop_start, NULL_ADDR, NULL_ADDR));
            
            // Label for loop end
            t->code = concat(t->code, gen(O_LBL, loop_end, NULL_ADDR, NULL_ADDR));
        }
        // For Loop
        else if (strcmp(t->symbolname, "forStatement") == 0) {
            // Generate labels for loop
            struct addr loop_start = *genlabel();
            struct addr loop_update = *genlabel();
            struct addr loop_end = *genlabel();
            
            // Generate code for initialization (typically kids[0])
            if (t->nkids > 0 && t->kids[0] != NULL) {
                if (t->kids[0]->code == NULL) {
                    generate_code(t->kids[0]);
                }
                t->code = concat(t->code, t->kids[0]->code);
            }
            
            // Label for loop start
            t->code = concat(t->code, gen(O_LBL, loop_start, NULL_ADDR, NULL_ADDR));
            
            // Generate code for condition (typically kids[1])
            if (t->nkids > 1 && t->kids[1] != NULL) {
                if (t->kids[1]->place.region == R_NONE) {
                    generate_code(t->kids[1]);
                }
                
                // Branch to end if condition is false
                t->code = concat(t->code, gen(O_BZ, loop_end, t->kids[1]->place, NULL_ADDR));
            }
            
            // Generate code for body (typically kids[3])
            if (t->nkids > 3 && t->kids[3] != NULL) {
                if (t->kids[3]->code == NULL) {
                    generate_code(t->kids[3]);
                }
                t->code = concat(t->code, t->kids[3]->code);
            }
            
            // Label for update section
            t->code = concat(t->code, gen(O_LBL, loop_update, NULL_ADDR, NULL_ADDR));
            
            // Generate code for update expression (typically kids[2])
            if (t->nkids > 2 && t->kids[2] != NULL) {
                if (t->kids[2]->code == NULL) {
                    generate_code(t->kids[2]);
                }
                t->code = concat(t->code, t->kids[2]->code);
            }
            
            // Jump back to start
            t->code = concat(t->code, gen(O_BR, loop_start, NULL_ADDR, NULL_ADDR));
            
            // Label for loop end
            t->code = concat(t->code, gen(O_LBL, loop_end, NULL_ADDR, NULL_ADDR));
        }
            // ... other node handlers ...
    }
    /* If no explicit rule applies, t->code remains the concatenation of its children’s code. */
}