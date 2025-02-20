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
    topLevelObjectList
    ;

topLevelObjectList:
    topLevelObject
    | topLevelObjectList topLevelObject
    ;

topLevelObject:
    declaration
    ;

// Declarations
declaration:
    functionDeclaration
    | propertyDeclaration
    | typeAlias
    ;

// Property declaration
propertyDeclaration:
    PROPERTY Identifier COLON type semi
    ;

// Type definition
type:
    Identifier
    | Identifier LANGLE type RANGLE
    ;

// Function declaration
functionDeclaration:
    FUN Identifier functionValueParameters returnType_section functionBody
    | FUN Identifier functionValueParameters functionBody
    ;

functionValueParameters:
    LPAREN functionParameterList_opt RPAREN
    ;

functionParameterList_opt:
    /* epsilon */
    | functionValueParameter
    | functionParameterList_opt COMMA functionValueParameter
    ;

functionValueParameter:
    Identifier COLON type
    ;

// Function components

functionBody:
    block
    ;

block:
    LCURL statements RCURL
    ;

statements:
    /* epsilon */
    | statement nl_opt
    | statements nl_opt statement
    ;

statement:
    expression semi
    | declaration
    | assignment semi
    | loopStatement
    | ifStatement
    | returnStatement semi
    ;

// Loop Statements
loopStatement:
    forStatement
    | whileStatement
    | doWhileStatement
    ;

forStatement:
    FOR nl_opt LPAREN annotation_opt forControl RPAREN nl_opt controlStructureBody
    ;

whileStatement:
    WHILE nl_opt LPAREN expression RPAREN nl_opt controlStructureBody
    | WHILE nl_opt LPAREN expression RPAREN nl_opt SEMICOLON
    ;

doWhileStatement:
    DO nl_opt controlStructureBody nl_opt WHILE nl_opt LPAREN expression RPAREN
    ;

// Control structure components
controlStructureBody:
    block
    | statement
    ;

forControl:
    variableDeclaration IN expression
    | multiVariableDeclaration IN expression
    | Identifier IN expression
    ;

// Assignments
assignment:
    Identifier ASSIGNMENT expression
    | Identifier ADD_ASSIGNMENT expression
    | Identifier SUB_ASSIGNMENT expression
    | variableDeclaration ASSIGNMENT expression
    | multiVariableDeclaration ASSIGNMENT expression
    ;

// Control structures
ifStatement:
    IF LPAREN expression RPAREN controlStructureBody %prec LOWER_THAN_ELSE
    | IF LPAREN expression RPAREN controlStructureBody ELSE controlStructureBody
    ;

// Variable declarations
variableDeclaration:
    VAL nl_opt Identifier
    | VAR nl_opt Identifier
    ;

multiVariableDeclaration:
    LPAREN variableDeclarationList RPAREN
    ;

variableDeclarationList:
    variableDeclaration
    | variableDeclarationList COMMA nl_opt variableDeclaration
    ;

returnType_section:
    COLON nl_opt type
    ;

// Expressions
expression:
    additive_expression
    ;

additive_expression:
    multiplicative_expression
    | additive_expression ADD multiplicative_expression
    | additive_expression SUB multiplicative_expression
    ;

multiplicative_expression:
    primary_expression
    | multiplicative_expression MULT primary_expression
    | multiplicative_expression DIV primary_expression
    | multiplicative_expression MOD primary_expression
    ;

primary_expression:
    IntegerLiteral
    | BooleanLiteral
    | NullLiteral
    | CharacterLiteral
    | Identifier
    | functionCall
    | LPAREN expression RPAREN
    ;

functionCall:
    Identifier LPAREN functionCallArguments RPAREN
    ;

functionCallArguments:
    /* epsilon */
    | expression
    | functionCallArguments COMMA expression
    ;

// Other components
annotation_opt:
    /* epsilon */
    | annotation
    ;

annotation:
    AT_NO_WS Identifier
    | AT_PRE_WS Identifier
    ;

returnStatement:
    RETURN expression
    ;

typeAlias:
    TYPE_ALIAS Identifier ASSIGNMENT type
    ;

%%
