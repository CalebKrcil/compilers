/* codegen.c - Code Generation Module for k0 Intermediate Code
 *
 * This module traverses the AST (from tree.h) and synthesizes three-address
 * code (TAC) for the subset of Kotlin supported by k0.
 *
 * Assumptions:
 *   - The AST nodes (struct tree) now include:
 *         struct addr place;      // The memory address (e.g., temporary or variable)
 *         struct instr *code;     // The linked list of TAC instructions computing the node's value
 *   - TAC instructions and associated helpers are defined in tac.h/tac.c.
 *   - Token categories (e.g., IntegerLiteral, Identifier) are defined (e.g., in ytab.h).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "tac.h"
#include "ytab.h"  // For token categories
#include "type.h"
#include "codegen.h"

#define NULL_ADDR ((struct addr){R_NONE, {0}})

/* External function prototypes from tac.c */
extern struct addr new_temp(void);
extern struct instr *gen(int op, struct addr a1, struct addr a2, struct addr a3);
extern struct instr *concat(struct instr *l1, struct instr *l2);

/* External functions from tac.c */
extern char *regionname(int i);
extern char *opcodename(int i);

/* 
 * generate_output_filename
 *  Given an input filename (e.g. "foo.kt"), produce an output filename (e.g. "foo.ic").
 */
static void generate_output_filename(const char *input_filename, char *output_filename, size_t size) {
    /* Copy input_filename to output_filename */
    strncpy(output_filename, input_filename, size - 1);
    output_filename[size - 1] = '\0';
    /* Replace extension with ".ic". If there is a dot, replace; otherwise, append. */
    char *dot = strrchr(output_filename, '.');
    if (dot != NULL) {
        strcpy(dot, ".ic");
    } else {
        strncat(output_filename, ".ic", size - strlen(output_filename) - 1);
    }
}

/*
 * output_operand
 *  Writes a single operand in the format: region:offset.
 */
static void output_operand(FILE *f, struct addr a) {
    fprintf(f, "%s:%d", regionname(a.region), a.u.offset);
}

/*
 * output_instruction
 *  Writes a single TAC instruction to the file.
 *  The format is: <opcode> <dest>, <src1>, <src2>
 */
static void output_instruction(FILE *f, struct instr *instr) {
    if (instr == NULL) {
        fprintf(stderr, "DEBUG: output_instruction called with NULL instr pointer\n");
        return;
    }
    /* Print details about the instruction for debugging */
    fprintf(stderr, "DEBUG: output_instruction: opcode=%d, dest.region=%d, src1.region=%d, src2.region=%d\n",
            instr->opcode, instr->dest.region, instr->src1.region, instr->src2.region);

    /* Optionally, if region values are out of range, print a warning */
    if (instr->dest.region < R_GLOBAL || instr->src1.region < R_GLOBAL || instr->src2.region < R_GLOBAL) {
        fprintf(stderr, "WARNING: One or more operand regions are invalid (less than R_GLOBAL)\n");
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
 *  Writes out the intermediate code file for the program.
 *  The file is split into three regions:
 *      .string  (for string constants)
 *      .data    (for global variable declarations)
 *      .code    (for the TAC instructions)
 *
 *  The output file name is derived from the input filename by replacing
 *  its extension with ".ic". The file is written to the current directory.
 *  The output file name is printed to stdout when the file is opened.
 */
void write_ic_file(const char *input_filename, struct instr *code) {
    char output_filename[256];
    generate_output_filename(input_filename, output_filename, sizeof(output_filename));
    
    FILE *f = fopen(output_filename, "w");
    if (!f) {
        fprintf(stderr, "ERROR: Could not open output file %s for writing\n", output_filename);
        return;
    }
    
    fprintf(stderr, "DEBUG: Writing intermediate code to file %s\n", output_filename);
    printf("Intermediate code will be written to %s\n", output_filename);
    
    fprintf(f, ".string\n");
    fprintf(f, "/* no string constants */\n\n");
    
    fprintf(f, ".data\n");
    fprintf(f, "/* global variable declarations */\n\n");
    
    fprintf(f, ".code\n");

    /* Debug: Check if code list is valid */
    if (code == NULL) {
        fprintf(stderr, "DEBUG: write_ic_file: TAC code list is NULL!\n");
    } else {
        fprintf(stderr, "DEBUG: write_ic_file: TAC code list exists; first instruction opcode=%d\n", code->opcode);
    }

    struct instr *curr = code;
    int instrCount = 0;
    while (curr != NULL) {
        fprintf(stderr, "DEBUG: Writing instruction %d at address %p\n", instrCount, (void *)curr);
        output_instruction(f, curr);
        curr = curr->next;
        instrCount++;
    }
    
    fprintf(stderr, "DEBUG: Finished writing %d instructions to %s\n", instrCount, output_filename);
    fclose(f);
}

/* 
* generate_code
*
* Recursively traverses the AST and computes two synthesized attributes for each node:
*   - t->place : A struct addr giving the memory location of the computed value.
*   - t->code  : A linked list of TAC instructions which compute that value.
*
* Supported constructs (for this implementation):
*   - Literals (IntegerLiteral, RealLiteral, StringLiteral)
*   - Identifier (assumed that t->place has already been resolved during semantic analysis)
*   - Binary arithmetic operators: addition ("add"), subtraction ("sub"),
*     multiplication ("mul") and division ("div")
*   - Assignment ("assignment")
*   - Function calls ("functionCall")
*
* Additional node types should follow a similar pattern.
*/
void generate_code(struct tree *t) {
    if (t == NULL) {
        fprintf(stderr, "DEBUG: generate_code called with t == NULL\n");
        return;
    }
    
    fprintf(stderr, "DEBUG: Entering generate_code: Node id=%d, symbol='%s', nkids=%d\n",
            t->id, (t->symbolname ? t->symbolname : "NULL"), t->nkids);
    
    /* Special Case: Built-in Type Nodes
       These nodes (such as "Int", "Double", etc.) are used in type annotations.
       They don't produce runtime code, but we need to assign them a valid dummy place.
    */
    if (t->symbolname != NULL &&
       (strcmp(t->symbolname, "Int") == 0 ||
        strcmp(t->symbolname, "Double") == 0 ||
        strcmp(t->symbolname, "Boolean") == 0 ||
        strcmp(t->symbolname, "String") == 0)) {
         t->code = NULL;
         t->place = NULL_ADDR;
         fprintf(stderr, "DEBUG: Built-in type node id=%d ('%s') assigned dummy place (region=%d, offset=%d)\n",
                 t->id, t->symbolname, t->place.region, t->place.u.offset);
         return;
    }
    
    /* Special Case: "type" Nodes */
    if (t->symbolname != NULL && strcmp(t->symbolname, "type") == 0) {
         t->code = NULL;
         t->place = NULL_ADDR;
         fprintf(stderr, "DEBUG: Type node id=%d assigned dummy place (region=%d, offset=%d)\n",
                 t->id, t->place.region, t->place.u.offset);
         return;
    }
    
    /* --- Case 1: Leaf Nodes --- */
    if (t->leaf != NULL) {
        fprintf(stderr, "DEBUG: Processing leaf node: category=%d, text='%s'\n",
                t->leaf->category, (t->leaf->text ? t->leaf->text : "NULL"));
        if (t->leaf->category == IntegerLiteral) {
            t->place = new_temp();
            fprintf(stderr, "DEBUG: new_temp() returned region=%d, offset=%d\n",
                    t->place.region, t->place.u.offset);
            struct addr imm;
            imm.region = R_IMMED;
            imm.u.offset = t->leaf->value.ival;
            t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
            fprintf(stderr, "DEBUG: Leaf (IntegerLiteral) generated TAC with dest at %s:%d\n",
                    regionname(t->place.region), t->place.u.offset);
        }
        else if (t->leaf->category == RealLiteral) {
            t->place = new_temp();
            struct addr imm;
            imm.region = R_IMMED;
            imm.u.offset = (int)t->leaf->value.dval;
            t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
            fprintf(stderr, "DEBUG: Leaf (RealLiteral) generated TAC with dest at %s:%d\n",
                    regionname(t->place.region), t->place.u.offset);
        }
        else if (t->leaf->category == StringLiteral) {
            t->place = new_temp();
            struct addr imm;
            imm.region = R_IMMED;
            imm.u.offset = 0;  /* Placeholder for string constant index */
            t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
            fprintf(stderr, "DEBUG: Leaf (StringLiteral) generated TAC with dest at %s:%d\n",
                    regionname(t->place.region), t->place.u.offset);
        }
        else if (t->leaf->category == Identifier) {
            /* If the identifier's text is a built-in type name, assign a dummy place. */
            if (strcmp(t->leaf->text, "Int") == 0 ||
                strcmp(t->leaf->text, "Double") == 0 ||
                strcmp(t->leaf->text, "Boolean") == 0 ||
                strcmp(t->leaf->text, "String") == 0) {
                t->place = NULL_ADDR;
                fprintf(stderr, "DEBUG: Leaf (built-in type '%s') assigned dummy place: region=%d, offset=%d\n",
                        t->leaf->text, t->place.region, t->place.u.offset);
            } else {
                /* For non-type identifiers, assume the symbol table already set t->place.
                   If not set, assign a dummy place. */
                if (t->place.region == 0) {
                    t->place = NULL_ADDR;
                    fprintf(stderr, "WARNING: Identifier '%s' had no pre-assigned place; assigning dummy place\n",
                            t->leaf->text);
                }
                fprintf(stderr, "DEBUG: Leaf (Identifier) using pre-assigned place at %s:%d\n",
                        regionname(t->place.region), t->place.u.offset);
            }
            t->code = NULL;
            return;
        }
        return;
    }
    
    /* --- Special Case: variableDeclaration Nodes --- */
    if (t->symbolname != NULL && strcmp(t->symbolname, "variableDeclaration") == 0 && t->nkids == 3) {
        /* Assume children: [0] Identifier, [1] type, [2] initializer */
        if (t->kids[2] != NULL)
            generate_code(t->kids[2]);  /* generate code for initializer */
        /* Use the pre-assigned address from the identifier (child 0) */
        t->place = t->kids[0]->place;
        if (t->kids[2] != NULL && t->kids[2]->place.region != 0) {
            struct instr *assign_inst = gen(O_ASN, t->place, t->kids[2]->place, NULL_ADDR);
            t->code = concat(t->kids[2]->code, assign_inst);
        } else {
            t->code = t->kids[2] ? t->kids[2]->code : NULL;
        }
        fprintf(stderr, "DEBUG: VariableDeclaration node id=%d; using child's place: region=%d, offset=%d\n",
                t->id, t->place.region, t->place.u.offset);
        return;
    }
    
    /* --- Process Internal Nodes: Recursively generate code for all children --- */
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] == NULL) {
            fprintf(stderr, "ERROR: Child %d of node id=%d is NULL\n", i, t->id);
            continue;
        }
        fprintf(stderr, "DEBUG: Generating code for child %d of node id=%d\n", i, t->id);
        generate_code(t->kids[i]);
    }
    
    /* Accumulate children code */
    struct instr *children_code = NULL;
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] == NULL) {
            fprintf(stderr, "ERROR: Cannot concatenate code, child %d of node id=%d is NULL\n", i, t->id);
            continue;
        }
        if (t->kids[i]->code == NULL) {
            fprintf(stderr, "WARNING: Child %d of node id=%d has no code\n", i, t->id);
        }
        children_code = concat(children_code, t->kids[i]->code);
    }
    fprintf(stderr, "DEBUG: Concatenated children code for node id=%d\n", t->id);
    
    /* --- Binary Arithmetic Operators --- */
    if (strcmp(t->symbolname, "add") == 0 && t->nkids >= 2) {
        t->place = new_temp();
        struct instr *arith = gen(O_IADD, t->place, t->kids[0]->place, t->kids[1]->place);
        t->code = concat(children_code, arith);
        fprintf(stderr, "DEBUG: Node id=%d computed add; result stored at %s:%d\n",
                t->id, regionname(t->place.region), t->place.u.offset);
        return;
    }
    else if (strcmp(t->symbolname, "sub") == 0 && t->nkids >= 2) {
        t->place = new_temp();
        struct instr *arith = gen(O_ISUB, t->place, t->kids[0]->place, t->kids[1]->place);
        t->code = concat(children_code, arith);
        fprintf(stderr, "DEBUG: Node id=%d computed sub; result stored at %s:%d\n",
                t->id, regionname(t->place.region), t->place.u.offset);
        return;
    }
    else if (strcmp(t->symbolname, "mul") == 0 && t->nkids >= 2) {
        t->place = new_temp();
        struct instr *arith = gen(O_IMUL, t->place, t->kids[0]->place, t->kids[1]->place);
        t->code = concat(children_code, arith);
        fprintf(stderr, "DEBUG: Node id=%d computed mul; result stored at %s:%d\n",
                t->id, regionname(t->place.region), t->place.u.offset);
        return;
    }
    else if (strcmp(t->symbolname, "div") == 0 && t->nkids >= 2) {
        t->place = new_temp();
        struct instr *arith = gen(O_IDIV, t->place, t->kids[0]->place, t->kids[1]->place);
        t->code = concat(children_code, arith);
        fprintf(stderr, "DEBUG: Node id=%d computed div; result stored at %s:%d\n",
                t->id, regionname(t->place.region), t->place.u.offset);
        return;
    }
    /* --- Assignment Operation --- */
    else if (strcmp(t->symbolname, "assignment") == 0) {
        /* If the assignment node has exactly one child (e.g., just a declaration)
           then propagate that child's code and place.
        */
        if (t->nkids == 1 && t->kids[0] != NULL) {
            generate_code(t->kids[0]);
            t->place = t->kids[0]->place;
            t->code = t->kids[0]->code;
            fprintf(stderr, "DEBUG: Assignment (single child) node id=%d; using child's place: region=%d, offset=%d\n",
                    t->id, t->place.region, t->place.u.offset);
            return;
        }
        else if (t->nkids >= 2 && t->kids[1] != NULL) {
            generate_code(t->kids[1]);  // Generate code for RHS.
            t->place = t->kids[0]->place;   // Use LHS location.
            struct instr *assign_inst = gen(O_ASN, t->place, t->kids[1]->place, NULL_ADDR);
            t->code = concat(children_code, assign_inst);
            fprintf(stderr, "DEBUG: Assignment node id=%d; value stored at %s:%d\n",
                    t->id, regionname(t->place.region), t->place.u.offset);
            return;
        }
    }
    /* --- Function Calls --- */
    else if (strcmp(t->symbolname, "functionCall") == 0 && t->nkids >= 1) {
        fprintf(stderr, "DEBUG: Processing functionCall node id=%d\n", t->id);
        int argCount = 0;
        struct instr *args_code = NULL;
        if (t->nkids >= 2 && t->kids[1] != NULL) {
            struct tree *argList = t->kids[1];
            for (int i = 0; i < argList->nkids; i++) {
                if (argList->kids[i] == NULL) {
                    fprintf(stderr, "ERROR: Argument %d of functionCall node id=%d is NULL\n", i, t->id);
                    continue;
                }
                generate_code(argList->kids[i]);
                argCount++;
                struct instr *parm_inst = gen(O_PARM, argList->kids[i]->place, NULL_ADDR, NULL_ADDR);
                args_code = concat(args_code, parm_inst);
                fprintf(stderr, "DEBUG: Processed argument %d for functionCall node id=%d\n", i, t->id);
            }
        }
        t->place = new_temp();  // Temporary for return value.
        struct addr arg_count;
        arg_count.region = R_IMMED;
        arg_count.u.offset = argCount;
        struct instr *call_inst = gen(O_CALL, t->place, t->kids[0]->place, arg_count);
        t->code = concat(concat(children_code, args_code), call_inst);
        fprintf(stderr, "DEBUG: Function call node id=%d; return value stored at %s:%d, arg count=%d\n",
                t->id, regionname(t->place.region), t->place.u.offset, argCount);
        return;
    }
    
    /* --- Fallback --- */
    if (t->nkids == 1 && t->kids[0] != NULL) {
        generate_code(t->kids[0]);
        t->place = t->kids[0]->place;
        fprintf(stderr, "DEBUG: Fallback for node id=%d; propagating child's place: region=%d, offset=%d\n",
                t->id, t->place.region, t->place.u.offset);
    } else {
        fprintf(stderr, "DEBUG: Fallback for node id=%d; no single child place to propagate\n", t->id);
    }
    t->code = children_code;
}
