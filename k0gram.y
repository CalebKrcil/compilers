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
%type <treeptr> doWhileStatement controlStructureBody assignment ifStatement
%type <treeptr> variableDeclaration multiVariableDeclaration variableDeclarationList
%type <treeptr> returnType_section expression additive_expression multiplicative_expression
%type <treeptr> primary_expression forInit forUpdate functionCall functionCallArguments 
%type <treeptr> unaryExpression boolExpression returnStatement typeAlias whenStatement whenBranchList whenBranch
%type <treeptr> expressionList

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
    ;

// Program structure
program:
    nl_opt topLevelObjectList nl_opt{
        root = $2;
    }
    ;

topLevelObjectList:
    topLevelObject { $$ = $1; }
    | topLevelObjectList nl_opt topLevelObject { $$ = alctree(0, "topLevelObjectList", 2, $1, $3); }
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
    PROPERTY Identifier COLON type nl_opt { $$ = alctree(PROPERTY, "propertyDeclaration", 2, $2, $4); }
    ;

// Type definition
type:
    Identifier { $$ = alctree(0, "type", 1, $1); }
    | Identifier LANGLE type RANGLE { $$ = alctree(0, "type", 2, $1, $3); }
    ;

// Function declaration
functionDeclaration:
    FUN Identifier functionValueParameters returnType_section functionBody { $$ = alctree(FUN, "functionDeclaration", 4, $2, $3, $4, $5); }
    | FUN Identifier functionValueParameters functionBody { $$ = alctree(FUN, "functionDeclaration", 3, $2, $3, $4); }
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
    LCURL nl_opt RCURL {$$ = NULL;}
    | LCURL statements nl_opt RCURL { $$ = alctree(0, "block", 1, $2); }
    ;

statements:
    nl_opt statement nl_opt { $$ = $2; }
    | statements nl_opt statement { $$ = alctree(0, "statements", 2, $1, $3); }
    ;

statement:
    expression nl_opt { $$ = $1; }
    | declaration nl_opt { $$ = $1; }
    | assignment nl_opt { $$ = $1; }
    | loopStatement nl_opt { $$ = $1; }
    | ifStatement nl_opt { $$ = $1; }
    | whenStatement nl_opt {$$ = $1; }
    | returnStatement nl_opt { $$ = $1; }
    ;

// Loop Statements
loopStatement:
    forStatement { $$ = $1; }
    | whileStatement { $$ = $1; }
    | doWhileStatement { $$ = $1; }
    ;

forStatement:
    FOR LPAREN nl_opt forInit SEMICOLON boolExpression SEMICOLON forUpdate RPAREN controlStructureBody nl_opt
        { $$ = alctree(FOR, "forStatement", 4, $4, $6, $8, $10); }
    | FOR LPAREN Identifier IN expression RANGE expression RPAREN controlStructureBody nl_opt
        { $$ = alctree(FOR, "forStatementKotlinRange", 4, $3, $5, $7, $9); }
    ;


whileStatement:
    WHILE nl_opt LPAREN boolExpression RPAREN nl_opt controlStructureBody { $$ = alctree(WHILE, "whileStatement", 2, $4, $7); }
    ;

doWhileStatement:
    DO nl_opt controlStructureBody nl_opt WHILE nl_opt LPAREN boolExpression RPAREN { $$ = alctree(DO, "doWhileStatement", 2, $3, $8); }
    ;

// Control structure components
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
    | /*epsilon */ {$$ = NULL; }
    ;

unaryExpression:
    INCR Identifier { $$ = alctree(INCR, "preIncrement", 1, $2); }  
    | DECR Identifier { $$ = alctree(DECR, "preDecrement", 1, $2); }  
    | Identifier INCR { $$ = alctree(INCR, "postIncrement", 1, $1); }  
    | Identifier DECR { $$ = alctree(DECR, "postDecrement", 1, $1); }
    ;

// Assignments
assignment:
    Identifier ASSIGNMENT expression { $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); }
    | Identifier ADD_ASSIGNMENT expression { $$ = alctree(ADD_ASSIGNMENT, "addAssignment", 2, $1, $3); }
    | Identifier SUB_ASSIGNMENT expression { $$ = alctree(SUB_ASSIGNMENT, "subAssignment", 2, $1, $3); }
    | variableDeclaration { $$ = alctree(ASSIGNMENT, "assignment", 1, $1); }
    | multiVariableDeclaration ASSIGNMENT expression { $$ = alctree(ASSIGNMENT, "assignment", 2, $1, $3); }
    | unaryExpression { $$ = $1; }
    ;

// Control structures
ifStatement:
    IF LPAREN boolExpression RPAREN nl_opt controlStructureBody nl_opt %prec LOWER_THAN_ELSE
        { $$ = alctree(IF, "ifStatement", 2, $3, $6); }
    | IF LPAREN boolExpression RPAREN nl_opt controlStructureBody nl_opt ELSE nl_opt controlStructureBody nl_opt 
        { $$ = alctree(IF, "ifElseStatement", 3, $3, $6, $10); }    
    ;

boolExpression:
    expression { $$ = $1; }
    | boolExpression EQEQ boolExpression { $$ = alctree(EQEQ, "booleanExpression", 2, $1, $3); }
    | boolExpression EXCL_EQ boolExpression { $$ = alctree(EXCL_EQ, "booleanExpression", 2, $1, $3); }
    | boolExpression LANGLE boolExpression { $$ = alctree(LANGLE, "booleanExpression", 2, $1, $3); }
    | boolExpression RANGLE boolExpression { $$ = alctree(RANGLE, "booleanExpression", 2, $1, $3); }
    | boolExpression LE boolExpression { $$ = alctree(LE, "booleanExpression", 2, $1, $3); }
    | boolExpression GE boolExpression { $$ = alctree(GE, "booleanExpression", 2, $1, $3); }
    | boolExpression CONJ boolExpression { $$ = alctree(CONJ, "booleanExpression", 2, $1, $3); }
    | boolExpression DISJ boolExpression { $$ = alctree(DISJ, "booleanExpression", 2, $1, $3); }
    | EXCL_NO_WS boolExpression { $$ = alctree(EXCL_NO_WS, "booleanExpression", 1, $2); }
    | LPAREN boolExpression RPAREN { $$ = $3; }
    ;

whenStatement:
    WHEN LPAREN expression RPAREN LCURL nl_opt whenBranchList nl_opt RCURL {
        $$ = alctree(WHEN, "whenStatement", 2, $3, $7);
    }
    ;

whenBranchList:
    whenBranch {$$ = $1; }
    | whenBranchList nl_opt whenBranch { $$ = alctree(0, "whenBranches", 2, $1, $3); }
    ;

whenBranch:
    expression ARROW controlStructureBody { $$ = alctree(0, "whenBranch", 2, $1, $3); }
    | ELSE ARROW controlStructureBody { $$ = alctree(ELSE, "whenBranchElse", 1, $3); }
    ;

// Variable declarations
variableDeclaration:
    VAL Identifier nl_opt { $$ = alctree(VAL, "variableDeclaration", 1, $2); }
    | VAR Identifier nl_opt { $$ = alctree(VAR, "variableDeclaration", 1, $2); }
    | VAL Identifier COLON type nl_opt { $$ = alctree(VAL, "variableDeclaration", 2, $2, $4); }
    | VAR Identifier COLON type nl_opt { $$ = alctree(VAR, "variableDeclaration", 2, $2, $4); }
    | VAL Identifier ASSIGNMENT expression nl_opt { $$ = alctree(VAL, "variableDeclaration", 2, $2, $4); }
    | VAR Identifier ASSIGNMENT expression nl_opt { $$ = alctree(VAR, "variableDeclaration", 2, $2, $4); }
    | VAL Identifier COLON type ASSIGNMENT expression nl_opt { $$ = alctree(VAL, "variableDeclaration", 3, $2, $4, $6); }
    | VAR Identifier COLON type ASSIGNMENT expression nl_opt { $$ = alctree(VAR, "variableDeclaration", 3, $2, $4, $6); }
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
    additive_expression nl_opt { $$ = $1; }
    ;

additive_expression:
    multiplicative_expression { $$ = $1; }
    | additive_expression ADD multiplicative_expression { $$ = alctree(0, "additive_expression", 2, $1, $3); }
    | additive_expression SUB multiplicative_expression { $$ = alctree(0, "additive_expression", 2, $1, $3); }
    ;

expressionList:
    expression { $$ = alctree(0, "expressionList", 1, $1); }
    | expressionList COMMA expression { $$ = alctree(0, "expressionList", 2, $1, $3); }
    ;

multiplicative_expression:
    primary_expression { $$ = $1; }
    | multiplicative_expression MULT primary_expression { $$ = alctree(0, "multiplicative_expression", 2, $1, $3); }
    | multiplicative_expression DIV primary_expression { $$ = alctree(0, "multiplicative_expression", 2, $1, $3); }
    | multiplicative_expression MOD primary_expression { $$ = alctree(0, "multiplicative_expression", 2, $1, $3); }
    ;

primary_expression:
    IntegerLiteral { $$ = alctree(IntegerLiteral, "IntegerLiteral", 1, $1); }
    | BooleanLiteral { $$ = alctree(BooleanLiteral, "BooleanLiteral", 1, $1); }
    | NullLiteral { $$ = alctree(NullLiteral, "NullLiteral", 1, $1); }
    | CharacterLiteral { $$ = alctree(CharacterLiteral, "CharacterLiteral", 1, $1); }
    | Identifier { $$ = alctree(Identifier, "Identifier", 1, $1); }
    | Identifier DOT primary_expression { $$ = alctree(0, "qualifiedName", 2, $1, $3); }
    | functionCall { $$ = $1; }
    | LPAREN expression RPAREN { $$ = $2; }
    ;


functionCall:
    Identifier LPAREN functionCallArguments RPAREN nl_opt { $$ = alctree(0, "functionCall", 2, $1, $3); }
    ;

functionCallArguments:
    /* epsilon */ { $$ = NULL; }
    | expressionList { $$ = $1; }
    ;


returnStatement:
    RETURN expression { $$ = $2; }|
    RETURN {$$ = $1;}
    ;

typeAlias:
    TYPE_ALIAS Identifier ASSIGNMENT type { $$ = alctree(TYPE_ALIAS, "typeAlias", 2, $2, $4); }
    ;

%%