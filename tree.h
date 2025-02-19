struct tree {
   int prodrule;
   char *symbolname;
   int nkids;
   struct tree *kids[10]; /* if nkids >0 */
   struct token *leaf;   /* if nkids == 0; NULL for ε productions */
};
