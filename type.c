#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "symtab.h"
#include "k0gram.tab.h"

/* 
   Global built-in type objects.
   These are shared among all symbols that refer to these types.
*/
struct typeinfo null_type    = { NONE_TYPE };
struct typeinfo integer_type = { INT_TYPE };
struct typeinfo double_type  = { DOUBLE_TYPE };
struct typeinfo boolean_type = { BOOL_TYPE };
struct typeinfo string_type  = { STRING_TYPE };

/* Global shared pointers */
typeptr null_typeptr    = &null_type;
typeptr integer_typeptr = &integer_type;
typeptr double_typeptr  = &double_type;
typeptr boolean_typeptr = &boolean_type;
typeptr string_typeptr  = &string_type;

/* The names for each base type.
   Note: We assume that the first base type constant is NONE_TYPE (value 1000000),
   so we subtract 1000000 from t->basetype to index into this array.
*/
char *typenam[] = {
   "none",    // index 0
   "int",     // index 1
   "float",   // index 2
   "bool",    // index 3
   "string",  // index 4
   "array",   // index 5
   "func",    // index 6
   "class",   // index 7
   "package", // index 8
   "any"      // index 9
};

typeptr alctype(int base)
{
    /* For built-in types, return the shared pointer */
    if (base == NONE_TYPE)   return null_typeptr;
    else if (base == INT_TYPE)    return integer_typeptr;
    else if (base == DOUBLE_TYPE) return double_typeptr;
    else if (base == BOOL_TYPE)   return boolean_typeptr;
    else if (base == STRING_TYPE) return string_typeptr;
    
    /* For any other type, allocate a new typeinfo structure */
    typeptr rv = (typeptr) calloc(1, sizeof(struct typeinfo));
    if (rv == NULL) return rv;
    rv->basetype = base;
    return rv;
}

/* Optional initialization function.
   With static initialization, nothing further is needed.
   However, you could call this function at program startup.
*/
void init_base_types(void) {
    /* Already initialized statically. */
}

#if 0
/* For languages that have a list type. */
typeptr alclist()
{
   typeptr rv = alctype(LIST_TYPE);
   return rv;
}

/* For languages that have a struct type. */
typeptr alcstructtype()
{
   typeptr rv = alctype(STRUCT_TYPE);
   /* Who initializes the fields? */
   return rv;
}
#endif

/* Construct a function type from syntax (sub)trees.
   This routine should eventually walk the provided subtrees (r and p)
   to set the return type and parameter list. For now, it returns a basic function type.
*/
typeptr alcfunctype(struct tree * r, struct tree * p, SymbolTable st)
{
    typeptr rv = alctype(FUNC_TYPE);
    if (rv == NULL) return NULL;
    rv->u.f.st = st;
    /* TODO: Fill in return type and parameter list based on r and p */
    return rv;
}

/* Construct an array type from syntax (sub)trees.
   This routine should eventually use the provided subtrees to set the element type and size.
*/
typeptr alcarraytype(struct tree * s, struct tree * e)
{
    typeptr rv = alctype(ARRAY_TYPE);
    if (rv == NULL) return NULL;
    /* TODO: Fill in element type and size from s and e */
    return rv;
}

char *typename(typeptr t)
{
    if (!t) return "(NULL)";
    else if (t->basetype < FIRST_TYPE || t->basetype > LAST_TYPE)
        return "(BOGUS)";
    else return typenam[t->basetype - 1000000];
}
