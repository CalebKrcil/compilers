struct token {
    int category;
    char *text;
    int lineno;
    char *filename;
    int ival;
    double dval;
    char *sval;
};

struct tree {
   int prodrule;
   char *symbolname;
   int nkids;
   struct tree *kids[10]; /* if nkids >0 */
   struct token *leaf;   /* if nkids == 0; NULL for ε productions */
};

struct tree* alctree(int prodrule, const char* symbolname, int nkids, ...);
struct tree* alcleaf(int category, const char* text, int lineno);
void printnode(struct tree* node);
void tree_print(struct tree* root);