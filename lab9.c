#include <stdio.h>
#include <stdlib.h>
#include "tac.h"

void print_addr(struct addr a) {
    if (a.region == R_NONE) {
        printf("<none>\t");
    } else if (a.region == R_CONST) {
        printf("const:%d\t", a.u.offset);
    } else if (a.region == R_NAME) {
        printf("name:%s\t", a.u.name);
    } else {
        printf("%s:%d\t", regionname(a.region), a.u.offset);
    }
}

void tacprint(struct instr *p) {
    while (p != NULL) {
        printf("%s\t", opcodename(p->opcode));

        print_addr(p->dest);
        print_addr(p->src1);
        print_addr(p->src2);

        printf("\n");
        p = p->next;
    }
}

int main(void) {
    struct instr *code = NULL;
    struct addr none = { R_NONE, .u.offset = 0 };

    /* 1. ASN(loc:0, const:5)  i = 5 */
    struct addr a_i = { R_LOCAL, .u.offset = 0 };
    struct addr a_const5 = { R_CONST, .u.offset = 5 };
    code = gen(O_ASN, a_i, a_const5, none);

    /* 2. MUL(loc:8, loc:0, loc:0)  t1 = i * i, result stored at loc:8 */
    struct addr a_tmp1 = { R_LOCAL, .u.offset = 8 };
    code = append(code, gen(O_MUL, a_tmp1, a_i, a_i));

    /* 3. ADD(loc:16, loc:8, const:1)  t2 = t1 + 1, result stored at loc:16 */
    struct addr a_tmp2 = { R_LOCAL, .u.offset = 16 };
    struct addr a_const1 = { R_CONST, .u.offset = 1 };
    code = append(code, gen(O_ADD, a_tmp2, a_tmp1, a_const1));

    /* 4. ASN(loc:0, loc:16)  i = t2 */
    code = append(code, gen(O_ASN, a_i, a_tmp2, none));

    /* 5. PARM(loc:0)  Push parameter: i */
    code = append(code, gen(O_PARM, a_i, none, none));

    /* 6. PARM(str:0)  Push parameter: string literal "Variable i is %d.\000" */
    struct addr a_str = { R_NAME, .u.name = "Variable i is %d.\000" };
    code = append(code, gen(O_PARM, a_str, none, none));

    /* 7. CALL(printf, 2, loc:24)  Call printf with two parameters; result stored in loc:24 */
    struct addr a_printf = { R_NAME, .u.name = "printf" };
    struct addr a_tmp3 = { R_LOCAL, .u.offset = 24 };
    code = append(code, gen(O_CALL, a_printf, a_tmp3, none));

    /* 8. RETURN  */
    code = append(code, gen(O_RET, none, none, none));

    tacprint(code);

    return 0;
}
