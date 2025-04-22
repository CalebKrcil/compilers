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


#define R_GLOBAL 2001   
#define R_CLASS  2002  
#define R_LABEL  2003  
#define R_CONST  2004   
#define R_NAME   2005   
#define R_NONE   2006   

#define R_STRUCT 2007   
#define R_PARAM  2008   
#define R_LOCAL  2009   
#define R_IMMED  2010   
#define R_FP     2011   
#define R_SP     2012   
#define R_MEM    2013   
#define R_RET    2014   


struct instr {
   int opcode;
   struct addr dest, src1, src2;
   struct instr *next;
};
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
#define O_IADD  3022   
#define O_DADD  3023   
#define O_ISUB  3024   
#define O_DSUB  3025   

#define O_IMUL  3026   
#define O_DMUL  3027   

#define O_IDIV  3028   
#define O_DDIV  3029   

#define O_IEQ 3030
#define O_ILT 3031
#define O_ILE 3032
#define O_IGT 3033
#define O_IGE 3034
#define O_INE 3035
#define O_LBL     3036   
#define O_BR      3037   
#define O_BZ      3038   
#define O_BNZ     3039   
#define O_NOT     3040   
#define O_PUSH    3041   
#define O_POP     3042   
#define O_ALLOC   3043   
#define O_DEALLOC 3044   
#define O_IMOD    3045
#define O_DMOD    4046
#define D_GLOB  3051
#define D_PROC  3052
#define D_LOCAL 3053
#define D_LABEL 3054
#define D_END   3055
#define D_PROT  3056 

struct instr *gen(int, struct addr, struct addr, struct addr);
struct instr *concat(struct instr *, struct instr *);
struct instr *append(struct instr *l1, struct instr *l2);  
char *regionname(int i);
char *opcodename(int i);
char *pseudoname(int i);
struct addr *genlabel();
tac_list *new_tac_list(struct instr *instr);

tac_list *concat_tac_lists(tac_list *list1, tac_list *list2);
void free_tac_list(tac_list *list);

#endif
