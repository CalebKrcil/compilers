#ifndef TYPE_H
#define TYPE_H


struct tree;
struct sym_table;
typedef struct sym_table *SymbolTable;

typedef struct param {
   char *name;
   struct typeinfo *type;
   struct param *next;
   } *paramlist;

struct field {			/* members (fields) of structs */
   char *name;
   struct typeinfo *elemtype;
};

/* base types. How many more base types do we need? */
#define FIRST_TYPE   1000000
#define NONE_TYPE    1000000
#define INT_TYPE     1000001
#define DOUBLE_TYPE  1000002
#define BOOL_TYPE    1000003
#define STRING_TYPE  1000004
#define ARRAY_TYPE   1000005
#define FUNC_TYPE    1000006
#define CLASSTYPE    1000007
#define PACKAGE_TYPE 1000008
#define ANY_TYPE     1000009
#define LAST_TYPE    1000009

typedef struct typeinfo {
   int basetype;
   union {
    struct funcinfo {
	 char *name; /* ? */
	 int defined; /* 0 == prototype, 1 == not prototype */
	 struct sym_table *st;
	 struct typeinfo *returntype;
	 int nparams;
	 struct param *parameters;
	}f;
    struct arrayinfo {
        int size; /* -1 == unspecified/unknown/dontcare */
	    struct typeinfo *elemtype;
    }a;
  } u;
} *typeptr;

/* add constructors for each of the types if needed*/
typeptr alctype(int base);
typeptr alcfunctype(struct tree * r, struct tree * p, struct sym_table * st);
typeptr alcarraytype(struct tree * s, struct tree * e);
char *typename(typeptr t);

extern struct sym_table *global_table;
extern typeptr integer_typeptr;
extern typeptr double_typeptr;
extern typeptr null_typeptr;
extern typeptr string_typeptr;
extern typeptr boolean_typeptr;

extern char *typenam[];

void init_base_types(void);

#endif