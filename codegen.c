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
#define DEBUG_OUTPUT 0  // Set to 1 to enable debug output, 0 to disable

/* External function prototypes from tac.c */
extern struct addr new_temp(void);
extern struct instr *gen(int op, struct addr a1, struct addr a2, struct addr a3);
extern struct instr *concat(struct instr *l1, struct instr *l2);

/* External functions from tac.c */
extern char *regionname(int i);
extern char *opcodename(int i);

/* Safe debugging function that only outputs if DEBUG_OUTPUT is enabled */
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
 *  Given an input filename (e.g. "foo.kt"), produce an output filename (e.g. "foo.ic").
 */
static void generate_output_filename(const char *input_filename, char *output_filename, size_t size) {
    if (!input_filename || !output_filename || size == 0) {
        fprintf(stderr, "ERROR: Invalid parameters to generate_output_filename\n");
        return;
    }
    
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
    if (!f) return;
    
    /* Check for valid region value before accessing regionname */
    if (a.region < R_NONE) {
        fprintf(f, "invalid:%d", a.u.offset);
    } else {
        fprintf(f, "%s:%d", regionname(a.region), a.u.offset);
    }
}

/*
 * output_instruction
 *  Writes a single TAC instruction to the file.
 *  The format is: <opcode> <dest>, <src1>, <src2>
 */
static void output_instruction(FILE *f, struct instr *instr) {
    if (!f || !instr) {
        debug_print("DEBUG: output_instruction called with NULL parameter\n");
        return;
    }
    
    /* Print details about the instruction for debugging */
    debug_print("DEBUG: output_instruction: opcode=%d, dest.region=%d, src1.region=%d, src2.region=%d\n",
            instr->opcode, instr->dest.region, instr->src1.region, instr->src2.region);

    /* Check if opcode is within valid range */
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

    /* Debug: Check if code list is valid */
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
        debug_print("DEBUG: generate_code called with t == NULL\n");
        return;
    }
    
    debug_print("DEBUG: Entering generate_code: Node id=%d, symbol='%s', nkids=%d\n",
            t->id, (t->symbolname ? t->symbolname : "NULL"), t->nkids);
    
    /* Initialize place to NULL_ADDR for all nodes by default */
    t->place = NULL_ADDR;
    t->code = NULL;

    /* Special Case: Built-in Type Nodes
       These nodes (such as "Int", "Double", etc.) are used in type annotations.
       They don't produce runtime code, but we need to assign them a valid dummy place.
    */
    if (t->symbolname != NULL &&
       (strcmp(t->symbolname, "Int") == 0 ||
        strcmp(t->symbolname, "Double") == 0 ||
        strcmp(t->symbolname, "Boolean") == 0 ||
        strcmp(t->symbolname, "String") == 0 ||
        strcmp(t->symbolname, "type") == 0)) {
         t->code = NULL;
         t->place = NULL_ADDR;
         debug_print("DEBUG: Built-in type node id=%d ('%s') assigned dummy place\n",
                 t->id, t->symbolname);
         return;
    }
    
    /* --- Case 1: Leaf Nodes --- */
    if (t->leaf != NULL) {
        debug_print("DEBUG: Processing leaf node: category=%d, text='%s'\n",
                t->leaf->category, (t->leaf->text ? t->leaf->text : "NULL"));
                
        if (t->leaf->category == IntegerLiteral) {
            t->place = new_temp();
            debug_print("DEBUG: new_temp() returned region=%d, offset=%d\n",
                    t->place.region, t->place.u.offset);
            struct addr imm;
            imm.region = R_IMMED;
            imm.u.offset = t->leaf->value.ival;
            t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
            debug_print("DEBUG: Leaf (IntegerLiteral) generated TAC with dest at %s:%d\n",
                    regionname(t->place.region), t->place.u.offset);
        }
        else if (t->leaf->category == RealLiteral) {
            t->place = new_temp();
            struct addr imm;
            imm.region = R_IMMED;
            imm.u.offset = (int)t->leaf->value.dval;
            t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
            debug_print("DEBUG: Leaf (RealLiteral) generated TAC with dest at %s:%d\n",
                    regionname(t->place.region), t->place.u.offset);
        }
        else if (t->leaf->category == StringLiteral) {
            t->place = new_temp();
            struct addr imm;
            imm.region = R_IMMED;
            imm.u.offset = 0;  /* Placeholder for string constant index */
            t->code = gen(O_ASN, t->place, imm, NULL_ADDR);
            debug_print("DEBUG: Leaf (StringLiteral) generated TAC with dest at %s:%d\n",
                    regionname(t->place.region), t->place.u.offset);
        }
        else if (t->leaf->category == Identifier) {
            /* If the identifier's text is a built-in type name, assign a dummy place. */
            if (t->leaf->text && (
                strcmp(t->leaf->text, "Int") == 0 ||
                strcmp(t->leaf->text, "Double") == 0 ||
                strcmp(t->leaf->text, "Boolean") == 0 ||
                strcmp(t->leaf->text, "String") == 0)) {
                t->place = NULL_ADDR;
                debug_print("DEBUG: Leaf (built-in type '%s') assigned dummy place\n", t->leaf->text);
            } else {
                /* For non-type identifiers, if place is not set, use a new temporary */
                if (t->place.region == R_NONE) {
                    /* Here we'd ideally look up the identifier in the symbol table.
                       For now, we'll just create a new temporary as a fallback. */
                    t->place = new_temp();
                    debug_print("WARNING: Identifier '%s' had no pre-assigned place; creating temp: %s:%d\n",
                            (t->leaf->text ? t->leaf->text : "unknown"), 
                            regionname(t->place.region), t->place.u.offset);
                } else {
                    debug_print("DEBUG: Leaf (Identifier) using pre-assigned place at %s:%d\n",
                            regionname(t->place.region), t->place.u.offset);
                }
            }
            /* No code generated for a simple identifier reference */
            return;
        }
        return;
    }
    
    /* --- Special Case: variableDeclaration Nodes --- */
    if (t->symbolname != NULL && strcmp(t->symbolname, "variableDeclaration") == 0 && t->nkids >= 2) {
        /* Process children first */
        for (int i = 0; i < t->nkids; i++) {
            if (t->kids[i] != NULL) {
                generate_code(t->kids[i]);
            }
        }
        
        /* Assume children: [0] Identifier, [1] type, [2] initializer (optional) */
        /* Use the pre-assigned address from the identifier (child 0) */
        if (t->kids[0] != NULL) {
            t->place = t->kids[0]->place;
        } else {
            t->place = new_temp(); /* Fallback if no identifier */
        }
        
        /* If there's an initializer, generate assignment */
        if (t->nkids > 2 && t->kids[2] != NULL && t->kids[2]->place.region != R_NONE) {
            struct instr *assign_inst = gen(O_ASN, t->place, t->kids[2]->place, NULL_ADDR);
            t->code = concat(t->kids[2]->code, assign_inst);
        } else if (t->nkids > 2 && t->kids[2] != NULL) {
            t->code = t->kids[2]->code;
        }
        
        debug_print("DEBUG: VariableDeclaration node id=%d; place: region=%d, offset=%d\n",
                t->id, t->place.region, t->place.u.offset);
        return;
    }
    
    /* --- Process Internal Nodes: Recursively generate code for all children --- */
    struct instr *children_code = NULL;
    
    for (int i = 0; i < t->nkids; i++) {
        if (t->kids[i] == NULL) {
            debug_print("WARNING: Child %d of node id=%d is NULL\n", i, t->id);
            continue;
        }
        debug_print("DEBUG: Generating code for child %d of node id=%d\n", i, t->id);
        generate_code(t->kids[i]);
        
        /* Concatenate code safely */
        if (t->kids[i]->code != NULL) {
            children_code = concat(children_code, t->kids[i]->code);
        }
    }
    
    debug_print("DEBUG: Concatenated children code for node id=%d\n", t->id);
    
    /* --- Binary Arithmetic Operators --- */
    if (t->symbolname != NULL) {
        if (strcmp(t->symbolname, "add") == 0 && t->nkids >= 2) {
            /* Check if both operands exist */
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                debug_print("ERROR: Missing operands for add operator\n");
                t->code = children_code;
                return;
            }
            
            t->place = new_temp();
            struct instr *arith = gen(O_IADD, t->place, t->kids[0]->place, t->kids[1]->place);
            t->code = concat(children_code, arith);
            debug_print("DEBUG: Node id=%d computed add; result stored at %s:%d\n",
                    t->id, regionname(t->place.region), t->place.u.offset);
            return;
        }
        else if (strcmp(t->symbolname, "sub") == 0 && t->nkids >= 2) {
            /* Check if both operands exist */
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                debug_print("ERROR: Missing operands for sub operator\n");
                t->code = children_code;
                return;
            }
            
            t->place = new_temp();
            struct instr *arith = gen(O_ISUB, t->place, t->kids[0]->place, t->kids[1]->place);
            t->code = concat(children_code, arith);
            debug_print("DEBUG: Node id=%d computed sub; result stored at %s:%d\n",
                    t->id, regionname(t->place.region), t->place.u.offset);
            return;
        }
        else if (strcmp(t->symbolname, "mul") == 0 && t->nkids >= 2) {
            /* Check if both operands exist */
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                debug_print("ERROR: Missing operands for mul operator\n");
                t->code = children_code;
                return;
            }
            
            t->place = new_temp();
            struct instr *arith = gen(O_IMUL, t->place, t->kids[0]->place, t->kids[1]->place);
            t->code = concat(children_code, arith);
            debug_print("DEBUG: Node id=%d computed mul; result stored at %s:%d\n",
                    t->id, regionname(t->place.region), t->place.u.offset);
            return;
        }
        else if (strcmp(t->symbolname, "div") == 0 && t->nkids >= 2) {
            /* Check if both operands exist */
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                debug_print("ERROR: Missing operands for div operator\n");
                t->code = children_code;
                return;
            }
            
            t->place = new_temp();
            struct instr *arith = gen(O_IDIV, t->place, t->kids[0]->place, t->kids[1]->place);
            t->code = concat(children_code, arith);
            debug_print("DEBUG: Node id=%d computed div; result stored at %s:%d\n",
                    t->id, regionname(t->place.region), t->place.u.offset);
            return;
        }
        /* --- Assignment Operation --- */
        else if (strcmp(t->symbolname, "assignment") == 0) {
            /* If the assignment node has exactly one child (e.g., just a declaration)
               then propagate that child's code and place.
            */
            if (t->nkids == 1 && t->kids[0] != NULL) {
                t->place = t->kids[0]->place;
                t->code = t->kids[0]->code;
                debug_print("DEBUG: Assignment (single child) node id=%d; using child's place: region=%d, offset=%d\n",
                        t->id, t->place.region, t->place.u.offset);
                return;
            }
            else if (t->nkids >= 2 && t->kids[0] != NULL && t->kids[1] != NULL) {
                /* Ensure both LHS and RHS have valid places */
                if (t->kids[0]->place.region == R_NONE) {
                    debug_print("WARNING: LHS of assignment has invalid place, creating temp\n");
                    t->kids[0]->place = new_temp();
                }
                
                if (t->kids[1]->place.region == R_NONE) {
                    debug_print("WARNING: RHS of assignment has invalid place, creating temp\n");
                    t->kids[1]->place = new_temp();
                }
                
                t->place = t->kids[0]->place;   // Use LHS location.
                struct instr *assign_inst = gen(O_ASN, t->place, t->kids[1]->place, NULL_ADDR);
                t->code = concat(children_code, assign_inst);
                debug_print("DEBUG: Assignment node id=%d; value stored at %s:%d\n",
                        t->id, regionname(t->place.region), t->place.u.offset);
                return;
            }
        }
        /* --- Function Calls --- */
        else if (strcmp(t->symbolname, "functionCall") == 0) {
            debug_print("DEBUG: Processing functionCall node id=%d\n", t->id);
            
            /* Check if function name exists */
            if (t->nkids < 1 || t->kids[0] == NULL) {
                debug_print("ERROR: Function call missing function name\n");
                t->code = children_code;
                return;
            }
            
            int argCount = 0;
            struct instr *args_code = NULL;
            
            /* Process arguments if they exist */
            if (t->nkids >= 2 && t->kids[1] != NULL) {
                struct tree *argList = t->kids[1];
                
                /* Count arguments and generate parameter passing code */
                for (int i = 0; i < argList->nkids; i++) {
                    if (argList->kids[i] == NULL) {
                        debug_print("ERROR: Argument %d of functionCall node id=%d is NULL\n", i, t->id);
                        continue;
                    }
                    
                    /* Ensure argument has a valid place */
                    if (argList->kids[i]->place.region == R_NONE) {
                        debug_print("WARNING: Argument %d has invalid place, skipping\n", i);
                        continue;
                    }
                    
                    argCount++;
                    struct instr *parm_inst = gen(O_PARM, argList->kids[i]->place, NULL_ADDR, NULL_ADDR);
                    args_code = concat(args_code, parm_inst);
                    debug_print("DEBUG: Processed argument %d for functionCall node id=%d\n", i, t->id);
                }
            }
            
            /* Create temporary for return value */
            t->place = new_temp();
            
            /* Ensure function name has a valid place */
            if (t->kids[0]->place.region == R_NONE) {
                debug_print("WARNING: Function name has invalid place, creating temp\n");
                t->kids[0]->place = new_temp();
            }
            
            /* Create immediate for argument count */
            struct addr arg_count;
            arg_count.region = R_IMMED;
            arg_count.u.offset = argCount;
            
            /* Generate call instruction */
            struct instr *call_inst = gen(O_CALL, t->place, t->kids[0]->place, arg_count);
            
            /* Combine all code: children code + argument setup + function call */
            t->code = concat(concat(children_code, args_code), call_inst);
            
            debug_print("DEBUG: Function call node id=%d; return value stored at %s:%d, arg count=%d\n",
                    t->id, regionname(t->place.region), t->place.u.offset, argCount);
            return;
        }
        /* --- Comparision Operators --- */
        else if (strcmp(t->symbolname, "comparison") == 0 && t->nkids >= 2) {
            /* Check if both operands exist */
            if (t->kids[0] == NULL || t->kids[1] == NULL) {
                debug_print("ERROR: Missing operands for comparison operator\n");
                t->code = children_code;
                return;
            }
            
            t->place = new_temp();
            /* Determine the specific comparison opcode based on context or a third token child */
            int op = O_IEQ; /* Default to equality comparison, should be refined based on actual operator */
            
            /* Additional token child would typically specify the comparison operator kind */
            if (t->nkids > 2 && t->kids[2] != NULL && t->kids[2]->leaf != NULL) {
                if (strcmp(t->kids[2]->leaf->text, "<") == 0) op = O_ILT;
                else if (strcmp(t->kids[2]->leaf->text, "<=") == 0) op = O_ILE;
                else if (strcmp(t->kids[2]->leaf->text, ">") == 0) op = O_IGT;
                else if (strcmp(t->kids[2]->leaf->text, ">=") == 0) op = O_IGE;
                else if (strcmp(t->kids[2]->leaf->text, "==") == 0) op = O_IEQ;
                else if (strcmp(t->kids[2]->leaf->text, "!=") == 0) op = O_INE;
            }
            
            struct instr *comp = gen(op, t->place, t->kids[0]->place, t->kids[1]->place);
            t->code = concat(children_code, comp);
            debug_print("DEBUG: Node id=%d computed comparison; result stored at %s:%d\n",
                    t->id, regionname(t->place.region), t->place.u.offset);
            return;
        }
    }
    
    /* --- Fallback --- */
    /* If node has a single child with a valid place, propagate that place */
    if (t->nkids == 1 && t->kids[0] != NULL && t->kids[0]->place.region != R_NONE) {
        t->place = t->kids[0]->place;
        debug_print("DEBUG: Fallback for node id=%d; propagating child's place: region=%d, offset=%d\n",
                t->id, t->place.region, t->place.u.offset);
    } else {
        /* If we get here and place is still not set, create a new temp */
        if (t->place.region == R_NONE && children_code != NULL) {
            t->place = new_temp();
            debug_print("DEBUG: Fallback for node id=%d; creating new temp: region=%d, offset=%d\n",
                    t->id, t->place.region, t->place.u.offset);
        }
    }
    
    /* Always set the code field, even if it's just the children's code */
    t->code = children_code;
}