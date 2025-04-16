/*
 * Three Address Code - skeleton for CSE 423
 */
#ifndef TAC_H
#define TAC_H

struct addr {
  int region;
  union {
    int offset;
    char *name;
  } u;
};

typedef struct tac_list {
    struct instr *head;
    struct instr *tail;
} tac_list;

/* Regions: */
/* Regions for 3-address code intermediate representation */
#define R_GLOBAL 2001   /* For global variables and string constants */
#define R_CLASS  2002   /* For class-related symbols (if needed) */
#define R_LABEL  2003   /* For jump labels in the code region */
#define R_CONST  2004   /* For global constants */
#define R_NAME   2005   /* For symbolic names, if used */
#define R_NONE   2006   /* Represents an invalid or unused region */

/* New regions required for the assignment: */
#define R_STRUCT 2007   /* For struct members */
#define R_PARAM  2008   /* For function parameters */
#define R_LOCAL  2009   /* For local variables and temporary values */
#define R_IMMED  2010   /* For immediate values used directly in instructions */


struct instr {
   int opcode;
   struct addr dest, src1, src2;
   struct instr *next;
};
/* Opcodes, per lecture notes */
#define O_ADD   3001
#define O_SUB   3002
#define O_MUL   3003
#define O_DIV   3004
#define O_NEG   3005
#define O_ASN   3006
#define O_ADDR  3007
#define O_LCONT 3008
#define O_SCONT 3009
#define O_GOTO  3010
#define O_BLT   3011
#define O_BLE   3012
#define O_BGT   3013
#define O_BGE   3014
#define O_BEQ   3015
#define O_BNE   3016
#define O_BIF   3017
#define O_BNIF  3018
#define O_PARM  3019
#define O_CALL  3020
#define O_RET   3021
#define O_IADD  3022   /* Integer addition */
#define O_DADD  3023   /* Double (or floating-point) addition */
#define O_ISUB  3024   /* Integer subtraction */
#define O_DSUB  3025   /* Double subtraction */

#define O_IMUL  3026   /* Integer multiplication */
#define O_DMUL  3027   /* Double multiplication */

#define O_IDIV  3028   /* Integer division */
#define O_DDIV  3029   /* Double division */

#define O_IEQ 3030
#define O_ILT 3031
#define O_ILE 3032
#define O_IGT 3033
#define O_IGE 3034
#define O_INE 3035
/* declarations/pseudo instructions */
#define D_GLOB  3051
#define D_PROC  3052
#define D_LOCAL 3053
#define D_LABEL 3054
#define D_END   3055
#define D_PROT  3056 /* prototype "declaration" */

struct instr *gen(int, struct addr, struct addr, struct addr);
struct instr *concat(struct instr *, struct instr *);
struct instr *append(struct instr *l1, struct instr *l2);  /* <-- Added prototype */
char *regionname(int i);
char *opcodename(int i);
char *pseudoname(int i);
struct addr *genlabel();
// Create a new tac_list with a single instruction.
tac_list *new_tac_list(struct instr *instr);

// Concatenates two tac_lists, returning a new tac_list.
tac_list *concat_tac_lists(tac_list *list1, tac_list *list2);
void free_tac_list(tac_list *list);

#endif
