#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "symtab.h"
#include "k0gram.tab.h"
   
struct typeinfo null_type = { NONE_TYPE };
struct typeinfo integer_type = { INT_TYPE };
struct typeinfo double_type = { DOUBLE_TYPE };
struct typeinfo boolean_type = { BOOL_TYPE };
struct typeinfo char_type = { CHAR_TYPE };

typeptr null_typeptr = &null_type;
typeptr integer_typeptr = &integer_type;
typeptr double_typeptr = &double_type;
typeptr boolean_typeptr = &boolean_type;
typeptr char_typeptr = &char_type;

char *typenam[] =
   {"none", "int", "float", "bool", "char",
    "string", "array", "func", "class", "package",
    "any"}; /* "list", "dict", ... */

typeptr alctype(int base)
{
   typeptr rv;
   if (base == NONE_TYPE) return null_typeptr;
   else if (base == INT_TYPE) return integer_typeptr;
   else if (base == DOUBLE_TYPE) return double_typeptr;
   else if (base == BOOL_TYPE) return boolean_typeptr;
   else if (base == CHAR_TYPE) return char_typeptr;

   rv = (typeptr) calloc(1, sizeof(struct typeinfo));
   if (rv == NULL) return rv;
   rv->basetype = base;
   return rv;
}

#if 0
/* For languages that have a list type.
 * List size determination from a tree nodeptr is sometimes reasonable?
 */
typeptr alclist()
{
   typeptr rv = alctype(LIST_TYPE);
   return rv;
}

/* For languages that have a struct type. */
typeptr alcstructtype()
{
   typeptr rv = alctype(STRUCT_TYPE);
   /* who initializes the fields, someone else I guess, later on */
   return rv;
}
#endif

/* Construct a function type from syntax (sub)tree(s).
 * For this to make any sense, you have to pass in the subtrees
 * for the return type (r) and the parameter list (p), but the calls to
 * to this function in the example are just passing NULL at present!
 */
typeptr alcfunctype(struct tree * r, struct tree * p, SymbolTable st)
{
   typeptr rv = alctype(FUNC_TYPE);
   if (rv == NULL) return NULL;
   rv->u.f.st = st;
   /* fill in return type and paramlist by traversing subtrees */
   /* rf->u.f.returntype = ... */
   return rv;
}

typeptr alcarraytype(struct tree * s, struct tree * e)
{
   typeptr rv = alctype(ARRAY_TYPE);
   if (rv == NULL) return NULL;
   /* fill in size and elemtype by traversing subtrees */
   /* rf->u.f.elemtype = ... */
   return rv;
}

char *typename(typeptr t)
{
   if (!t) return "(NULL)";
   else if (t->basetype < FIRST_TYPE || t->basetype > LAST_TYPE)
      return "(BOGUS)";
   else return typenam[t->basetype-1000000];
}