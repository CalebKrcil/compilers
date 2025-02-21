%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "k0gram.tab.h"
#include "tree.h"

extern int yylineno;
extern char *current_filename;  // Add this
extern void update_last_token(const char *token_text);  // Add this
int yylex();
void yyerror(const char *s);

int comment_depth = 0;
char string_buffer[16384];
int string_pos = 0;
%}

%x IN_COMMENT  
%x IN_MULTILINE_STRING  

%option noyywrap
%option noinput
%option nounput

WS        [ \t\f]
NEWLINE   \n|\r\n?
ID        [a-zA-Z_][a-zA-Z0-9_]*
NUMBER    -?[0-9]+
FLOAT     -?[0-9]+\.[0-9]+([eE][-+]?[0-9]+)?
HEX       0[xX][0-9a-fA-F]+
STRING    \"(\\.|[^"\\])*\"|'(\\.|[^'\\])*'
SHEBANG   ^#![^\r\n]*

%%

{SHEBANG}  { /* Ignore */ }
{WS}+      { /* Ignore */ }
{NEWLINE}+ { yylineno++; return alctoken(NL, yytext);}
"//".*     { /* Ignore line comments */ }
"#".*      { /* Ignore hash-based comments */ }

\"\"\"     { BEGIN(IN_MULTILINE_STRING); }

"/*"  { comment_depth = 1; BEGIN(IN_COMMENT); }

"*/" {
    fprintf(stderr, "Error: Unexpected '*/' at line %d in %s\n", yylineno, current_filename);
    return 0;
}

","           { update_last_token(","); return alctoken(COMMA, yytext); }
"("           { update_last_token("("); return alctoken(LPAREN, yytext); }
")"           { update_last_token(")"); return alctoken(RPAREN, yytext); }
"["           { update_last_token("["); return alctoken(LSQUARE, yytext); }
"]"           { update_last_token("]"); return alctoken(RSQUARE, yytext); }
"{"           { update_last_token("{"); return alctoken(LCURL, yytext); }
"}"           { update_last_token("}"); return alctoken(RCURL, yytext); }
":"           { update_last_token(":"); return alctoken(COLON, yytext); }
";"           { update_last_token(";"); return alctoken(SEMICOLON, yytext); }
"="           { update_last_token("="); return alctoken(ASSIGNMENT, yytext); }
"+="          { update_last_token("+="); return alctoken(ADD_ASSIGNMENT, yytext); }
"-="          { update_last_token("-="); return alctoken(SUB_ASSIGNMENT, yytext); }
"*"           { update_last_token("*"); return alctoken(MULT, yytext); }
"%"           { update_last_token("%"); return alctoken(MOD, yytext); }
"/"           { update_last_token("/"); return alctoken(DIV, yytext); }
"+"           { update_last_token("+"); return alctoken(ADD, yytext); }
"-"           { update_last_token("-"); return alctoken(SUB, yytext); }
"++"          { update_last_token("++"); return alctoken(INCR, yytext); }
"--"          { update_last_token("--"); return alctoken(DECR, yytext); }
"<"           { update_last_token("<"); return alctoken(LANGLE, yytext); }
">"           { update_last_token(">"); return alctoken(RANGLE, yytext); }
"<="          { update_last_token("<="); return alctoken(LE, yytext); }
">="          { update_last_token(">="); return alctoken(GE, yytext); }
"!="          { update_last_token("!="); return alctoken(EXCL_EQ, yytext); }
"!=="         { update_last_token("!=="); return alctoken(EXCL_EQEQ, yytext); }
"==="         { update_last_token("==="); return alctoken(EQEQEQ, yytext); }
"=="          { update_last_token("=="); return alctoken(EQEQ, yytext); }
"&&"          { update_last_token("&&"); return alctoken(CONJ, yytext); }
"||"          { update_last_token("||"); return alctoken(DISJ, yytext); }
"!"{WS}+      { update_last_token("! "); return alctoken(EXCL_WS, yytext); }
"!"           { update_last_token("!"); return alctoken(EXCL_NO_WS, yytext); }
"."           { update_last_token("."); return alctoken(DOT, yytext); }
"?"           { update_last_token("?"); return alctoken(QUEST_WS, yytext); }
".."          { update_last_token(".."); return alctoken(RANGE, yytext); }
"..<"         { update_last_token("..<"); return alctoken(RANGE_UNTIL, yytext); }

"..."         { update_last_token("..."); return alctoken(RESERVED, yytext); }
"*="          { update_last_token("*="); return alctoken(MULT_ASSIGNMENT, yytext); }
"/="          { update_last_token("/="); return alctoken(DIV_ASSIGNMENT, yytext); }
"%="          { update_last_token("%="); return alctoken(MOD_ASSIGNMENT, yytext); }
"->"          { update_last_token("->"); return alctoken(ARROW, yytext); }
"=>"          { update_last_token("=>"); return alctoken(DOUBLE_ARROW, yytext); }
"::"          { update_last_token("::"); return alctoken(COLONCOLON, yytext); }
";;"          { update_last_token(";;"); return alctoken(DOUBLE_SEMICOLON, yytext); }
"@"           { update_last_token("@"); return alctoken(AT_NO_WS, yytext); }
"@ "          { update_last_token("@ "); return alctoken(AT_POST_WS, yytext); }
" @"          { update_last_token(" @"); return alctoken(AT_PRE_WS, yytext); }
" @ "         { update_last_token(" @ "); return alctoken(AT_BOTH_WS, yytext); }
"??"          { update_last_token("??"); return alctoken(QUEST_NO_WS, yytext); }
"'"           { update_last_token("'"); return alctoken(SINGLE_QUOTE, yytext); }
"as\?"        { update_last_token("as?"); return alctoken(AS_SAFE, yytext); }
"&"           { update_last_token("&"); return alctoken(AMP, yytext); }

"break"         { update_last_token("break"); return alctoken(BREAK, yytext); }
"continue"      { update_last_token("continue"); return alctoken(CONTINUE, yytext); }
"do"            { update_last_token("do"); return alctoken(DO, yytext); }
"else"          { update_last_token("else"); return alctoken(ELSE, yytext); }
"false"         { update_last_token("false"); return alctoken(BooleanLiteral, yytext); }
"for"           { update_last_token("for"); return alctoken(FOR, yytext); }
"fun"           { update_last_token("fun"); return alctoken(FUN, yytext); }
"if"            { update_last_token("if"); return alctoken(IF, yytext); }
"in"            { update_last_token("in"); return alctoken(IN, yytext); }
"null"          { update_last_token("null"); return alctoken(NullLiteral, yytext); }
"return"        { update_last_token("return"); return alctoken(RETURN, yytext); }
"true"          { update_last_token("true"); return alctoken(BooleanLiteral, yytext); }
"val"           { update_last_token("val"); return alctoken(VAL, yytext); }
"var"           { update_last_token("var"); return alctoken(VAR, yytext); }
"when"          { update_last_token("when"); return alctoken(WHEN, yytext); }
"while"         { update_last_token("while"); return alctoken(WHILE, yytext); }
"import"        { update_last_token("import"); return alctoken(IMPORT, yytext); }
"const"         { update_last_token("const"); return alctoken(CONST, yytext); }

"return@"       { update_last_token("return@"); return alctoken(RETURN_AT, yytext); }
"continue@"     { update_last_token("continue@"); return alctoken(CONTINUE_AT, yytext); }
"break@"        { update_last_token("break@"); return alctoken(BREAK_AT, yytext); }
"this@"         { update_last_token("this@"); return alctoken(THIS_AT, yytext); }
"super@"        { update_last_token("super@"); return alctoken(SUPER_AT, yytext); }
"file"          { update_last_token("file"); return alctoken(K0_FILE, yytext); }
"field"         { update_last_token("field"); return alctoken(FIELD, yytext); }
"property"      { update_last_token("property"); return alctoken(PROPERTY, yytext); }
"get"           { update_last_token("get"); return alctoken(GET, yytext); }
"set"           { update_last_token("set"); return alctoken(SET, yytext); }
"receiver"      { update_last_token("receiver"); return alctoken(RECIEVER, yytext); }
"param"         { update_last_token("param"); return alctoken(PARAM, yytext); }
"setparam"      { update_last_token("setparam"); return alctoken(SETPARAM, yytext); }
"delegate"      { update_last_token("delegate"); return alctoken(DELEGATE, yytext); }
"package"       { update_last_token("package"); return alctoken(PACKAGE, yytext); }
"class"         { update_last_token("class"); return alctoken(CLASS, yytext); }
"interface"     { update_last_token("interface"); return alctoken(INTERFACE, yytext); }
"object"        { update_last_token("object"); return alctoken(OBJECT, yytext); }
"typealias"     { update_last_token("typealias"); return alctoken(TYPE_ALIAS, yytext); }
"constructor"   { update_last_token("constructor"); return alctoken(CONSTRUCTOR, yytext); }
"by"            { update_last_token("by"); return alctoken(BY, yytext); }
"companion"     { update_last_token("companion"); return alctoken(COMPANION, yytext); }
"init"          { update_last_token("init"); return alctoken(INIT, yytext); }
"this"          { update_last_token("this"); return alctoken(THIS, yytext); }
"super"         { update_last_token("super"); return alctoken(SUPER, yytext); }
"typeof"        { update_last_token("typeof"); return alctoken(TYPEOF, yytext); }
"where"         { update_last_token("where"); return alctoken(WHERE, yytext); }
"try"           { update_last_token("try"); return alctoken(TRY, yytext); }
"catch"         { update_last_token("catch"); return alctoken(CATCH, yytext); }
"finally"       { update_last_token("finally"); return alctoken(FINALLY, yytext); }
"throw"         { update_last_token("throw"); return alctoken(THROW, yytext); }
"as"            { update_last_token("as"); return alctoken(AS, yytext); }
"is"            { update_last_token("is"); return alctoken(IS, yytext); }
"!is"           { update_last_token("!is"); return alctoken(NOT_IS, yytext); }
"!in"           { update_last_token("!in"); return alctoken(NOT_IN, yytext); }
"out"           { update_last_token("out"); return alctoken(OUT, yytext); }
"dynamic"       { update_last_token("dynamic"); return alctoken(DYNAMIC, yytext); }
"public"        { update_last_token("public"); return alctoken(PUBLIC, yytext); }
"private"       { update_last_token("private"); return alctoken(PRIVATE, yytext); }
"protected"     { update_last_token("protected"); return alctoken(PROTECTED, yytext); }
"internal"      { update_last_token("internal"); return alctoken(INTERNAL, yytext); }
"enum"          { update_last_token("enum"); return alctoken(ENUM, yytext); }
"sealed"        { update_last_token("sealed"); return alctoken(SEALED, yytext); }
"annotation"    { update_last_token("annotation"); return alctoken(ANNOTATION, yytext); }
"data"          { update_last_token("data"); return alctoken(DATA, yytext); }
"inner"         { update_last_token("inner"); return alctoken(INNER, yytext); }
"value"         { update_last_token("value"); return alctoken(VALUE, yytext); }
"tailrec"       { update_last_token("tailrec"); return alctoken(TAILREC, yytext); }
"operator"      { update_last_token("operator"); return alctoken(OPERATOR, yytext); }
"inline"        { update_last_token("inline"); return alctoken(INLINE, yytext); }
"infix"         { update_last_token("infix"); return alctoken(INFIX, yytext); }
"external"      { update_last_token("external"); return alctoken(EXTERNAL, yytext); }
"suspend"       { update_last_token("suspend"); return alctoken(SUSPEND, yytext); }
"override"      { update_last_token("override"); return alctoken(OVERRIDE, yytext); }
"abstract"      { update_last_token("abstract"); return alctoken(ABSTRACT, yytext); }
"final"         { update_last_token("final"); return alctoken(FINAL, yytext); }
"open"          { update_last_token("open"); return alctoken(OPEN, yytext); }
"lateinit"      { update_last_token("lateinit"); return alctoken(LATEINIT, yytext); }
"vararg"        { update_last_token("vararg"); return alctoken(VARARG, yytext); }
"noinline"      { update_last_token("noinline"); return alctoken(NOINLINE, yytext); }
"crossinline"   { update_last_token("crossinline"); return alctoken(CROSSINLINE, yytext); }
"reified"       { update_last_token("reified"); return alctoken(REIFIED, yytext); }
"expect"        { update_last_token("expect"); return alctoken(EXPECT, yytext); }
"actual"        { update_last_token("actual"); return alctoken(ACTUAL, yytext); }

{NUMBER}      { update_last_token(yytext); return alctoken(IntegerLiteral, yytext); }
{FLOAT}       { update_last_token(yytext); return alctoken(RealLiteral, yytext); }
{STRING}      { update_last_token(yytext); return alctoken(CharacterLiteral, yytext); }
{ID}          { update_last_token(yytext); return alctoken(Identifier, yytext); }

. {
    fprintf(stderr, "Error: Unrecognized token '%s' at line %d in %s\n", 
            yytext, yylineno, current_filename);
    return 0;
}

<<EOF>> {
    if (comment_depth > 0) {
        fprintf(stderr, "Error: Unclosed comment at line %d in %s\n", 
                yylineno, current_filename);
        return 0;
    }
    return -1;  /* Normal EOF */
}

<IN_MULTILINE_STRING>{
    \"\"\"    { 
        BEGIN(INITIAL); 
        string_buffer[string_pos] = '\0';
        yytext = string_buffer;
        yyleng = string_pos;
        update_last_token(yytext);
        return alctoken(CharacterLiteral, yytext); 
    }
    \n        { 
        yylineno++; 
        string_buffer[string_pos++] = '\n';
    }
    .         { 
        string_buffer[string_pos++] = yytext[0];
    }
}

<IN_COMMENT>"/*"   { comment_depth++; }
<IN_COMMENT>"*/"   { 
    if (--comment_depth == 0) BEGIN(INITIAL); 
}

<IN_COMMENT>\n { yylineno++; }

<IN_COMMENT>. { }

%%
