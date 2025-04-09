%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "tree.h"
    #include "type.h"
    #include "symtab.h"
    #include "semantics.h"
    int yylex(void);
    void yyerror(const char *s);
    struct tree *root;
%}

%token <treeptr> RESERVED DOT COMMA LPAREN RPAREN LSQUARE RSQUARE
%token <treeptr> LCURL RCURL MULT MOD DIV ADD SUB INCR DECR
%token <treeptr> CONJ DISJ EXCL_WS EXCL_NO_WS COLON SEMICOLON ASSIGNMENT
%token <treeptr> ADD_ASSIGNMENT SUB_ASSIGNMENT MULT_ASSIGNMENT DIV_ASSIGNMENT
%token <treeptr> MOD_ASSIGNMENT ARROW DOUBLE_ARROW RANGE RANGE_UNTIL
%token <treeptr> COLONCOLON DOUBLE_SEMICOLON AT_NO_WS AT_POST_WS AT_PRE_WS
%token <treeptr> AT_BOTH_WS QUEST_WS QUEST_NO_WS LANGLE RANGLE LE GE
%token <treeptr> EXCL_EQ EXCL_EQEQ AS_SAFE EQEQ EQEQEQ SINGLE_QUOTE AMP
%token <treeptr> RETURN_AT CONTINUE_AT BREAK_AT THIS_AT SUPER_AT K0_FILE
%token <treeptr> FIELD PROPERTY GET SET RECIEVER PARAM SETPARAM
%token <treeptr> DELEGATE PACKAGE IMPORT CLASS INTERFACE FUN OBJECT
%token <treeptr> VAL VAR TYPE_ALIAS CONSTRUCTOR BY COMPANION INIT
%token <treeptr> THIS SUPER TYPEOF WHERE IF ELSE WHEN TRY CATCH FINALLY
%token <treeptr> FOR DO WHILE THROW RETURN CONTINUE BREAK AS IS IN
%token <treeptr> NOT_IS NOT_IN OUT DYNAMIC PUBLIC PRIVATE PROTECTED INTERNAL
%token <treeptr> ENUM SEALED ANNOTATION DATA INNER VALUE TAILREC
%token <treeptr> OPERATOR INLINE INFIX EXTERNAL SUSPEND OVERRIDE
%token <treeptr> ABSTRACT FINAL OPEN CONST LATEINIT VARARG NOINLINE
%token <treeptr> CROSSINLINE REIFIED EXPECT ACTUAL NL
%token <treeptr> IntegerLiteral RealLiteral FloatLiteral DoubleLiteral
%token <treeptr> BooleanLiteral NullLiteral StringLiteral Identifier
%token <treeptr> HexLiteral BinLiteral

%type <treeptr> program topLevelObject topLevelObjectList declaration globalVariableDeclaration
%type <treeptr> propertyDeclaration type functionDeclaration functionValueParameters
%type <treeptr> functionParameterList_opt functionValueParameter functionBody
%type <treeptr> block statements statement loopStatement forStatement whileStatement
%type <treeptr> doWhileStatement controlStructureBody assignment ifStatement
%type <treeptr> variableDeclaration multiVariableDeclaration variableDeclarationList
%type <treeptr> returnType_section expression additive_expression multiplicative_expression
%type <treeptr> primary_expression forInit forUpdate functionCall functionCallArguments 
%type <treeptr> unaryExpression boolExpression returnStatement typeAlias
%type <treeptr> expressionList breakStatement continueStatement disjunction conjunction
%type <treeptr> equality comparison logical_unary_expression arrayInitializer arrayAccess

%start program

%left ADD 
%left SUB
%left MULT 
%left DIV 
%left MOD
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE_IF
%right ASSIGNMENT  
%precedence FUNCTION_CALL_ARGS 
%precedence EXPR  
%precedence LOWER_THAN_FUNCTION_CALL_ARGS

%union {
   struct tree *treeptr;
}

%%

nl_opt:
      /* epsilon */
    | NL nl_opt
    | SEMICOLON nl_opt
    ;

program:
    nl_opt topLevelObjectList nl_opt { root = $2; }
    ;

topLevelObjectList:
    topLevelObject { $$ = $1; }
    | topLevelObjectList nl_opt topLevelObject { 
          $$ = alctree(101, "topLevelObjectList", 2, $1, $3); 
          $$->type = NULL; 
      }
    ;

topLevelObject:
    declaration { $$ = $1; }
    | globalVariableDeclaration { $$ = $1; }
    ;

declaration:
    functionDeclaration { $$ = $1; }
    | propertyDeclaration { $$ = $1; }
    | typeAlias { $$ = $1; }
    ;

globalVariableDeclaration:
    variableDeclaration {$$ =$1;}
    ;

propertyDeclaration:
    PROPERTY Identifier COLON type nl_opt { 
         $$ = alctree(PROPERTY, "propertyDeclaration", 2, $2, $4); 
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    ;

type:
    Identifier { 
         $$ = alctree(102, "type", 1, $1); 
         $$->is_nullable = 0; 
         $$->type = typeptr_name($1->leaf->text);
    }
    | Identifier QUEST_NO_WS { 
         $$ = alctree(103, "nullableType", 1, $1); 
         $$->is_nullable = 1;
         $$->type = typeptr_name($1->leaf->text);
    }
    | Identifier LANGLE type RANGLE { 
        $$ = alctree(104, "genericType", 2, $1, $3); 
        $$->is_nullable = 0;
        if (strcmp($1->leaf->text, "Array") == 0) {
            /* Create an array type using the element type from $3.
                You may use your alcarraytype function; here we assume
                that the size will be determined later from the array initializer. */
            $$->type = alcarraytype($3, NULL);
            if ($3->type)
                $$->type->u.a.elemtype = $3->type;
            else
                $$->type->u.a.elemtype = null_typeptr;
        } else {
            $$->type = typeptr_name($1->leaf->text);
        }
    }
    | Identifier LANGLE type RANGLE QUEST_NO_WS { 
         $$ = alctree(105, "nullableGenericType", 2, $1, $3); 
         $$->is_nullable = 1;
         $$->type = typeptr_name($1->leaf->text);
    }
    ;

functionDeclaration:
    FUN Identifier functionValueParameters returnType_section functionBody { 
         $$ = alctree(FUN, "functionDeclaration", 4, $2, $3, $4, $5); 
         $$->type = $4->type;
    }
    | FUN Identifier functionValueParameters functionBody { 
         $$ = alctree(FUN, "functionDeclaration", 3, $2, $3, $4); 
         $$->type = typeptr_name("Unit");
    }
    ;

functionValueParameters:
    LPAREN functionParameterList_opt RPAREN { 
         $$ = alctree(108, "functionValueParameters", 1, $2); 
         $$->type = NULL;
    }
    ;

functionParameterList_opt:
    /* epsilon */ { $$ = NULL; }
    | functionValueParameter { $$ = $1; }
    | functionParameterList_opt COMMA functionValueParameter { 
          $$ = alctree(109, "functionParameterList", 2, $1, $3); 
          $$->type = NULL;
    }
    ;

functionValueParameter:
    Identifier COLON type { 
         $$ = alctree(110, "functionValueParameter", 2, $1, $3); 
         $$->is_nullable = $3->is_nullable;
         $$->type = $3->type;
    }
    ;

functionBody:
    block { $$ = $1; }
    ;

block:
    LCURL nl_opt RCURL { $$ = NULL; }
    | LCURL statements nl_opt RCURL { 
         $$ = alctree(111, "block", 1, $2); 
         $$->type = NULL;
    }
    ;

statements:
    nl_opt statement nl_opt { $$ = $2; }
    | statements nl_opt statement { 
          $$ = alctree(112, "statements", 2, $1, $3); 
          $$->type = NULL;
      }
    ;

statement:
    expression nl_opt { $$ = $1; }
    | propertyDeclaration { $$ = $1; }
    | typeAlias { $$ = $1; }
    | assignment nl_opt { $$ = $1; }
    | loopStatement nl_opt { $$ = $1; }
    | ifStatement nl_opt { $$ = $1; }
    | returnStatement nl_opt { $$ = $1; }
    | breakStatement nl_opt { $$ = $1; }
    | continueStatement nl_opt { $$ = $1; }
    ;

breakStatement:
    BREAK { $$ = alctree(BREAK, "breakStatement", 0); $$->type = NULL; }
    ;

continueStatement:
    CONTINUE { $$ = alctree(CONTINUE, "continueStatement", 0); $$->type = NULL; }
    ;

loopStatement:
    forStatement { $$ = $1; }
    | whileStatement { $$ = $1; }
    | doWhileStatement { $$ = $1; }
    ;

forStatement:
    FOR LPAREN nl_opt forInit SEMICOLON boolExpression SEMICOLON forUpdate RPAREN controlStructureBody nl_opt
        { $$ = alctree(FOR, "forStatement", 4, $4, $6, $8, $10); $$->type = NULL; }
    | FOR LPAREN Identifier IN expression RANGE expression RPAREN controlStructureBody nl_opt
        { $$ = alctree(FOR, "forStatementKotlinRange", 4, $3, $5, $7, $9); $$->type = NULL; }
    | FOR LPAREN Identifier IN expression RANGE_UNTIL expression RPAREN controlStructureBody nl_opt
        { $$ = alctree(FOR, "forStatementKotlinRangeUntil", 4, $3, $5, $7, $9); $$->type = NULL; }
    ;

whileStatement:
    WHILE nl_opt LPAREN boolExpression RPAREN nl_opt controlStructureBody { $$ = alctree(WHILE, "whileStatement", 2, $4, $7); $$->type = NULL; }
    ;

doWhileStatement:
    DO nl_opt controlStructureBody nl_opt WHILE nl_opt LPAREN boolExpression RPAREN { $$ = alctree(DO, "doWhileStatement", 2, $3, $8); $$->type = NULL; }
    ;

controlStructureBody:
    block nl_opt { $$ = $1; }
    | statement nl_opt { $$ = $1; }
    ;

forInit:
    assignment { $$ = $1; }
    | /* epsilon */ { $$ = NULL; }
    ;

forUpdate:
    assignment { $$ = $1; }
    | /* epsilon */ { $$ = NULL; }
    ;

unaryExpression:
    INCR Identifier { $$ = alctree(INCR, "preIncrement", 1, $2); $$->type = integer_typeptr; }  
    | DECR Identifier { $$ = alctree(DECR, "preDecrement", 1, $2); $$->type = integer_typeptr; }  
    | Identifier INCR { $$ = alctree(INCR, "postIncrement", 1, $1); $$->type = integer_typeptr; }  
    | Identifier DECR { $$ = alctree(DECR, "postDecrement", 1, $1); $$->type = integer_typeptr; }
    ;

assignment:
    Identifier ASSIGNMENT expression { 
         $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); 
         $$->type = $1->type;
    }
    | Identifier ADD_ASSIGNMENT expression { 
         $$ = alctree(ADD_ASSIGNMENT, "addAssignment", 2, $1, $3); 
         $$->type = $1->type;
    }
    | Identifier SUB_ASSIGNMENT expression { 
         $$ = alctree(SUB_ASSIGNMENT, "subAssignment", 2, $1, $3); 
         $$->type = $1->type;
    }
    | arrayAccess ASSIGNMENT expression { 
         $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); 
         $$->type = $1->type;
    }
    | arrayAccess ADD_ASSIGNMENT expression { 
         $$ = alctree(ADD_ASSIGNMENT, "addAssignment", 2, $1, $3); 
         $$->type = $1->type;
    }
    | arrayAccess SUB_ASSIGNMENT expression { 
         $$ = alctree(SUB_ASSIGNMENT, "subAssignment", 2, $1, $3); 
         $$->type = $1->type;
    }
    | variableDeclaration { $$ = alctree(ASSIGNMENT, "assignment", 1, $1); $$->type = $1->type; }
    | multiVariableDeclaration ASSIGNMENT expression { $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); $$->type = $1->type; }
    | unaryExpression { $$ = $1; }
    ;

ifStatement:
    IF LPAREN boolExpression RPAREN nl_opt controlStructureBody nl_opt %prec LOWER_THAN_ELSE
        { $$ = alctree(IF, "ifStatement", 2, $3, $6); $$->type = NULL; }
    | IF LPAREN boolExpression RPAREN nl_opt controlStructureBody nl_opt ELSE nl_opt controlStructureBody nl_opt 
        { $$ = alctree(IF, "ifElseStatement", 3, $3, $6, $10); $$->type = NULL; }
    ;

boolExpression:
    expression { $$ = $1; }
    | LPAREN boolExpression RPAREN { $$ = $2; }
    ;

disjunction:
    conjunction { $$ = $1; }
    | disjunction DISJ conjunction { $$ = alctree(DISJ, "disjunction", 2, $1, $3); $$->type = boolean_typeptr; }
    ;

conjunction:
    equality { $$ = $1; }
    | conjunction CONJ equality { $$ = alctree(CONJ, "conjunction", 2, $1, $3); $$->type = boolean_typeptr; }
    ;

equality:
    comparison { $$ = $1; }
    | equality EQEQ comparison { $$ = alctree(EQEQ, "equality", 2, $1, $3); $$->type = boolean_typeptr; }
    | equality EQEQEQ comparison { $$ = alctree(EQEQEQ, "equality", 2, $1, $3); $$->type = boolean_typeptr; }
    | equality EXCL_EQ comparison { $$ = alctree(EXCL_EQ, "equality", 2, $1, $3); $$->type = boolean_typeptr; }
    ;

comparison:
    additive_expression { $$ = $1; }
    | comparison LANGLE additive_expression { $$ = alctree(LANGLE, "comparison", 2, $1, $3); $$->type = boolean_typeptr; }
    | comparison RANGLE additive_expression { $$ = alctree(RANGLE, "comparison", 2, $1, $3); $$->type = boolean_typeptr; }
    | comparison LE additive_expression { $$ = alctree(LE, "comparison", 2, $1, $3); $$->type = boolean_typeptr; }
    | comparison GE additive_expression { $$ = alctree(GE, "comparison", 2, $1, $3); $$->type = boolean_typeptr; }
    ;

logical_unary_expression:
    primary_expression { $$ = $1; }
    | EXCL_NO_WS logical_unary_expression { $$ = alctree(EXCL_NO_WS, "negation", 1, $2); $$->type = boolean_typeptr; }
    | EXCL_WS logical_unary_expression { $$ = alctree(EXCL_WS, "negation", 1, $2); $$->type = boolean_typeptr; }
    ;

variableDeclaration:
    VAL Identifier nl_opt {
         $$ = alctree(VAL, "variableDeclaration", 1, $2);
         $$->is_mutable = 0;
         $$->type = NULL;
    }
    | VAR Identifier nl_opt {
         $$ = alctree(VAR, "variableDeclaration", 1, $2);
         $$->is_mutable = 1;
         $$->type = NULL;
    }
    | CONST VAL Identifier nl_opt {
         $$ = alctree(CONST, "constVariableDeclaration", 1, $3);
         $$->is_mutable = 0;
         $$->type = NULL;
    }
    | VAL Identifier COLON type nl_opt {
         $$ = alctree(VAL, "variableDeclaration", 2, $2, $4);
         $$->is_mutable = 0;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    | VAR Identifier COLON type nl_opt {
         $$ = alctree(VAR, "variableDeclaration", 2, $2, $4);
         $$->is_mutable = 1;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    | CONST VAL Identifier COLON type nl_opt {
         $$ = alctree(CONST, "constVariableDeclaration", 2, $3, $5);
         $$->is_mutable = 0;
         $$->is_nullable = $5->is_nullable;
         $$->type = $5->type;
    }
    | VAL Identifier ASSIGNMENT boolExpression nl_opt {
         $$ = alctree(VAL, "variableDeclaration", 2, $2, $4);
         $$->is_mutable = 0;
         $$->type = $4->type;
    }
    | VAR Identifier ASSIGNMENT boolExpression nl_opt {
         $$ = alctree(VAR, "variableDeclaration", 2, $2, $4);
         $$->is_mutable = 1;
         $$->type = $4->type;
    }
    | CONST VAL Identifier ASSIGNMENT boolExpression nl_opt {
         $$ = alctree(CONST, "constVariableDeclaration", 2, $3, $5);
         $$->is_mutable = 0;
         $$->type = $5->type;
    }
    | VAL Identifier COLON type ASSIGNMENT boolExpression nl_opt {
         $$ = alctree(VAL, "variableDeclaration", 3, $2, $4, $6);
         $$->is_mutable = 0;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    | VAR Identifier COLON type ASSIGNMENT boolExpression nl_opt {
         $$ = alctree(VAR, "variableDeclaration", 3, $2, $4, $6);
         $$->is_mutable = 1;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    | CONST VAL Identifier COLON type ASSIGNMENT boolExpression nl_opt {
         $$ = alctree(CONST, "constVariableDeclaration", 3, $3, $5, $7);
         $$->is_mutable = 0;
         $$->is_nullable = $5->is_nullable;
         $$->type = $5->type;
    }
    | VAL Identifier COLON type arrayInitializer nl_opt {
         $$ = alctree(VAR, "arrayDeclaration", 3, $2, $4, $5);
         $$->is_mutable = 0;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    | VAR Identifier COLON type arrayInitializer nl_opt {
         $$ = alctree(VAR, "arrayDeclaration", 3, $2, $4, $5);
         $$->is_mutable = 1;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    | VAR Identifier COLON type ASSIGNMENT Identifier LANGLE type RANGLE arrayInitializer nl_opt {
         $$ = alctree(VAR, "arrayAssignmentDeclaration", 5, $2, $4, $6, $8, $10);
         $$->is_mutable = 1;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    | VAL Identifier COLON type ASSIGNMENT Identifier LANGLE type RANGLE arrayInitializer nl_opt {
         $$ = alctree(VAL, "arrayAssignmentDeclaration", 5, $2, $4, $6, $8, $10);
         $$->is_mutable = 0;
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    ;

multiVariableDeclaration:
    LPAREN variableDeclarationList RPAREN { $$ = $2; }
    ;

variableDeclarationList:
    variableDeclaration { $$ = $1; }
    | variableDeclarationList COMMA nl_opt variableDeclaration { 
         $$ = alctree(113, "variableDeclarationList", 2, $1, $4); 
         $$->type = NULL; 
      }
    ;

returnType_section:
    COLON nl_opt type { $$ = $3; $$->is_nullable = $3->is_nullable; }
    ;

expression:
    disjunction { $$ = $1; }
    ;

additive_expression:
    multiplicative_expression { $$ = $1; }
    | additive_expression ADD multiplicative_expression { 
          $$ = alctree(114, "additive_expression", 2, $1, $3);
          if (check_type_compatibility($1->type, $3->type))
              $$->type = $1->type;
          else {
              $$->type = double_typeptr;
          }
      }
    | additive_expression SUB multiplicative_expression { 
          $$ = alctree(115, "additive_expression", 2, $1, $3);
          if (check_type_compatibility($1->type, $3->type))
              $$->type = $1->type;
          else {
              $$->type = double_typeptr;
          }
      }
    ;

expressionList:
    expression { $$ = alctree(116, "expressionList", 1, $1); }
    | expressionList COMMA expression { $$ = alctree(117, "expressionList", 2, $1, $3); }
    ;

multiplicative_expression:
    logical_unary_expression { $$ = $1; }
    | multiplicative_expression MULT logical_unary_expression { 
          $$ = alctree(118, "multiplicative_expression", 2, $1, $3);
          if (check_type_compatibility($1->type, $3->type))
              $$->type = $1->type;
          else {
              $$->type = double_typeptr;
          }
      }
    | multiplicative_expression DIV logical_unary_expression { 
          $$ = alctree(119, "multiplicative_expression", 2, $1, $3);
          if (check_type_compatibility($1->type, $3->type))
              $$->type = $1->type;
          else {
              $$->type = double_typeptr;
          }
      }
    | multiplicative_expression MOD logical_unary_expression { 
          $$ = alctree(120, "multiplicative_expression", 2, $1, $3);
          if (check_type_compatibility($1->type, $3->type))
              $$->type = $1->type;
          else {
              $$->type = double_typeptr;
          }
      }
    ;

primary_expression:
    IntegerLiteral { 
         $$ = alctree(IntegerLiteral, "IntegerLiteral", 1, $1); 
         $$->type = integer_typeptr; 
    }
    | RealLiteral { 
         $$ = alctree(RealLiteral, "RealLiteral", 1, $1); 
         $$->type = double_typeptr; 
    }
    | BooleanLiteral { 
         $$ = alctree(BooleanLiteral, "BooleanLiteral", 1, $1); 
         $$->type = boolean_typeptr; 
    }
    | NullLiteral { 
         $$ = alctree(NullLiteral, "NullLiteral", 1, $1); 
         $$->type = null_typeptr; 
    }
    | StringLiteral { 
         $$ = alctree(StringLiteral, "StringLiteral", 1, $1); 
         $$->type = string_typeptr; 
    }
    | Identifier { 
         $$ = alctree(Identifier, "Identifier", 1, $1); 
         /* Type may be resolved later via symbol table lookup */
         $$->type = NULL;
    }
    | primary_expression LSQUARE expression RSQUARE { 
         $$ = alctree(300, "arrayAccess", 2, $1, $3); 
         $$->type = null_typeptr;
    }
    | functionCall { $$ = $1; }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

functionCall:
    Identifier LPAREN functionCallArguments RPAREN nl_opt { 
         $$ = alctree(122, "functionCall", 2, $1, $3); 
         /* Function call type will be set later during semantic analysis after symbol resolution */
         $$->type = NULL;
    }
    ;

functionCallArguments:
    /* epsilon */ { $$ = NULL; }
    | expressionList { $$ = $1; }
    ;

arrayAccess:
    primary_expression LSQUARE expression RSQUARE { 
         $$ = alctree(300, "arrayAccess", 2, $1, $3); 
         $$->type = null_typeptr; 
    }
    ;

arrayInitializer:
    LPAREN expression RPAREN LCURL expression RCURL { 
         $$ = alctree(200, "arrayInitializer", 2, $2, $5); 
         $$->type = NULL; 
    }
    ;

returnStatement:
    RETURN expression { $$ = $2; }
    | RETURN { $$ = alctree(RETURN, "returnStatement", 0); $$->type = NULL; }
    ;

typeAlias:
    TYPE_ALIAS Identifier ASSIGNMENT type { 
         $$ = alctree(TYPE_ALIAS, "typeAlias", 2, $2, $4); 
         $$->is_nullable = $4->is_nullable;
         $$->type = $4->type;
    }
    ;

%%
