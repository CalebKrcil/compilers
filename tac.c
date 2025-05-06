/*
 * Three Address Code - skeleton for CS 423
 */
#include <stdio.h>
#include <stdlib.h>
#include "tac.h"
#include "symtab.h"

char *regionnames[] = {
    "global",  /* R_GLOBAL, 2001 */
    "class",   /* R_CLASS, 2002 */
    "label",   /* R_LABEL, 2003 */
    "const",   /* R_CONST, 2004 */
    "name",    /* R_NAME, 2005 */
    "none",    /* R_NONE, 2006 */
    "struct",  /* R_STRUCT, 2007 */
    "param",   /* R_PARAM, 2008 */
    "local",   /* R_LOCAL, 2009 */
    "immed",    /* R_IMMED, 2010 */
    "fp",
    "sp",
    "mem",
    "ret"
};
char *regionname(int i) { return regionnames[i-R_GLOBAL]; }
char *opcodenames[] = {
    "ADD","SUB", "MUL", "DIV", "NEG", "ASN", "ADDR", "LCONT", "SCONT", "GOTO",
    "BLT", "BLE", "BGT", "BGE", "BEQ", "BNE", "BIF", "BNIF", "PARM", "CALL",
    "RETURN", "IADD", "DADD", "ISUB", "DSUB", "IMUL", "DMUL", "IDIV", "DDIV",
    "IEQ", "ILT", "ILE", "IGT", "IGE", "INE", "LBL", "BR", "BZ", "BNZ", "NOT",
    "PUSH", "POP", "ALLOC", "DEALLOC", "MOD"
   };
char *opcodename(int i) { return opcodenames[i-O_ADD]; }
char *pseudonames[] = {
   "glob","proc", "loc", "lab", "end", "prot"
   };
char *pseudoname(int i) { return pseudonames[i-D_GLOB]; }

int labelcounter;

extern SymbolTable currentFunctionSymtab;
struct addr new_temp(void) {
    struct addr temp;
    temp.region = R_LOCAL;  
    temp.u.offset = currentFunctionSymtab->nextOffset;
    currentFunctionSymtab->nextOffset += 8;
    // fprintf(stderr,
    //     "DEBUG: new_temp() using local:%d  (nextOffset now %d)\n",
    //     temp.u.offset,
    //     currentFunctionSymtab->nextOffset);
    return temp;
}

struct addr *genlabel(void)
{
   struct addr *a = malloc(sizeof(struct addr));
   if (!a) {
       fprintf(stderr, "Memory allocation failed in genlabel\n");
       exit(4);
   }
   a->region = R_LABEL;
   a->u.offset = labelcounter++;
//    printf("generated a label %d\n", a->u.offset);
   return a;
}

struct instr *gen(int op, struct addr a1, struct addr a2, struct addr a3)
{
  struct instr *rv = malloc(sizeof (struct instr));
  if (rv == NULL) {
     fprintf(stderr, "out of memory\n");
     exit(4);
     }
  rv->opcode = op;
  rv->dest = a1;
  rv->src1 = a2;
  rv->src2 = a3;
  rv->next = NULL;
  rv->is_double = 0;
  rv->is_ptr = 0;
//   fprintf(stderr, "DEBUG: gen -> %s %s:%d, %s:%d, %s:%d\n",
//     opcodename(op),
//     regionname(a1.region), a1.u.offset,
//     regionname(a2.region), a2.u.offset,
//     regionname(a3.region), a3.u.offset);
  return rv;
}

struct instr *copylist(struct instr *l)
{
   if (l == NULL) return NULL;
   // gen() zeroes out is_double by default
   struct instr *lcopy = gen(l->opcode, l->dest, l->src1, l->src2);
   // *carry over* the double-width flag
   lcopy->is_double = l->is_double;
    lcopy->is_ptr = l->is_ptr;
   // copy the rest of the chain
   lcopy->next = copylist(l->next);
   return lcopy;
}

struct instr *append(struct instr *l1, struct instr *l2)
{
   if (l1 == NULL) return l2;
   struct instr *ltmp = l1;
   while(ltmp->next != NULL) ltmp = ltmp->next;
   ltmp->next = l2;
   return l1;
}

struct instr *concat(struct instr *l1, struct instr *l2)
{
    // fprintf(stderr, "DEBUG: concat lists %p and %p\n", l1, l2);
   return append(copylist(l1), l2);
}


tac_list *new_tac_list(struct instr *instr) {
    tac_list *list = malloc(sizeof(tac_list));
    if (!list) {
        fprintf(stderr, "Memory allocation failed for tac_list\n");
        exit(EXIT_FAILURE);
    }
    list->head = instr;
    list->tail = instr;
    if (instr != NULL) {
        instr->next = NULL;
    }
    return list;
}


tac_list *concat_tac_lists(tac_list *list1, tac_list *list2) {
    if (list1 == NULL)
        return list2;
    if (list2 == NULL)
        return list1;
    
    list1->tail->next = list2->head;
    list1->tail = list2->tail;
    
    free(list2);
    return list1;
}


void free_tac_list(tac_list *list) {
    if (list == NULL)
        return;
    
    struct instr *current = list->head;
    while (current != NULL) {
        struct instr *next_instr = current->next;
        free(current);
        current = next_instr;
    }
    free(list);
}
