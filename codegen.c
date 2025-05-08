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

typedef struct { char label[32]; double val; } RealEntry;
static RealEntry *dbltab   = NULL;
static int       dblcount = 0;
 
static void add_string_literal(const char *label, const char *text) {
    strtab[strcount].label = strdup(label);
    strtab[strcount].text  = strdup(text);
    strcount++;
}

struct addr empty_addr() {
    struct addr a;
    a.region = R_NONE;
    a.u.offset = 0;
    return a;
}



static int add_real_literal(const char *label, double v) {
    dbltab = realloc(dbltab, (dblcount+1)*sizeof *dbltab);
    strcpy(dbltab[dblcount].label, label);
    dbltab[dblcount].val   = v;
    return dblcount++;
}

static void flattenExprList(struct tree *elist, struct tree ***outArgs, int *outCount) {
    if (!elist) return;
    if (elist->symbolname && (strcmp(elist->symbolname, "expressionList") == 0 || strcmp(elist->symbolname, "valueArgumentList") == 0)) {
        for (int i = 0; i < elist->nkids; i++) {
            flattenExprList(elist->kids[i], outArgs, outCount);
        }
    } else {
        *outArgs = realloc(*outArgs,
        sizeof(struct tree*) * (*outCount + 1));
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
    debug_print("ASM EMIT  %s   is_double=%d   dest=%s:%d   src1=%s:%d\n",
        opcodename(instr->opcode),
        instr->is_double,
        regionname(instr->dest.region), instr->dest.u.offset,
        regionname(instr->src1.region), instr->src1.u.offset);

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

    if (t->symbolname && strcmp(t->symbolname, "postIncrement") == 0 && t->nkids == 1) {
        struct tree *varNode = t->kids[0];
        generate_code(varNode);
        struct addr oldval = new_temp();
        struct instr *code = gen(O_ASN, oldval, varNode->place, NULL_ADDR);
        struct addr one = { .region = R_IMMED, .u.offset = 1 };
        code = concat(code, gen(O_IADD, varNode->place, varNode->place, one));
        t->code  = concat(varNode->code, code);
        t->place = oldval;
        return;
    }
    
    if (t->symbolname && strcmp(t->symbolname, "postDecrement") == 0 && t->nkids == 1) {
        struct tree *varNode = t->kids[0];
        generate_code(varNode);
        struct addr oldval = new_temp();
        struct instr *code = gen(O_ASN, oldval, varNode->place, NULL_ADDR);
        struct addr one = { .region = R_IMMED, .u.offset = 1 };
        code = concat(code, gen(O_ISUB, varNode->place, varNode->place, one));
        t->code = concat(varNode->code, code);
        t->place = oldval;
        return;
    }

    // collapse any single-child node, propagating type too
    if (!t->leaf && t->nkids == 1) {
        generate_code(t->kids[0]);
        t->place = t->kids[0]->place;
        t->code  = t->kids[0]->code;
        t->type  = t->kids[0]->type;
        return;
    }

    if (t->leaf) {
        debug_print("Leaf node: category=%d, text=%s\n", 
                   t->leaf->category,
                   t->leaf->text ? t->leaf->text : "null");
    } else {
        debug_print("Interior node with %d kids\n", t->nkids);
    }

    if (t->leaf) {
        switch (t->leaf->category) {
            case IntegerLiteral: {
                t->type  = integer_typeptr;
                t->place = new_temp();
                struct addr imm = { .region = R_IMMED,
                                    .u.offset = t->leaf->value.ival };
                t->code  = gen(O_ASN, t->place, imm, NULL_ADDR);
                debug_print("LIT int %d -> %s:%d\n",
                            t->leaf->value.ival,
                            regionname(t->place.region),
                            t->place.u.offset);
                return;
            }
            case RealLiteral: {
                // Real literals are always 8‐byte doubles
                t->type = double_typeptr;
            
                // 1) give it a label .D<id> and record it
                char lbl[32];
                snprintf(lbl, sizeof(lbl), "D%d", dblcount);
                int id = add_real_literal(lbl, t->leaf->value.dval);
            
                // 2) allocate an 8‐byte temp slot
                t->place = new_temp();
            
                // 3) load the constant into xmm0 then store it into the temp
                struct instr *lit = gen(
                    O_LCONT,
                    t->place,
                    (struct addr){ .region = R_IMMED, .u.offset = id },
                    NULL_ADDR
                );
                // mark this load as a double so downstream stores use movsd
                lit->is_double = 1;
                debug_print("CODEGEN RealLiteral: place=%s:%d  is_double=%d\n",
                    regionname(t->place.region), t->place.u.offset,
                    lit->is_double);
            
                t->code = lit;
                debug_print(
                  "LIT real %f → .D%d @ %s:%d\n",
                   t->leaf->value.dval,
                   id,
                   regionname(t->place.region),
                   t->place.u.offset
                );
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
                t->type = string_typeptr;
                add_string_literal(label, t->leaf->text);
                stringLabelCounter++;
                return;
            }
            
            case BooleanLiteral: {
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
                SymbolTableEntry e = lookup_symbol(currentFunctionSymtab, t->leaf->text);
                if (e) {
                    t->place = e->location;
                    t->type  = e->type;
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

    if (t->symbolname) {

        if (strcmp(t->symbolname, "arrayAssignmentDeclaration")==0) {
            struct tree *varId    = t->kids[0];
            struct tree *typeNode = t->kids[1];
            struct tree *initTree = t->kids[4];
            struct tree *sizeExpr = initTree->kids[0];
            struct tree *initExpr = initTree->kids[1];
        
            generate_code(sizeExpr);
            struct addr bytesPer   = { .region = R_IMMED, .u.offset = 4 };
            struct addr totalBytes = new_temp();
            struct instr *code     = gen(O_IMUL,
                                         totalBytes,
                                         sizeExpr->place,
                                         bytesPer);
            code = concat(sizeExpr->code, code);
        
            struct addr basePtr = new_temp();
            struct instr *m = gen(O_MALLOC,
                                  basePtr,
                                  totalBytes,
                                  NULL_ADDR);
            m->is_ptr = 1;
            code = concat(code, m);
        
            SymbolTableEntry entry =
                lookup_symbol(currentFunctionSymtab, varId->leaf->text);
            struct instr *copyPtr = gen(O_ASN,
                                        entry->location,
                                        basePtr,
                                        NULL_ADDR);
            copyPtr->is_ptr = 1;
            code = concat(code, copyPtr);
        
            struct addr idx = new_temp();
            code = concat(code,
                          gen(O_ASN,
                              idx,
                              (struct addr){ .region=R_IMMED, .u.offset=0 },
                              NULL_ADDR));
        
            struct addr *lblTop  = genlabel();
            struct addr *lblExit = genlabel();
            code = concat(code,
                          gen(D_LABEL,
                              *lblTop,
                              NULL_ADDR,
                              NULL_ADDR));
        
            struct addr cmp = new_temp();
            code = concat(code, gen(O_IGE, cmp, idx, sizeExpr->place));
            code = concat(code, gen(O_BNZ, *lblExit, cmp, NULL_ADDR));
        
            struct addr off = new_temp();
            code = concat(code,
                          gen(O_IMUL, off, idx, bytesPer));
            struct addr eltAddr = new_temp();
            {
              struct instr *addPtr = gen(O_IADD,
                                         eltAddr,
                                         entry->location,
                                         off);
              addPtr->is_ptr = 1;
              code = concat(code, addPtr);
            }
        
            generate_code(initExpr);
            code = concat(code, initExpr->code);
            code = concat(code,
                          gen(O_ASN,
                              (struct addr){ .region=R_MEM,
                                             .u.offset=eltAddr.u.offset },
                              initExpr->place,
                              NULL_ADDR));
        
            code = concat(code,
                          gen(O_IADD,
                              idx,
                              idx,
                              (struct addr){ .region=R_IMMED, .u.offset=1 }));
            code = concat(code,
                          gen(O_BR, *lblTop, NULL_ADDR, NULL_ADDR));
        
            code = concat(code,
                          gen(D_LABEL, *lblExit, NULL_ADDR, NULL_ADDR));
        
            t->place = entry->location;
            t->code  = code;
            t->type  = typeNode->type;
            return;
        }
        

        if (strcmp(t->symbolname, "arrayAccess")==0) {
            struct tree *arr = t->kids[0];
            struct tree *idx = t->kids[1];
        
            generate_code(arr);
            struct addr ptrVal = new_temp();
            struct instr *ldPtr = gen(O_ASN,
                                     ptrVal,
                                     arr->place,
                                     NULL_ADDR);
            ldPtr->is_ptr = 1;
            struct instr *code = concat(arr->code, ldPtr);
        
            generate_code(idx);
            code = concat(code, idx->code);
        
            t->type = arr->type->u.a.elemtype;
            struct addr bytesPer = { .region = R_IMMED, .u.offset = 4 };
        
            struct addr offset = new_temp();
            code = concat(code,
                          gen(O_IMUL, offset, idx->place, bytesPer));
        
            struct addr addrTemp = new_temp();
            struct instr *addPtr = gen(O_IADD,
                                       addrTemp,
                                       ptrVal,
                                       offset);
            addPtr->is_ptr = 1;
            code = concat(code, addPtr);
        
            t->place = new_temp();
            struct instr *loadElem = gen(O_ASN,
                                        t->place,
                                        (struct addr){ .region=R_MEM,
                                                       .u.offset=addrTemp.u.offset },
                                        NULL_ADDR);
            code = concat(code, loadElem);
        
            t->code = code;
            return;
        }

        if (strcmp(t->symbolname, "arrayDeclaration")==0) {
            struct tree *varId    = t->kids[0];
            struct tree *typeNode = t->kids[1];
            struct tree *initTree = t->kids[2];
            struct tree *sizeExpr = initTree->kids[0];
        
            generate_code(sizeExpr);
            struct addr bytesPer   = { .region = R_IMMED, .u.offset = 4 };
            struct addr totalBytes = new_temp();
            struct instr *code     = gen(O_IMUL,
                                         totalBytes,
                                         sizeExpr->place,
                                         bytesPer);
            code = concat(sizeExpr->code, code);
        
            struct addr basePtr = new_temp();
            struct instr *m = gen(O_MALLOC,
                                  basePtr,
                                  totalBytes,
                                  NULL_ADDR);
            m->is_ptr = 1;
            code = concat(code, m);
        
            SymbolTableEntry entry =
                lookup_symbol(currentFunctionSymtab, varId->leaf->text);
            struct instr *copyPtr = gen(O_ASN,
                                        entry->location,
                                        basePtr,
                                        NULL_ADDR);
            copyPtr->is_ptr = 1;
            code = concat(code, copyPtr);
        
            struct addr idx = new_temp();
            code = concat(code,
                          gen(O_ASN,
                              idx,
                              (struct addr){ .region=R_IMMED, .u.offset=0 },
                              NULL_ADDR));
        
            struct addr *lblTop  = genlabel();
            struct addr *lblExit = genlabel();
            code = concat(code,
                          gen(D_LABEL,
                              *lblTop,
                              NULL_ADDR,
                              NULL_ADDR));
        
            struct addr cmp = new_temp();
            code = concat(code, gen(O_IGE, cmp, idx, sizeExpr->place));
            code = concat(code, gen(O_BNZ, *lblExit, cmp, NULL_ADDR));
        
            struct addr off = new_temp();
            code = concat(code,
                          gen(O_IMUL, off, idx, bytesPer));
            struct addr eltAddr = new_temp();
            {
              struct instr *addPtr = gen(O_IADD,
                                         eltAddr,
                                         entry->location,
                                         off);
              addPtr->is_ptr = 1;
              code = concat(code, addPtr);
            }
        
            code = concat(code,
                          gen(O_ASN,
                              (struct addr){ .region=R_MEM,
                                             .u.offset=eltAddr.u.offset },
                              (struct addr){ .region=R_IMMED, .u.offset=0 },
                              NULL_ADDR));
        
            code = concat(code,
                          gen(O_IADD,
                              idx,
                              idx,
                              (struct addr){ .region=R_IMMED, .u.offset=1 }));
            code = concat(code,
                          gen(O_BR, *lblTop, NULL_ADDR, NULL_ADDR));
        
            code = concat(code,
                          gen(D_LABEL, *lblExit, NULL_ADDR, NULL_ADDR));
        
            t->place = entry->location;
            t->code  = code;
            t->type  = typeNode->type;
            return;
        }
        
        if (strcmp(t->symbolname, "assignment")==0 && t->kids[0] && strcmp(t->kids[0]->symbolname, "arrayAccess")==0)
        {
            struct tree *access = t->kids[0];
            struct tree *arr    = access->kids[0];
            struct tree *idx    = access->kids[1];
            struct tree *rhs    = t->kids[1];

            generate_code(arr);
            struct addr ptrVal = new_temp();
            struct instr *ldPtr = gen(
                O_ASN,
                ptrVal,
                arr->place,
                NULL_ADDR);
            ldPtr->is_ptr = 1;
            struct instr *code = concat(arr->code, ldPtr);

            generate_code(idx);
            code = concat(code, idx->code);
            generate_code(rhs);

            t->type = rhs->type;
            struct addr bytesPer = { .region = R_IMMED, .u.offset = 4 };

            struct addr offset = new_temp();
            code = concat(code,
                gen(O_IMUL, offset, idx->place, bytesPer));

            struct addr addrTemp = new_temp();
            struct instr *addPtr = gen(
                O_IADD,
                addrTemp,
                ptrVal,
                offset);
            addPtr->is_ptr = 1;
            code = concat(code, addPtr);

            struct instr *storeElem = gen(
                O_ASN,
                (struct addr){ .region=R_MEM,
                            .u.offset=addrTemp.u.offset },
                rhs->place,
                NULL_ADDR);
            code = concat(code, storeElem);

            t->code  = concat(concat(arr->code, rhs->code), code);
            t->place = rhs->place;
            t->type  = rhs->type;
            return;
        }

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
            t->type  = rhs->type;
            struct instr *asn = gen(O_ASN,
                            lhs->place,
                            rhs->place,
                            NULL_ADDR);
            SymbolTableEntry entry = lookup_symbol(currentFunctionSymtab, lhs->leaf->text);
            asn->is_double = (entry && entry->type == double_typeptr) ? 1 : 0;
            asn->is_ptr = (entry && entry->type == string_typeptr) ? 1 : 0;
            debug_print("CODEGEN ASN to '%s': dest=%s:%d  src=%s:%d  is_double=%d\n",
                lhs->leaf->text,
                regionname(asn->dest.region), asn->dest.u.offset,
                regionname(asn->src1.region), asn->src1.u.offset,
                asn->is_double);
            t->code = concat(rhs->code, asn);

            debug_print("DEBUG: Generated assignment: %s:%d = %s:%d (is_double=%d)\n",
                regionname(lhs->place.region), lhs->place.u.offset,
                regionname(rhs->place.region), rhs->place.u.offset,
                (int)asn->is_double);
            return;
        }

        else if (strcmp(t->symbolname, "additive_expression")==0 && t->nkids==2) {
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);
            int isD = (t->kids[0]->type==double_typeptr
                     || t->kids[1]->type==double_typeptr);
            t->type = isD ? double_typeptr : integer_typeptr;
        
            int opcode = isD
                       ? (t->prodrule==ADD ? O_DADD : O_DSUB)
                       : (t->prodrule==ADD ? O_IADD : O_ISUB);
        
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

        else if (strcmp(t->symbolname, "multiplicative_expression") == 0 && t->nkids == 2) {
            generate_code(t->kids[0]);
            generate_code(t->kids[1]);

            int isD = (t->kids[0]->type == double_typeptr
                    || t->kids[1]->type == double_typeptr);
            t->type  = isD ? double_typeptr : integer_typeptr;

            int opcode;
            if (isD) {
                if      (t->prodrule == MULT) opcode = O_DMUL;
                else if (t->prodrule == DIV ) opcode = O_DDIV;
                else                          opcode = O_DMOD;
            } else {
                if      (t->prodrule == MULT) opcode = O_IMUL;
                else if (t->prodrule == DIV ) opcode = O_IDIV;
                else                          opcode = O_IMOD;
            }

            t->place = new_temp();

            t->code = concat(
                concat(t->kids[0]->code, t->kids[1]->code),
                gen(opcode,
                    t->place,
                    t->kids[0]->place,
                    t->kids[1]->place)
            );
            return;
        }
        


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
            t->type  = boolean_typeptr;
            return;
        }

        else if (strcmp(t->symbolname, "negation")==0 && t->nkids>=1) {
            generate_code(t->kids[0]);
            t->place = new_temp();
            t->code  = gen(O_NEG,
                           t->place,
                           t->kids[0]->place,
                           NULL_ADDR);
            return;
        }

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
            t->type  = boolean_typeptr;
            return;
        }       

        else if (strcmp(t->symbolname, "functionCall") == 0) {
            struct tree *fnNode = t->kids[0];
            const char *methodName = fnNode->leaf->text;
        
            struct instr *code = NULL;
            struct tree **args = NULL;
            int argc = 0;
            for (int i = 1; i < t->nkids; i++)
                flattenExprList(t->kids[i], &args, &argc);
        
            if (strcmp(methodName, "java.lang.Math.abs") == 0 && argc == 1) {
                generate_code(args[0]);
                t->place = new_temp();
                t->code = concat(args[0]->code, gen(O_ABS, t->place, args[0]->place, NULL_ADDR));
                t->type = double_typeptr;
                free(args);
                return;
            } else if (strcmp(methodName, "java.lang.Math.max") == 0 && argc == 2) {
                generate_code(args[0]); generate_code(args[1]);
                t->place = new_temp();
                t->code = concat(concat(args[0]->code, args[1]->code),
                                 gen(O_MAX, t->place, args[0]->place, args[1]->place));
                t->type = double_typeptr;
                free(args);
                return;
            } else if (strcmp(methodName, "java.lang.Math.min") == 0 && argc == 2) {
                generate_code(args[0]); generate_code(args[1]);
                t->place = new_temp();
                t->code = concat(concat(args[0]->code, args[1]->code),
                                 gen(O_MIN, t->place, args[0]->place, args[1]->place));
                t->type = double_typeptr;
                free(args);
                return;
            } else if (strcmp(methodName, "java.lang.Math.pow") == 0 && argc == 2) {
                generate_code(args[0]); generate_code(args[1]);
                t->place = new_temp();
                t->code = concat(concat(args[0]->code, args[1]->code),
                                 gen(O_POW, t->place, args[0]->place, args[1]->place));
                t->type = double_typeptr;
                free(args);
                return;
            } else if (strcmp(methodName, "java.lang.Math.cos") == 0 && argc == 1) {
                generate_code(args[0]);
                t->place = new_temp();
                t->code = concat(args[0]->code, gen(O_COS, t->place, args[0]->place, NULL_ADDR));
                t->type = double_typeptr;
                free(args);
                return;
            } else if (strcmp(methodName, "java.lang.Math.sin") == 0 && argc == 1) {
                generate_code(args[0]);
                t->place = new_temp();
                t->code = concat(args[0]->code, gen(O_SIN, t->place, args[0]->place, NULL_ADDR));
                t->type = double_typeptr;
                free(args);
                return;
            } else if (strcmp(methodName, "java.lang.Math.tan") == 0 && argc == 1) {
                generate_code(args[0]);
                t->place = new_temp();
                t->code = concat(args[0]->code, gen(O_TAN, t->place, args[0]->place, NULL_ADDR));
                t->type = double_typeptr;
                free(args);
                return;
            } else if (strcmp(fnNode->leaf->text, "java.util.Random.nextInt") == 0) {
                static int seeded = 0;
                struct instr *code = NULL;
            
                if (!seeded) {
                    struct instr *srand_tac = gen(O_SRAND, empty_addr(), empty_addr(), empty_addr());
                    code = append(code, srand_tac);
                    seeded = 1;
                }
            
                t->place = new_temp();
                struct instr *rand_tac = gen(O_RAND, t->place, empty_addr(), empty_addr());
                code = append(code, rand_tac);
            
                t->code = append(t->code, code);
                return;
            }
            
            SymbolTableEntry fentry =
                lookup_symbol(globalSymtab, (char *)methodName);
            if (!fentry)
                fentry = lookup_symbol(currentFunctionSymtab, (char *)methodName);
        
            for (int i = 0; i < argc; i++) {
                generate_code(args[i]);
                code = concat(code, args[i]->code);
                struct instr *p = gen(O_PARM, NULL_ADDR, args[i]->place, NULL_ADDR);
                p->is_double = (args[i]->type == double_typeptr);
                p->is_ptr    = (args[i]->type == string_typeptr);
                code = concat(code, p);
            }
            free(args);
        
            struct addr nameAddr = {
                .region = R_NAME,
                .u.name = strdup(fentry->s)
            };
        
            int returnsValue = fentry &&
                               fentry->type &&
                               fentry->type->u.f.returntype != null_typeptr;
        
            if (returnsValue) {
                t->place = new_temp();
                struct instr *callInstr = gen(O_CALL, t->place, nameAddr, NULL_ADDR);
                callInstr->is_double = (fentry->type->u.f.returntype == double_typeptr);
                code = concat(code, callInstr);
            } else {
                t->place = (struct addr){ R_NONE, { .offset = 0 } };
                code = concat(code, gen(O_CALL, NULL_ADDR, nameAddr, NULL_ADDR));
            }
        
            t->code = code;
            return;
        }   

        else if (strcmp(t->symbolname, "functionDeclaration") == 0) {
            SymbolTable oldSymtab = currentFunctionSymtab;
            char *funcName = t->kids[0]->leaf->text;
            struct addr label_addr = *genlabel();
        
            SymbolTableEntry fentry = lookup_symbol(globalSymtab, funcName);
            if (fentry) fentry->location = label_addr;
        
            struct addr name_addr = { .region = R_NAME, .u.name = strdup(funcName) };
            t->code = gen(D_GLOB,  name_addr, NULL_ADDR, NULL_ADDR);
            t->code = concat(t->code,
                             gen(D_PROC, label_addr, name_addr, NULL_ADDR));
        
            if (t->scope) currentFunctionSymtab = t->scope;
        
            struct instr *allocInstr = gen(
                O_ALLOC,
                NULL_ADDR,
                (struct addr){ .region = R_IMMED,
                               .u.offset = currentFunctionSymtab->nextOffset },
                NULL_ADDR
            );
            t->code = concat(t->code, allocInstr);
        
            {
                struct tree **params = NULL;
                int paramCount = 0;
                flattenParameterList(t->kids[1], &params, &paramCount);
                for (int i = 0; i < paramCount; i++) {
                    char *pname = params[i]->kids[0]->leaf->text;
                    SymbolTableEntry pe = lookup_symbol(currentFunctionSymtab, pname);
                    if (!pe) continue;
                    struct addr preg = { .region = R_PARAM, .u.offset = i };
                    struct instr *parmCopy = gen(O_ASN, pe->location, preg, NULL_ADDR);
                    parmCopy->is_double = (pe->type == double_typeptr);
                    parmCopy->is_ptr    = (pe->type == string_typeptr);
                    t->code = concat(t->code, parmCopy);
                }
                free(params);
            }
        
            struct tree *body = NULL;
            for (int i = 0; i < t->nkids; i++) {
                if (t->kids[i] && strcmp(t->kids[i]->symbolname, "block") == 0) {
                    body = t->kids[i];
                    break;
                }
            }
            if (body) {
                generate_code(body);
                t->code = concat(t->code, body->code);
            }
        
            int maxOffset = currentFunctionSymtab->nextOffset;
            for (struct instr *ip = t->code; ip; ip = ip->next) {
                if (ip->dest.region == R_LOCAL && ip->dest.u.offset > maxOffset)
                    maxOffset = ip->dest.u.offset;
                if (ip->src1.region == R_LOCAL && ip->src1.u.offset > maxOffset)
                    maxOffset = ip->src1.u.offset;
                if (ip->src2.region == R_LOCAL && ip->src2.u.offset > maxOffset)
                    maxOffset = ip->src2.u.offset;
            }
            int frameSize = ((maxOffset + 15) / 16) * 16;
        
            for (struct instr *ip = t->code; ip; ip = ip->next) {
                if (ip->opcode == O_ALLOC) {
                    ip->src1.u.offset = frameSize;
                    break;
                }
            }
        
            {
                struct instr *last = t->code;
                while (last && last->next) last = last->next;
                if (!last || last->opcode != O_RET) {
                    t->code = concat(t->code,
                                     gen(O_DEALLOC,
                                         NULL_ADDR,
                                         (struct addr){ .region = R_IMMED,
                                                        .u.offset = frameSize },
                                         NULL_ADDR));
                    t->code = concat(t->code,
                                     gen(O_RET, NULL_ADDR, NULL_ADDR, NULL_ADDR));
                }
            }
        
            t->code = concat(t->code,
                             gen(D_END, label_addr, name_addr, NULL_ADDR));
        
            currentFunctionSymtab = oldSymtab;
            return;
        }
        
        
    
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
                struct tree *idNode   = t->kids[0];
                struct tree *init     = t->kids[2];
            
                SymbolTableEntry entry =
                  lookup_symbol(currentFunctionSymtab, idNode->leaf->text);
                if (!entry) {
                    fprintf(stderr,
                      "ERROR: variableDeclaration \"%s\" not found\n",
                      idNode->leaf->text);
                    exit(1);
                }
                struct addr var_loc = entry->location;
            
                generate_code(init);
            
                struct instr *asn = gen(
                    O_ASN,
                    entry->location,
                    init->place,
                    NULL_ADDR
                );
                asn->is_double = (entry->type == double_typeptr) ? 1 : 0;
                asn->is_ptr = (entry->type == string_typeptr) ? 1 : 0;
                debug_print("CODEGEN varDecl '%s': dest=%s:%d  init_place=%s:%d  is_double=%d\n",
                    idNode->leaf->text,
                    regionname(asn->dest.region), asn->dest.u.offset,
                    regionname(init->place.region), init->place.u.offset,
                    asn->is_double);
            
                t->code  = concat(init->code, asn);
                t->place = var_loc;
                t->type  = init->type;
                return;
            }       
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

    fprintf(f, "\t.file\t\"%s\"\n", input_filename);
    fprintf(f, "\t.section\t.rodata\n\t.align\t8\n");
    for (int i = 0; i < strcount; i++) {
        fprintf(f,
                ".LC%d:\n"
                "\t.string\t%s\n",
                i, strtab[i].text);
    }
    for (int i = 0; i < dblcount; i++) {
        fprintf(f,
            ".%s:\n"
            "\t.double\t%f\n",
            dbltab[i].label,
            dbltab[i].val);
    }
    fprintf(f,
        ".LCdouble_fmt:\n"
        "\t.string \"%%f\\n\"\n");
    fprintf(f,
        ".LCint_fmt:\n"
        "\t.string \"%%d\\n\"\n");
    fprintf(f, "\t.text\n");

    const char *ireg[6] = { "%edi","%esi","%edx","%ecx","%r8d","%r9d" };
    const char *qreg[6] = { "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    const char *xmmreg[6] = {"%xmm0","%xmm1","%xmm2","%xmm3","%xmm4","%xmm5"};
    int argc = 0, args_off[6], args_is_ptr[6] = {0}, args_is_double[6] = {0};
    int args_region[6];

    int inFunction = 0;
    int frameSize  = 0;

    for (struct instr *cur = code; cur; cur = cur->next) {
        switch (cur->opcode) {

          // ————————— PSEUDO‐OPS —————————

            case D_GLOB:
                fprintf(f, "\t.globl\t%s\n",
                    cur->dest.u.name);
                break;

            case D_PROC:
                inFunction = 1;
                argc       = 0;
                frameSize  = 0;
            
                fprintf(f,
                    "\t.type\t%s, @function\n"
                    "%s:\n"
                    ".LFB%d:\n"
                    "\t.cfi_startproc\n"
                    "\tendbr64\n"
                    "\tpushq\t%%rbp\n"
                    "\t.cfi_def_cfa_offset 16\n"
                    "\t.cfi_offset 6, -16\n"
                    "\tmovq\t%%rsp, %%rbp\n"
                    "\t.cfi_def_cfa_register 6\n"
                    ,
                    cur->src1.u.name,
                    cur->src1.u.name,
                    cur->dest.u.offset
                );
                break;

            case D_LABEL:
                // basic‐block label inside current function
                fprintf(f, ".L%d:\n", cur->dest.u.offset);
                break;

            case D_END:
                fprintf(f,
                    "\taddq\t$%d, %%rsp\n"
                    "\tleave\n"
                    "\t.cfi_def_cfa   7, 8\n"
                    "\t.cfi_endproc\n"
                    "\tret\n"
                    ".LFE%d:\n"
                    "\t.size\t%s, .-%s\n",
                    frameSize,
                    cur->dest.u.offset,
                    cur->src1.u.name,
                    cur->src1.u.name
                );
                inFunction = 0;
                break;

          // —————— STACK ALLOCATION ——————

            case O_ALLOC:
                if (inFunction && frameSize == 0) {
                    frameSize = cur->src1.u.offset;
                    fprintf(f, "\tsubq\t$%d, %%rsp\n", frameSize);
                } else if (cur->src1.u.offset > 0) {
                    fprintf(f, "\tsubq\t$%d, %%rsp\n", cur->src1.u.offset);
                }
                break;

            case O_MALLOC: {
                if (cur->src1.region == R_IMMED) {
                    fprintf(f, "\tmovl\t$%d, %%edi\n", cur->src1.u.offset);
                } else {
                    fprintf(f, "\tmovl\t-%d(%%rbp), %%edi\n",
                            cur->src1.u.offset);
                }
                fprintf(f, "\tcall\tmalloc\n");
                fprintf(f, "\tmovq\t%%rax, -%d(%%rbp)\n",
                        cur->dest.u.offset);
                argc = 0;
                break;
            }

            case O_DEALLOC:
                if (!inFunction) {
                    fprintf(f, "\taddq\t$%d, %%rsp\n",
                            cur->src1.u.offset);
                }
                break;

            // ——————— PARAM / CALL ———————

            case O_PARM:
                if (argc < 6) {
                    args_off[argc]     = cur->src1.u.offset;
                    args_region[argc]  = cur->src1.region;
                    args_is_double[argc] = cur->is_double;
                    args_is_ptr[argc]    = (cur->src1.region == R_GLOBAL)
                                          || cur->is_ptr;
                    argc++;
                }
                break;

            case O_CALL: {
                if (cur->src1.region == R_NAME
                    && strcmp(cur->src1.u.name, "println") == 0
                    && argc == 1) {
                    if (args_is_ptr[0]) {
                        if (args_region[0] == R_GLOBAL) {
                            fprintf(f, "\tleaq\t.LC%d(%%rip), %%rdi\n",
                                    args_off[0]);
                        } else {
                            fprintf(f, "\tmovq\t-%d(%%rbp), %%rdi\n",
                                    args_off[0]);
                        }
                        fprintf(f, "\txor\t%%eax, %%eax\n");
                    }
                    else if (args_is_double[0]) {
                        fprintf(f, "\tleaq\t.LCdouble_fmt(%%rip), %%rdi\n");
                        fprintf(f, "\tmovsd\t-%d(%%rbp), %%xmm0\n", args_off[0]);
                        fprintf(f, "\tmovl\t$1, %%eax\n");
                    }
                    else {
                        fprintf(f, "\tleaq\t.LCint_fmt(%%rip), %%rdi\n");
                        fprintf(f, "\tmovl\t-%d(%%rbp), %%esi\n", args_off[0]);
                        fprintf(f, "\txor\t%%eax, %%eax\n");
                    }
                    fprintf(f, "\tcall\tprintf\n");
                    argc = 0;
                    break;
                }
                
                for (int i = 0; i < argc && i < 6; i++) {
                    if (args_is_double[i]) {
                        fprintf(f,
                            "\tmovsd\t-%d(%%rbp), %s\n",
                            args_off[i],
                            xmmreg[i]);
                    }
                    else if (args_is_ptr[i]) {
                        if (args_region[i] == R_GLOBAL) {
                            fprintf(f,
                                "\tleaq\t.LC%d(%%rip), %s\n",
                                args_off[i],
                                qreg[i]);
                        } else {
                            fprintf(f,
                                "\tmovq\t-%d(%%rbp), %s\n",
                                args_off[i],
                                qreg[i]);
                        }
                    }
                    else {
                        fprintf(f,
                            "\tmovl\t-%d(%%rbp), %s\n",
                            args_off[i],
                            ireg[i]);
                    }
                }
                if (cur->src1.region == R_NAME) {
                    fprintf(f, "\tcall\t%s\n", cur->src1.u.name);
                } else {
                    fprintf(f, "\tcall\t.L%d\n", cur->src1.u.offset);
                }
                if (cur->dest.u.offset > 0) {
                    if (cur->is_double) {
                        fprintf(f,
                            "\tmovsd\t%%xmm0, -%d(%%rbp)\n",
                            cur->dest.u.offset);
                    } else {
                        fprintf(f,
                            "\tmovl\t%%eax, -%d(%%rbp)\n",
                            cur->dest.u.offset);
                    }
                }
                argc = 0;
                break;
            }               

            case O_RET:
                if (cur->src1.region == R_IMMED) {
                    fprintf(f, "\tmovl\t$%d, %%eax\n",
                            cur->src1.u.offset);
                }
                else if (cur->src1.region != R_NONE) {
                    fprintf(f, "\tmovl\t-%d(%%rbp), %%eax\n",
                            cur->src1.u.offset);
                }
                break;

          // ———————— ARITHMETIC ————————

            case O_ASN:
                if (cur->dest.region == R_MEM) {
                    fprintf(f, "\tmovq\t-%d(%%rbp), %%rax\n",
                            cur->dest.u.offset);
                    if (cur->src1.region == R_IMMED) {
                        fprintf(f, "\tmovl\t$%d, (%%rax)\n",
                                cur->src1.u.offset);
                    }
                    else if (cur->src1.region == R_PARAM) {
                        fprintf(f, "\tmovl\t%s, (%%rax)\n",
                                ireg[cur->src1.u.offset]);
                    }
                    else {
                        fprintf(f,
                            "\tmovl\t-%d(%%rbp), %%ecx\n"
                            "\tmovl\t%%ecx,(%%rax)\n",
                            cur->src1.u.offset);
                    }
                    break;
                }
      
                if (cur->src1.region == R_MEM && cur->dest.region == R_LOCAL) {
                    fprintf(f,
                        "\tmovq\t-%d(%%rbp), %%rax\n"
                        "\tmovl\t(%%rax), %%eax\n"
                        "\tmovl\t%%eax, -%d(%%rbp)\n",
                        cur->src1.u.offset,
                        cur->dest.u.offset);
                    break;
                }
                    if (cur->is_double) {
                        if (cur->src1.region == R_PARAM) {
                            fprintf(f,
                                "\tmovsd\t%s, %d(%%rbp)\n",
                                xmmreg[cur->src1.u.offset],
                                -cur->dest.u.offset);
                        } else {
                            fprintf(f,
                                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                                -cur->src1.u.offset,
                                -cur->dest.u.offset);
                        }
                    } else if (cur->is_ptr) {
                        if (cur->src1.region == R_GLOBAL) {
                            fprintf(f,
                                "\tleaq\t.LC%d(%%rip), %%rax\n"
                                "\tmovq\t%%rax, %d(%%rbp)\n",
                                cur->src1.u.offset,
                                -cur->dest.u.offset);
                        } else if (cur->src1.region == R_PARAM) {
                            static const char *pr[] = {"%rdi","%rsi","%rdx","%rcx","%r8","%r9"};
                            fprintf(f,
                                "\tmovq\t%s, %d(%%rbp)\n",
                                pr[cur->src1.u.offset],
                                -cur->dest.u.offset);
                        } else {
                            fprintf(f,
                                "\tmovq\t%d(%%rbp), %%rax\n"
                                "\tmovq\t%%rax, %d(%%rbp)\n",
                                -cur->src1.u.offset,
                                -cur->dest.u.offset);
                        }
                    } else {
                        if (cur->src1.region == R_IMMED) {
                            fprintf(f,
                                "\tmovl\t$%d, %d(%%rbp)\n",
                                cur->src1.u.offset,
                                -cur->dest.u.offset);
                        } else if (cur->src1.region == R_PARAM) {
                            static const char *p[6] = {"%edi","%esi","%edx","%ecx","%r8d","%r9d"};
                            fprintf(f,
                                "\tmovl\t%s, %d(%%rbp)\n",
                                p[cur->src1.u.offset],
                                -cur->dest.u.offset);
                        } else {
                            fprintf(f,
                                "\tmovl\t%d(%%rbp), %%eax\n"
                                "\tmovl\t%%eax, %d(%%rbp)\n",
                                -cur->src1.u.offset,
                                -cur->dest.u.offset);
                        }
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
                    "\tmovsd\t.D%d(%%rip), %%xmm0\n"
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
                if (cur->is_ptr) {
                    fprintf(f,
                        "\tmovq\t-%d(%%rbp), %%rax\n"
                        "\tmovslq\t-%d(%%rbp), %%rcx\n"
                        "\taddq\t%%rcx, %%rax\n"
                        "\tmovq\t%%rax, -%d(%%rbp)\n",
                        cur->src1.u.offset,
                        cur->src2.u.offset,
                        cur->dest.u.offset
                    );
                }
                else if (cur->src2.region == R_IMMED) {
                    fprintf(f,
                        "\tmovl\t%d(%%rbp), %%eax\n"
                        "\taddl\t$%d, %%eax\n"
                        "\tmovl\t%%eax, %d(%%rbp)\n",
                        -cur->src1.u.offset,
                        cur->src2.u.offset,
                        -cur->dest.u.offset
                    );
                } else {
                    fprintf(f,
                        "\tmovl\t%d(%%rbp), %%eax\n"
                        "\taddl\t%d(%%rbp), %%eax\n"
                        "\tmovl\t%%eax, %d(%%rbp)\n",
                        -cur->src1.u.offset,
                        -cur->src2.u.offset,
                        -cur->dest.u.offset
                    );
                }
                break;
        
        case O_ISUB:
            if (cur->src2.region == R_IMMED) {
                fprintf(f,
                    "\tmovl\t%d(%%rbp), %%eax\n"
                    "\tsubl\t$%d, %%eax\n"
                    "\tmovl\t%%eax, %d(%%rbp)\n",
                    -cur->src1.u.offset,
                     cur->src2.u.offset,
                    -cur->dest.u.offset
                );
            } else {
                fprintf(f,
                    "\tmovl\t%d(%%rbp), %%eax\n"
                    "\tsubl\t%d(%%rbp), %%eax\n"
                    "\tmovl\t%%eax, %d(%%rbp)\n",
                    -cur->src1.u.offset,
                    -cur->src2.u.offset,
                    -cur->dest.u.offset
                );
            }
            break;
        

            case O_IMUL:
                if (cur->src2.region == R_IMMED) {
                    fprintf(f,
                        "\tmovl\t%d(%%rbp), %%eax\n"
                        "\timull\t$%d, %%eax\n"
                        "\tmovl\t%%eax, %d(%%rbp)\n",
                        -cur->src1.u.offset,
                        cur->src2.u.offset,
                        -cur->dest.u.offset
                    );
                } else {
                    fprintf(f,
                        "\tmovl\t%d(%%rbp), %%eax\n"
                        "\timull\t%d(%%rbp), %%eax\n"
                        "\tmovl\t%%eax, %d(%%rbp)\n",
                        -cur->src1.u.offset,
                        -cur->src2.u.offset,
                        -cur->dest.u.offset
                    );
                }
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

          case O_LBL:
            fprintf(f, ".L%d:\n", cur->dest.u.offset);
            break;

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

          case O_POP:
            fprintf(f, "\tpopq\t%%rbp\n");
            break;
            case O_ABS:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tmovapd\t%%xmm0, %%xmm1\n"
                "\tmovabsq\t$0x7FFFFFFFFFFFFFFF, %%rax\n"
                "\tmovq\t%%rax, %%xmm2\n"
                "\tandpd\t%%xmm2, %%xmm1\n"
                "\tmovsd\t%%xmm1, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->dest.u.offset);
            break;
        
        case O_MAX:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%d(%%rbp), %%xmm1\n"
                "\tmaxsd\t%%xmm1, %%xmm0\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->src2.u.offset,
                -cur->dest.u.offset);
            break;
        
        case O_MIN:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%d(%%rbp), %%xmm1\n"
                "\tminsd\t%%xmm1, %%xmm0\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->src2.u.offset,
                -cur->dest.u.offset);
            break;
        
        case O_POW:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tmovsd\t%d(%%rbp), %%xmm1\n"
                "\tcall\tpow\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->src2.u.offset,
                -cur->dest.u.offset);
            break;
        
        case O_SIN:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tcall\tsin\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->dest.u.offset);
            break;
        
        case O_COS:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tcall\tcos\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->dest.u.offset);
            break;
        
        case O_TAN:
            fprintf(f,
                "\tmovsd\t%d(%%rbp), %%xmm0\n"
                "\tcall\ttan\n"
                "\tmovsd\t%%xmm0, %d(%%rbp)\n",
                -cur->src1.u.offset,
                -cur->dest.u.offset);
            break;
        
        case O_RAND:
            fprintf(f,
                "\tcall\trand\n"
                "\tmovl\t%%eax, %d(%%rbp)\n",
                -cur->dest.u.offset);
            break;
        
        case O_SRAND:
            fprintf(f,
                "\tmovl\t$0, %%edi\n"
                "\tcall\ttime\n"
                "\tmovq\t%%rax, %%rdi\n"
                "\tcall\tsrand\n");
            break;
        
          default:
            // any unhandled opcode
            fprintf(f, "\t# unhandled opcode %s\n",
                    opcodename(cur->opcode));
            break;
        }
    }
    fprintf(f, "\t.section .note.GNU-stack,\"\",@progbits\n");

    fclose(f);
}