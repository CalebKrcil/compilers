/*
 * codegen.c - Code Generation Module for k0 Intermediate Code
 *
 * This module traverses the AST (from tree.h) and synthesizes three-address
 * code (TAC) for the subset of Kotlin supported by k0.
 *
 * It provides functionality such as generating the intermediate code file,
 * printing operands/instructions, and traversing the AST for code generation.
 * The generate_code() function below has been rewritten to perform a complete
 * postorder traversal (mimicking the traversal in semantics.c :contentReference[oaicite:0]{index=0}&#8203;:contentReference[oaicite:1]{index=1}),
 * so that every node in the AST is visited and all instructions are generated.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdarg.h>
 
 #include "tree.h"
 #include "tac.h"
 #include "ytab.h"   // For token categories
 #include "type.h"
 #include "codegen.h"
 
 #define NULL_ADDR ((struct addr){R_NONE, {.offset = 0}})
 #define DEBUG_OUTPUT 1  // Set to 1 to enable debug output, 0 to disable
 
 /* External function prototypes from tac.c */
 extern struct addr new_temp(void);
 extern struct instr *gen(int op, struct addr a1, struct addr a2, struct addr a3);
 extern struct instr *concat(struct instr *l1, struct instr *l2);
 
 /* External functions from tac.c */
 extern char *regionname(int i);
 extern char *opcodename(int i);
 extern char *pseudoname(int i);
 
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
     
     if (a.region < R_NONE) {
         fprintf(f, "invalid:%d", a.u.offset);
     } else {
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
     fprintf(f, "/* no string constants */\n\n");
     
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
  * This complete implementation traverses the AST in postorder.
  * 1. Recursively process all children.
  * 2. Concatenate the TAC instructions from all children.
  * 3. Generate the current node's TAC based on its kind.
  *
  * This implementation handles:
  *   • Literals (IntegerLiteral, RealLiteral, StringLiteral)
  *   • Identifiers
  *   • Variable declarations (variableDeclaration)
  *   • Assignments (assignment)
  *   • Binary arithmetic operators (additive, subtractive, multiplicative, division)
  *   • Comparison operators (comparison)
  *   • Unary negation (negation)
  *   • Function calls (functionCall)
  *
  * Additional node types can be added following the same pattern.
  */
 void generate_code(struct tree *t) {
    if (t == NULL) {
        fprintf(stderr, "Warning: generate_code called with NULL tree\n");
        return;
    }
    
    // Initialize t->code to NULL if not already set
    if (t->code == NULL) {
        t->code = NULL;  // Explicit NULL initialization
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
    
    // Handle leaf nodes
    if (t->leaf != NULL) {
        switch (t->leaf->category) {
            case IntegerLiteral: {
                t->place = new_temp();
                struct addr imm;
                imm.region = R_IMMED;
                imm.u.offset = t->leaf->value.ival;
                struct instr *inst = gen(O_ASN, t->place, imm, NULL_ADDR);
                if (inst != NULL) {
                    t->code = concat(t->code, inst);
                }
                break;
            }
            case RealLiteral: {
                t->place = new_temp();
                struct addr imm;
                imm.region = R_IMMED;
                imm.u.offset = (int)t->leaf->value.dval;
                struct instr *inst = gen(O_ASN, t->place, imm, NULL_ADDR);
                if (inst != NULL) {
                    t->code = concat(t->code, inst);
                }
                break;
            }
            case StringLiteral: {
                t->place = new_temp();
                struct addr imm;
                imm.region = R_IMMED;
                imm.u.offset = 0; /* Placeholder for string constant index */
                struct instr *inst = gen(O_ASN, t->place, imm, NULL_ADDR);
                if (inst != NULL) {
                    t->code = concat(t->code, inst);
                }
                break;
            }
            case Identifier: {
                // Ensure identifier has a place
                if (t->place.region == R_NONE) {
                    t->place = new_temp();
                }
                // No generation needed for simple identifier reference
                break;
            }
            default:
                // For unsupported leaf types, just ensure we have a place
                if (t->place.region == R_NONE) {
                    t->place = new_temp();
                }
                break;
        }
        // For leaf nodes, we've now handled the code generation
        return;
    }
    
    // Handle non-leaf nodes
    if (t->symbolname != NULL) {
        // Variable Declaration
        if (strcmp(t->symbolname, "variableDeclaration") == 0) {
            // Minimal required kids check
            if (t->nkids < 1) {
                fprintf(stderr, "Error: variableDeclaration node has insufficient children\n");
                return;
            }
            
            // Ensure identifier (first child) has a place
            if (t->kids[0] != NULL) {
                if (t->kids[0]->place.region == R_NONE) {
                    t->kids[0]->place = new_temp();
                }
                t->place = t->kids[0]->place;
            } else {
                t->place = new_temp();
            }
            
            // Handle initialization if present and valid
            if (t->nkids > 2 && t->kids[2] != NULL) {
                // Ensure initializer has a place
                if (t->kids[2]->place.region == R_NONE) {
                    // Cannot generate code for initializer without a place
                    fprintf(stderr, "Warning: Variable initializer has no place\n");
                } else {
                    // Generate assignment for initialization
                    struct instr *asn = gen(O_ASN, t->place, t->kids[2]->place, NULL_ADDR);
                    if (asn != NULL) {
                        t->code = concat(t->code, asn);
                    }
                }
            }
        }
        // Assignment
        else if (strcmp(t->symbolname, "assignment") == 0) {
            // Minimal required kids check
            if (t->nkids < 2) {
                fprintf(stderr, "Error: assignment node has insufficient children\n");
                return;
            }
            
            // Ensure both LHS and RHS have places
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                fprintf(stderr, "Error: NULL child in assignment\n");
                return;
            }
            
            if (t->kids[0]->place.region == R_NONE) {
                t->kids[0]->place = new_temp();
            }
            if (t->kids[1]->place.region == R_NONE) {
                t->kids[1]->place = new_temp();
            }
            
            // Generate assignment instruction
            t->place = t->kids[0]->place;
            struct instr *asn = gen(O_ASN, t->kids[0]->place, t->kids[1]->place, NULL_ADDR);
            if (asn != NULL) {
                t->code = concat(t->code, asn);
            }
        }
        // Binary arithmetic operations
        else if (strcmp(t->symbolname, "additive_expression") == 0 || 
                 strcmp(t->symbolname, "subtractive_expression") == 0 ||
                 strcmp(t->symbolname, "multiplicative_expression") == 0 ||
                 strcmp(t->symbolname, "division_expression") == 0) {
            // Minimal required kids check
            if (t->nkids < 2) {
                fprintf(stderr, "Error: binary expression node has insufficient children\n");
                return;
            }
            
            // Ensure both operands have places
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                fprintf(stderr, "Error: NULL child in binary expression\n");
                return;
            }
            
            if (t->kids[0]->place.region == R_NONE) {
                t->kids[0]->place = new_temp();
            }
            if (t->kids[1]->place.region == R_NONE) {
                t->kids[1]->place = new_temp();
            }
            
            // Generate arithmetic instruction based on operation type
            int opcode;
            if (strcmp(t->symbolname, "additive_expression") == 0) {
                opcode = O_IADD;
            } else if (strcmp(t->symbolname, "subtractive_expression") == 0) {
                opcode = O_ISUB;
            } else if (strcmp(t->symbolname, "multiplicative_expression") == 0) {
                opcode = O_IMUL;
            } else { // division_expression
                opcode = O_IDIV;
            }
            
            t->place = new_temp();
            struct instr *arith = gen(opcode, t->place, t->kids[0]->place, t->kids[1]->place);
            if (arith != NULL) {
                t->code = concat(t->code, arith);
            }
        }
        // Comparison operations
        else if (strcmp(t->symbolname, "comparison") == 0) {
            // Minimal required kids check
            if (t->nkids < 2) {
                fprintf(stderr, "Error: comparison node has insufficient children\n");
                return;
            }
            
            // Ensure both operands have places
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                fprintf(stderr, "Error: NULL child in comparison\n");
                return;
            }
            
            if (t->kids[0]->place.region == R_NONE) {
                t->kids[0]->place = new_temp();
            }
            if (t->kids[1]->place.region == R_NONE) {
                t->kids[1]->place = new_temp();
            }
            
            // Generate comparison instruction
            t->place = new_temp();
            struct instr *cmp = gen(O_IEQ, t->place, t->kids[0]->place, t->kids[1]->place);
            if (cmp != NULL) {
                t->code = concat(t->code, cmp);
            }
        }
        // Unary negation
        else if (strcmp(t->symbolname, "negation") == 0) {
            // Minimal required kids check
            if (t->nkids < 1) {
                fprintf(stderr, "Error: negation node has no child\n");
                return;
            }
            
            // Ensure operand has a place
            if (t->kids[0] == NULL) {
                fprintf(stderr, "Error: NULL child in negation\n");
                return;
            }
            
            if (t->kids[0]->place.region == R_NONE) {
                t->kids[0]->place = new_temp();
            }
            
            // Generate negation instruction
            t->place = new_temp();
            struct instr *neg = gen(O_NEG, t->place, t->kids[0]->place, NULL_ADDR);
            if (neg != NULL) {
                t->code = concat(t->code, neg);
            }
        }
        // Function Call
        else if (strcmp(t->symbolname, "functionCall") == 0) {
            // Minimal required kids check
            if (t->nkids < 1) {
                fprintf(stderr, "Error: functionCall node has no function name\n");
                return;
            }
            
            // Ensure function name has a place
            if (t->kids[0] == NULL) {
                fprintf(stderr, "Error: NULL function name in functionCall\n");
                return;
            }
            
            if (t->kids[0]->place.region == R_NONE) {
                t->kids[0]->place = new_temp();
            }
            
            // Generate call instruction
            t->place = new_temp();
            struct instr *call = gen(O_CALL, t->place, t->kids[0]->place, NULL_ADDR);
            if (call != NULL) {
                t->code = concat(t->code, call);
            }
        }
        // Add more node types as needed...
    }
    
    // Default: no special processing needed
    // The node's code is already set to the concatenated children's code
}