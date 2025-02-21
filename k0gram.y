%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "tree.h"
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
%token <treeptr> BooleanLiteral NullLiteral CharacterLiteral Identifier
%token <treeptr> HexLiteral BinLiteral

%type <treeptr> program topLevelObject topLevelObjectList declaration 
%type <treeptr> propertyDeclaration type functionDeclaration functionValueParameters
%type <treeptr> functionParameterList_opt functionValueParameter functionBody
%type <treeptr> block statements statement loopStatement forStatement whileStatement
%type <treeptr> doWhileStatement controlStructureBody forControl assignment ifStatement
%type <treeptr> variableDeclaration multiVariableDeclaration variableDeclarationList
%type <treeptr> returnType_section expression additive_expression multiplicative_expression
%type <treeptr> primary_expression functionCall functionCallArguments annotation_opt
%type <treeptr> annotation returnStatement typeAlias

%start program

%left ADD 
%left SUB
%left MULT 
%left DIV 
%left MOD

%union {
   struct tree *treeptr;
}

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

// Basic separator rules
semi:
    SEMICOLON nl_opt
    | nl nl_opt
    ;

nl_opt:
    /* epsilon */
    | nl
    | nl_opt nl
    ;

nl:
    NL
    | nl NL
    ;

// Program structure
program:
    topLevelObjectList{
        root = $1;
        printtree(root, 0);
    }
    ;

topLevelObjectList:
    topLevelObject { $$ = $1; }
    | topLevelObjectList topLevelObject { $$ = alctree(0, "topLevelObjectList", 2, $1, $2); }
    ;

topLevelObject:
    declaration { $$ = $1; }
    ;

// Declarations
declaration:
    functionDeclaration { $$ = $1; }
    | propertyDeclaration { $$ = $1; }
    | typeAlias { $$ = $1; }
    ;

// Property declaration
propertyDeclaration:
    PROPERTY Identifier COLON type semi { $$ = alctree(PROPERTY, "propertyDeclaration", 2, $2, $4); }
    ;

// Type definition
type:
    Identifier { $$ = $1; }
    | Identifier LANGLE type RANGLE { $$ = alctree(0, "type", 2, $1, $3); }
    ;

// Function declaration
functionDeclaration:
    FUN Identifier functionValueParameters returnType_section functionBody { $$ = alctree(FUN, "functionDeclaration", 4, $2, $3, $4, $5); }
    | FUN Identifier functionValueParameters functionBody { $$ = alctree(FUN, "functionDeclaration", 2, $2, $3); }
    ;

functionValueParameters:
    LPAREN functionParameterList_opt RPAREN { $$ = alctree(0, "functionValueParameters", 1, $2); }
    ;

functionParameterList_opt:
    /* epsilon */ { $$ = NULL; }
    | functionValueParameter { $$ = $1; }
    | functionParameterList_opt COMMA functionValueParameter { $$ = alctree(0, "functionParameterList", 2, $1, $3); }
    ;

functionValueParameter:
    Identifier COLON type { $$ = alctree(0, "functionValueParameter", 2, $1, $3); }
    ;

// Function components

functionBody:
    block { $$ = $1; }
    ;

block:
    LCURL statements RCURL { $$ = alctree(0, "block", 1, $2); }
    ;

statements:
    /* epsilon */ { $$ = NULL; }
    | statement nl_opt { $$ = $1; }
    | statements nl_opt statement { $$ = alctree(0, "statements", 2, $1, $3); }
    ;

statement:
    expression semi { $$ = $1; }
    | declaration { $$ = $1; }
    | assignment semi { $$ = $1; }
    | loopStatement { $$ = $1; }
    | ifStatement { $$ = $1; }
    | returnStatement semi { $$ = $1; }
    ;

// Loop Statements
loopStatement:
    forStatement { $$ = $1; }
    | whileStatement { $$ = $1; }
    | doWhileStatement { $$ = $1; }
    ;

forStatement:
    FOR nl_opt LPAREN annotation_opt forControl RPAREN nl_opt controlStructureBody { $$ = alctree(FOR, "forStatement", 3, $4, $5, $8); }
    ;

whileStatement:
    WHILE nl_opt LPAREN expression RPAREN nl_opt controlStructureBody { $$ = alctree(WHILE, "whileStatement", 2, $4, $7); }
    | WHILE nl_opt LPAREN expression RPAREN nl_opt SEMICOLON { $$ = alctree(WHILE, "whileStatement", 1, $4); }
    ;

doWhileStatement:
    DO nl_opt controlStructureBody nl_opt WHILE nl_opt LPAREN expression RPAREN { $$ = alctree(DO, "doWhileStatement", 2, $3, $8); }
    ;

// Control structure components
controlStructureBody:
    block { $$ = $1; }
    | statement { $$ = $1; }
    ;

forControl:
    variableDeclaration IN expression { $$ = alctree(0, "forControl", 2, $1, $3); }
    | multiVariableDeclaration IN expression { $$ = alctree(0, "forControl", 2, $1, $3); }
    | Identifier IN expression { $$ = alctree(0, "forControl", 2, $1, $3); }
    ;

// Assignments
assignment:
    Identifier ASSIGNMENT expression { $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); }
    | Identifier ADD_ASSIGNMENT expression { $$ = alctree(ADD_ASSIGNMENT, "addAssignment", 2, $1, $3); }
    | Identifier SUB_ASSIGNMENT expression { $$ = alctree(SUB_ASSIGNMENT, "subAssignment", 2, $1, $3); }
    | variableDeclaration ASSIGNMENT expression { $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); }
    | multiVariableDeclaration ASSIGNMENT expression { $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); }
    ;

// Control structures
ifStatement:
    IF LPAREN expression RPAREN controlStructureBody %prec LOWER_THAN_ELSE { $$ = alctree(IF, "ifStatement", 2, $3, $5); }
    | IF LPAREN expression RPAREN controlStructureBody ELSE controlStructureBody { $$ = alctree(IF, "ifStatement", 3, $3, $5, $7); }
    ;

// Variable declarations
variableDeclaration:
    VAL nl_opt Identifier { $$ = $1; }
    | VAR nl_opt Identifier { $$ = $1; }
    ;

multiVariableDeclaration:
    LPAREN variableDeclarationList RPAREN { $$ = $2; }
    ;

variableDeclarationList:
    variableDeclaration { $$ = $1; }
    | variableDeclarationList COMMA nl_opt variableDeclaration { $$ = alctree(0, "variableDeclarationList", 2, $1, $4); }
    ;

returnType_section:
    COLON nl_opt type { $$ = $3; }
    ;

// Expressions
expression:
    additive_expression { $$ = $1; }
    ;

additive_expression:
    multiplicative_expression { $$ = $1; }
    | additive_expression ADD multiplicative_expression { $$ = alctree(0, "additive_expression", 2, $1, $3); }
    | additive_expression SUB multiplicative_expression { $$ = alctree(0, "additive_expression", 2, $1, $3); }
    ;

multiplicative_expression:
    primary_expression { $$ = $1; }
    | multiplicative_expression MULT primary_expression { $$ = alctree(0, "multiplicative_expression_expression", 2, $1, $3); }
    | multiplicative_expression DIV primary_expression { $$ = alctree(0, "multiplicative_expression_expression", 2, $1, $3); }
    | multiplicative_expression MOD primary_expression { $$ = alctree(0, "multiplicative_expression_expression", 2, $1, $3); }
    ;

primary_expression:
    IntegerLiteral { $$ = $1; }
    | BooleanLiteral { $$ = $1; }
    | NullLiteral { $$ = $1; }
    | CharacterLiteral { $$ = $1; }
    | Identifier { $$ = $1; }
    | functionCall { $$ = $1; }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

functionCall:
    Identifier LPAREN functionCallArguments RPAREN { $$ = alctree(0, "functionCall", 2, $1, $3); }
    ;

functionCallArguments:
    /* epsilon */ { $$ = NULL; }
    | expression { $$ = $1; }
    | functionCallArguments COMMA expression { $$ = alctree(0, "functionCallArguments", 2, $1, $3); }
    ;

// Other components
annotation_opt:
    /* epsilon */ { $$ = NULL; }
    | annotation { $$ = $1; }
    ;

annotation:
    AT_NO_WS Identifier { $$ = $2; }
    | AT_PRE_WS Identifier { $$ = $2; }
    ;

returnStatement:
    RETURN expression { $$ = $2; }
    ;

typeAlias:
    TYPE_ALIAS Identifier ASSIGNMENT type { $$ = alctree(TYPE_ALIAS, "typeAlias", 2, $2, $4); }
    ;

%%
