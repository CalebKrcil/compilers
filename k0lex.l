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
{NEWLINE}+ { yylineno++; return NL; }
"//".*     { /* Ignore line comments */ }
"#".*      { /* Ignore hash-based comments */ }

\"\"\"     { BEGIN(IN_MULTILINE_STRING); }

"/*"  { comment_depth = 1; BEGIN(IN_COMMENT); }

"*/" {
    fprintf(stderr, "Error: Unexpected '*/' at line %d in %s\n", yylineno, current_filename);
    return 0;
}

/* Operators in k0 */
","           { update_last_token(","); return COMMA; }
"("           { update_last_token("("); return LPAREN; }
")"           { update_last_token(")"); return RPAREN; }
"["           { update_last_token("["); return LSQUARE; }
"]"           { update_last_token("]"); return RSQUARE; }
"{"           { update_last_token("{"); return LCURL; }
"}"           { update_last_token("}"); return RCURL; }
":"           { update_last_token(":"); return COLON; }
";"           { update_last_token(";"); return SEMICOLON; }
"="           { update_last_token("="); return ASSIGNMENT; }
"+="          { update_last_token("+="); return ADD_ASSIGNMENT; }
"-="          { update_last_token("-="); return SUB_ASSIGNMENT; }
"*"           { update_last_token("*"); return MULT; }
"%"           { update_last_token("%"); return MOD; }
"/"           { update_last_token("/"); return DIV; }
"+"           { update_last_token("+"); return ADD; }
"-"           { update_last_token("-"); return SUB; }
"++"          { update_last_token("++"); return INCR; }
"--"          { update_last_token("--"); return DECR; }
"<"           { update_last_token("<"); return LANGLE; }
">"           { update_last_token(">"); return RANGLE; }
"<="          { update_last_token("<="); return LE; }
">="          { update_last_token(">="); return GE; }
"!="          { update_last_token("!="); return EXCL_EQ; }
"!=="         { update_last_token("!=="); return EXCL_EQEQ; }
"==="         { update_last_token("==="); return EQEQEQ; }
"=="          { update_last_token("=="); return EQEQ; }
"&&"          { update_last_token("&&"); return CONJ; }
"||"          { update_last_token("||"); return DISJ; }
"!"{WS}+      { update_last_token("! "); return EXCL_WS; }
"!"           { update_last_token("!"); return EXCL_NO_WS; }
"."           { update_last_token("."); return DOT; }
"?"           { update_last_token("?"); return QUEST_WS; }
".."          { update_last_token(".."); return RANGE; }
"..<"         { update_last_token("..<"); return RANGE_UNTIL; }

/* Operators not in k0? */
"..."         { update_last_token("..."); return RESERVED; }
"*="          { update_last_token("*="); return MULT_ASSIGNMENT; }
"/="          { update_last_token("/="); return DIV_ASSIGNMENT; }
"%="          { update_last_token("%="); return MOD_ASSIGNMENT; }
"->"          { update_last_token("->"); return ARROW; }
"=>"          { update_last_token("=>"); return DOUBLE_ARROW; }
"::"          { update_last_token("::"); return COLONCOLON; }
";;"          { update_last_token(";;"); return DOUBLE_SEMICOLON; }
"@"           { update_last_token("@"); return AT_NO_WS; }
"@ "          { update_last_token("@ "); return AT_POST_WS; }
" @"          { update_last_token(" @"); return AT_PRE_WS; }
" @ "         { update_last_token(" @ "); return AT_BOTH_WS; }
"??"          { update_last_token("??"); return QUEST_NO_WS; }
"'"           { update_last_token("'"); return SINGLE_QUOTE; }
"as\?"        { update_last_token("as?"); return AS_SAFE; }
"&"           { update_last_token("&"); return AMP; }

/* Keywords in k0 */
"break"         { update_last_token("break"); return BREAK; }
"continue"      { update_last_token("continue"); return CONTINUE; }
"do"            { update_last_token("do"); return DO; }
"else"          { update_last_token("else"); return ELSE; }
"false"         { update_last_token("false"); return BooleanLiteral; }
"for"           { update_last_token("for"); return FOR; }
"fun"           { update_last_token("fun"); return FUN; }
"if"            { update_last_token("if"); return IF; }
"in"            { update_last_token("in"); return IN; }
"null"          { update_last_token("null"); return NullLiteral; }
"return"        { update_last_token("return"); return RETURN; }
"true"          { update_last_token("true"); return BooleanLiteral; }
"val"           { update_last_token("val"); return VAL; }
"var"           { update_last_token("var"); return VAR; }
"when"          { update_last_token("when"); return WHEN; }
"while"         { update_last_token("while"); return WHILE; }
"import"        { update_last_token("import"); return IMPORT; }
"const"         { update_last_token("const"); return CONST; }

/* Keywords not in k0 */
"return@"       { update_last_token("return@"); return RETURN_AT; }
"continue@"     { update_last_token("continue@"); return CONTINUE_AT; }
"break@"        { update_last_token("break@"); return BREAK_AT; }
"this@"         { update_last_token("this@"); return THIS_AT; }
"super@"        { update_last_token("super@"); return SUPER_AT; }
"file"          { update_last_token("file"); return K0_FILE; }
"field"         { update_last_token("field"); return FIELD; }
"property"      { update_last_token("property"); return PROPERTY; }
"get"           { update_last_token("get"); return GET; }
"set"           { update_last_token("set"); return SET; }
"receiver"      { update_last_token("receiver"); return RECIEVER; }
"param"         { update_last_token("param"); return PARAM; }
"setparam"      { update_last_token("setparam"); return SETPARAM; }
"delegate"      { update_last_token("delegate"); return DELEGATE; }
"package"       { update_last_token("package"); return PACKAGE; }
"class"         { update_last_token("class"); return CLASS; }
"interface"     { update_last_token("interface"); return INTERFACE; }
"fun"           { update_last_token("fun"); return FUN; }
"object"        { update_last_token("object"); return OBJECT; }
"typealias"     { update_last_token("typealias"); return TYPE_ALIAS; }
"constructor"   { update_last_token("constructor"); return CONSTRUCTOR; }
"by"            { update_last_token("by"); return BY; }
"companion"     { update_last_token("companion"); return COMPANION; }
"init"          { update_last_token("init"); return INIT; }
"this"          { update_last_token("this"); return THIS; }
"super"         { update_last_token("super"); return SUPER; }
"typeof"        { update_last_token("typeof"); return TYPEOF; }
"where"         { update_last_token("where"); return WHERE; }
"try"           { update_last_token("try"); return TRY; }
"catch"         { update_last_token("catch"); return CATCH; }
"finally"       { update_last_token("finally"); return FINALLY; }
"throw"         { update_last_token("throw"); return THROW; }
"as"            { update_last_token("as"); return AS; }
"is"            { update_last_token("is"); return IS; }
"!is"           { update_last_token("!is"); return NOT_IS; }
"!in"           { update_last_token("!in"); return NOT_IN; }
"out"           { update_last_token("out"); return OUT; }
"dynamic"       { update_last_token("dynamic"); return DYNAMIC; }
"public"        { update_last_token("public"); return PUBLIC; }
"private"       { update_last_token("private"); return PRIVATE; }
"protected"     { update_last_token("protected"); return PROTECTED; }
"internal"      { update_last_token("internal"); return INTERNAL; }
"enum"          { update_last_token("enum"); return ENUM; }
"sealed"        { update_last_token("sealed"); return SEALED; }
"annotation"    { update_last_token("annotation"); return ANNOTATION; }
"data"          { update_last_token("data"); return DATA; }
"inner"         { update_last_token("inner"); return INNER; }
"value"         { update_last_token("value"); return VALUE; }
"tailrec"       { update_last_token("tailrec"); return TAILREC; }
"operator"      { update_last_token("operator"); return OPERATOR; }
"inline"        { update_last_token("inline"); return INLINE; }
"infix"         { update_last_token("infix"); return INFIX; }
"external"      { update_last_token("external"); return EXTERNAL; }
"suspend"       { update_last_token("suspend"); return SUSPEND; }
"override"      { update_last_token("override"); return OVERRIDE; }
"abstract"      { update_last_token("abstract"); return ABSTRACT; }
"final"         { update_last_token("final"); return FINAL; }
"open"          { update_last_token("open"); return OPEN; }
"lateinit"      { update_last_token("lateinit"); return LATEINIT; }
"vararg"        { update_last_token("vararg"); return VARARG; }
"noinline"      { update_last_token("noinline"); return NOINLINE; }
"crossinline"   { update_last_token("crossinline"); return CROSSINLINE; }
"reified"       { update_last_token("reified"); return REIFIED; }
"expect"        { update_last_token("expect"); return EXPECT; }
"actual"        { update_last_token("actual"); return ACTUAL; }

{NUMBER}      { update_last_token(yytext); return IntegerLiteral; }
{FLOAT}       { update_last_token(yytext); return RealLiteral; }
{BIN}         { update_last_token(yytext); return BinLiteral; }
{STRING}      { update_last_token(yytext); return CharacterLiteral; }
{ID}          { update_last_token(yytext); return Identifier; }

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
        return CharacterLiteral; 
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
