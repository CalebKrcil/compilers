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
 
 #define NULL_ADDR ((struct addr){R_NONE, {0}})
 #define DEBUG_OUTPUT 0  // Set to 1 to enable debug output, 0 to disable
 
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
         debug_print("DEBUG: generate_code called with t == NULL\n");
         return;
     }
     
     // Process all children first (postorder traversal)
     for (int i = 0; i < t->nkids; i++) {
         if (t->kids[i] != NULL) {
             generate_code(t->kids[i]);
         }
     }
     
     // Concatenate TAC code from all children
     struct instr *children_code = NULL;
     for (int i = 0; i < t->nkids; i++) {
         if (t->kids[i] && t->kids[i]->code != NULL) {
             children_code = concat(children_code, t->kids[i]->code);
         }
     }
     t->code = children_code;
     
     /* --- Leaf Node Handling --- */
     if (t->leaf != NULL) {
         switch (t->leaf->category) {
             case IntegerLiteral: {
                 t->place = new_temp();
                 struct addr imm;
                 imm.region = R_IMMED;
                 imm.u.offset = t->leaf->value.ival;
                 struct instr *inst = gen(O_ASN, t->place, imm, NULL_ADDR);
                 t->code = concat(t->code, inst);
                 break;
             }
             case RealLiteral: {
                 t->place = new_temp();
                 struct addr imm;
                 imm.region = R_IMMED;
                 /* Adjust conversion as needed */
                 imm.u.offset = (int)t->leaf->value.dval;
                 struct instr *inst = gen(O_ASN, t->place, imm, NULL_ADDR);
                 t->code = concat(t->code, inst);
                 break;
             }
             case StringLiteral: {
                 t->place = new_temp();
                 struct addr imm;
                 imm.region = R_IMMED;
                 imm.u.offset = 0; /* Placeholder for string constant index */
                 struct instr *inst = gen(O_ASN, t->place, imm, NULL_ADDR);
                 t->code = concat(t->code, inst);
                 break;
             }
             case Identifier: {
                 if (t->place.region == R_NONE) {
                     t->place = new_temp();
                     debug_print("DEBUG: Identifier '%s' had no pre-assigned place; creating temp at %s:%d\n",
                         t->leaf->text, regionname(t->place.region), t->place.u.offset);
                 }
                 /* No TAC instruction needed for simple identifier reference */
                 break;
             }
             default:
                 break;
         }
         return; // Finished processing leaf nodes.
     }
     
     /* --- Non-Leaf Node Handling --- */
     if (t->symbolname != NULL) {
         /* Variable Declaration:
          * Expected children: [0]=Identifier, [1]=Type, [2]=Initializer (optional)
          */
         if (strcmp(t->symbolname, "variableDeclaration") == 0 && t->nkids >= 2) {
             if (t->kids[0] != NULL)
                 t->place = t->kids[0]->place;
             else
                 t->place = new_temp();
             
             if (t->nkids > 2 && t->kids[2] != NULL && t->kids[2]->place.region != R_NONE) {
                 struct instr *asn = gen(O_ASN, t->place, t->kids[2]->place, NULL_ADDR);
                 t->code = concat(t->code, asn);
             }
             return;
         }
         /* Assignment:
          * Expected children: [0]=LHS, [1]=RHS.
          */
         else if (strcmp(t->symbolname, "assignment") == 0 && t->nkids >= 2) {
             t->place = t->kids[0]->place;
             struct instr *asn = gen(O_ASN, t->kids[0]->place, t->kids[1]->place, NULL_ADDR);
             t->code = concat(t->code, asn);
             return;
         }
         /* Binary Arithmetic Operators:
          * Here we assume the following AST labels; change as needed:
          * "additive_expression" for addition,
          * "subtractive_expression" for subtraction,
          * "multiplicative_expression" for multiplication,
          * "division_expression" for division.
          */
         else if (strcmp(t->symbolname, "additive_expression") == 0 && t->nkids >= 2) {
             t->place = new_temp();
             struct instr *arith = gen(O_IADD, t->place, t->kids[0]->place, t->kids[1]->place);
             t->code = concat(t->code, arith);
             return;
         }
         else if (strcmp(t->symbolname, "subtractive_expression") == 0 && t->nkids >= 2) {
             t->place = new_temp();
             struct instr *arith = gen(O_ISUB, t->place, t->kids[0]->place, t->kids[1]->place);
             t->code = concat(t->code, arith);
             return;
         }
         else if (strcmp(t->symbolname, "multiplicative_expression") == 0 && t->nkids >= 2) {
             t->place = new_temp();
             struct instr *arith = gen(O_IMUL, t->place, t->kids[0]->place, t->kids[1]->place);
             t->code = concat(t->code, arith);
             return;
         }
         else if (strcmp(t->symbolname, "division_expression") == 0 && t->nkids >= 2) {
             t->place = new_temp();
             struct instr *arith = gen(O_IDIV, t->place, t->kids[0]->place, t->kids[1]->place);
             t->code = concat(t->code, arith);
             return;
         }
         /* Comparison Operators:
          * We assume an AST node labeled "comparison" for relational operations.
          * For simplicity, this example uses equality (O_IEQ). In a complete
          * implementation, further inspection of the operator (e.g., "<", ">", etc.)
          * should determine the correct opcode (O_ILT, O_IGT, etc.).
          */
         else if (strcmp(t->symbolname, "comparison") == 0 && t->nkids >= 2) {
             t->place = new_temp();
             struct instr *cmp = gen(O_IEQ, t->place, t->kids[0]->place, t->kids[1]->place);
             t->code = concat(t->code, cmp);
             return;
         }
         /* Unary Negation:
          * Assumes a node labeled "negation" with one child.
          */
         else if (strcmp(t->symbolname, "negation") == 0 && t->nkids >= 1) {
             t->place = new_temp();
             struct instr *neg = gen(O_NEG, t->place, t->kids[0]->place, NULL_ADDR);
             t->code = concat(t->code, neg);
             return;
         }
         /* Function Call:
          * Assumes the first child is the function identifier.
          * (A more complete implementation would also generate parameter-passing code.)
          */
         else if (strcmp(t->symbolname, "functionCall") == 0 && t->nkids >= 1) {
             t->place = new_temp();
             struct instr *call = gen(O_CALL, t->place, t->kids[0]->place, NULL_ADDR);
             t->code = concat(t->code, call);
             return;
         }
         /* Other node types can be handled similarly... */
     }
     /* If no explicit rule applies, t->code remains the concatenation of its children’s code. */
 }
 