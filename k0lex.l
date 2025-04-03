%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "k0gram.tab.h"
#include "tree.h"

extern int yylineno;
extern char *current_filename;  
extern void update_last_token(const char *token_text);  
int yylex();
extern YYSTYPE yylval;
void yyerror(const char *s);

int comment_depth = 0;
char string_buffer[16384];
int string_pos = 0;
int multiline_start_line = 0;
%}

%x IN_COMMENT  
%x IN_MULTILINE_STRING  

%option noyywrap
%option noinput
%option nounput

WS        [ \t\f]
NEWLINE   \r\n|\n|\r
ID        [a-zA-Z_][a-zA-Z0-9_]*
NUMBER    -?[0-9]+
FLOAT     -?[0-9]+\.[0-9]+([eE][-+]?[0-9]+)?
HEX       0[xX][0-9a-fA-F]+
STRING    \"(\\.|[^"\\])*\"|'(\\.|[^'\\])*'
SHEBANG   ^#!*

%%

{SHEBANG}  { yylineno++; }
{WS}+      { /* Ignore */ }
{NEWLINE}+ { yylineno++; return alctoken(NL, yytext);}
"//".*     { yylineno++; }
"#".*      { yylineno++; }

\"\"\"     { 
    BEGIN(IN_MULTILINE_STRING); 
    string_pos = 0; 
    multiline_start_line = yylineno; 
    memset(string_buffer, 0, sizeof(string_buffer)); 
}

"/*"  { comment_depth = 1; BEGIN(IN_COMMENT); }

"*/" {
    fprintf(stderr, "Error: Unexpected '*/' at line %d in %s\n", yylineno, current_filename);
    return 0;
}

"as" { fprintf(stderr, "Error: Unsupported keyword 'as' at line %d\n", yylineno); exit(1); }
"as?" { fprintf(stderr, "Error: Unsupported keyword 'as?' at line %d\n", yylineno); exit(1); }
"class" { fprintf(stderr, "Error: Unsupported keyword 'class' at line %d\n", yylineno); exit(1); }
"!in" { fprintf(stderr, "Error: Unsupported keyword '!in' at line %d\n", yylineno); exit(1); }
"is" { fprintf(stderr, "Error: Unsupported keyword 'is' at line %d\n", yylineno); exit(1); }
"!is" { fprintf(stderr, "Error: Unsupported keyword '!is' at line %d\n", yylineno); exit(1); }
"object" { fprintf(stderr, "Error: Unsupported keyword 'object' at line %d\n", yylineno); exit(1); }
"package" { fprintf(stderr, "Error: Unsupported keyword 'package' at line %d\n", yylineno); exit(1); }
"super" { fprintf(stderr, "Error: Unsupported keyword 'super' at line %d\n", yylineno); exit(1); }
"this" { fprintf(stderr, "Error: Unsupported keyword 'this' at line %d\n", yylineno); exit(1); }
"throw" { fprintf(stderr, "Error: Unsupported keyword 'throw' at line %d\n", yylineno); exit(1); }
"try" { fprintf(stderr, "Error: Unsupported keyword 'try' at line %d\n", yylineno); exit(1); }
"typealias" { fprintf(stderr, "Error: Unsupported keyword 'typealias' at line %d\n", yylineno); exit(1); }
"typeof" { fprintf(stderr, "Error: Unsupported keyword 'typeof' at line %d\n", yylineno); exit(1); }
"by" { fprintf(stderr, "Error: Unsupported keyword 'by' at line %d\n", yylineno); exit(1); }
"catch" { fprintf(stderr, "Error: Unsupported keyword 'catch' at line %d\n", yylineno); exit(1); }
"constructor" { fprintf(stderr, "Error: Unsupported keyword 'constructor' at line %d\n", yylineno); exit(1); }
"delegate" { fprintf(stderr, "Error: Unsupported keyword 'delegate' at line %d\n", yylineno); exit(1); }
"dynamic" { fprintf(stderr, "Error: Unsupported keyword 'dynamic' at line %d\n", yylineno); exit(1); }
"field" { fprintf(stderr, "Error: Unsupported keyword 'field' at line %d\n", yylineno); exit(1); }
"file" { fprintf(stderr, "Error: Unsupported keyword 'file' at line %d\n", yylineno); exit(1); }
"finally" { fprintf(stderr, "Error: Unsupported keyword 'finally' at line %d\n", yylineno); exit(1); }
"get" { fprintf(stderr, "Error: Unsupported keyword 'get' at line %d\n", yylineno); exit(1); }
"init" { fprintf(stderr, "Error: Unsupported keyword 'init' at line %d\n", yylineno); exit(1); }
"param" { fprintf(stderr, "Error: Unsupported keyword 'param' at line %d\n", yylineno); exit(1); }
"property" { fprintf(stderr, "Error: Unsupported keyword 'property' at line %d\n", yylineno); exit(1); }
"receiver" { fprintf(stderr, "Error: Unsupported keyword 'receiver' at line %d\n", yylineno); exit(1); }
"set" { fprintf(stderr, "Error: Unsupported keyword 'set' at line %d\n", yylineno); exit(1); }
"setparam" { fprintf(stderr, "Error: Unsupported keyword 'setparam' at line %d\n", yylineno); exit(1); }
"value" { fprintf(stderr, "Error: Unsupported keyword 'value' at line %d\n", yylineno); exit(1); }
"where" { fprintf(stderr, "Error: Unsupported keyword 'where' at line %d\n", yylineno); exit(1); }
"when" { fprintf(stderr, "Error: Unsupported keyword 'when' at line %d\n", yylineno); exit(1); }

"..." { fprintf(stderr, "Error: Unsupported operator '...' at line %d\n", yylineno); exit(1); }
"*=" { fprintf(stderr, "Error: Unsupported operator '*=' at line %d\n", yylineno); exit(1); }
"/=" { fprintf(stderr, "Error: Unsupported operator '/=' at line %d\n", yylineno); exit(1); }
"%=" { fprintf(stderr, "Error: Unsupported operator '%%=' at line %d\n", yylineno); exit(1); }
"->" { fprintf(stderr, "Error: Unsupported operator '->' at line %d\n", yylineno); exit(1); }
"=>" { fprintf(stderr, "Error: Unsupported operator '=>' at line %d\n", yylineno); exit(1); }
"::" { fprintf(stderr, "Error: Unsupported operator '::' at line %d\n", yylineno); exit(1); }
";;" { fprintf(stderr, "Error: Unsupported operator ';;' at line %d\n", yylineno); exit(1); }
"@" { fprintf(stderr, "Error: Unsupported operator '@' at line %d\n", yylineno); exit(1); }
"'" { fprintf(stderr, "Error: Unsupported operator ''' at line %d\n", yylineno); exit(1); }

"`" { fprintf(stderr, "Error: Unsupported punctuation '`' at line %d\n", yylineno); exit(1); }

"~" { fprintf(stderr, "Error: Bitwise operator '~' at line %d\n", yylineno); exit(1); }
"&=" { fprintf(stderr, "Error: Bitwise operator '&=' at line %d\n", yylineno); exit(1); }
"|=" { fprintf(stderr, "Error: Bitwise operator '|=' at line %d\n", yylineno); exit(1); }
"|" { fprintf(stderr, "Error: Bitwise operator '|' at line %d\n", yylineno); exit(1); }
"^=" { fprintf(stderr, "Error: Bitwise operator '^=' at line %d\n", yylineno); exit(1); }
"^" { fprintf(stderr, "Error: Bitwise operator '^' at line %d\n", yylineno); exit(1); }
">>=" { fprintf(stderr, "Error: Bitwise operator '>>=' at line %d\n", yylineno); exit(1); }
">>" { fprintf(stderr, "Error: Bitwise operator '>>' at line %d\n", yylineno); exit(1); }
"<<=" { fprintf(stderr, "Error: Bitwise operator '<<=' at line %d\n", yylineno); exit(1); }
"<<" { fprintf(stderr, "Error: Bitwise operator '<<' at line %d\n", yylineno); exit(1); }
"&" { fprintf(stderr, "Error: Bitwise operator '&' at line %d\n", yylineno); exit(1); }



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
"?"           { update_last_token("?"); return alctoken(QUEST_NO_WS, yytext); }
".."          { update_last_token(".."); return alctoken(RANGE, yytext); }
"..<"         { update_last_token("..<"); return alctoken(RANGE_UNTIL, yytext); }

"break"         { update_last_token("break"); return alctoken(BREAK, yytext); }
"continue"      { update_last_token("continue"); return alctoken(CONTINUE, yytext); }
"do"            { update_last_token("do"); return alctoken(DO, yytext); }
"else"          { update_last_token("else"); return alctoken(ELSE, yytext); }
"false"         { update_last_token("false"); return alctoken(BooleanLiteral, yytext); }
"for"           { update_last_token("for"); return alctoken(FOR, yytext); }
"fun"           { update_last_token("fun"); return alctoken(FUN, yytext); }
"if"            { update_last_token("if"); return alctoken(IF, yytext); }
"null"          { update_last_token("null"); return alctoken(NullLiteral, yytext); }
"return"        { update_last_token("return"); return alctoken(RETURN, yytext); }
"true"          { update_last_token("true"); return alctoken(BooleanLiteral, yytext); }
"val"           { update_last_token("val"); return alctoken(VAL, yytext); }
"var"           { update_last_token("var"); return alctoken(VAR, yytext); }
"while"         { update_last_token("while"); return alctoken(WHILE, yytext); }
"import"        { update_last_token("import"); return alctoken(IMPORT, yytext); }
"const"         { update_last_token("const"); return alctoken(CONST, yytext); }
"return@"       { update_last_token("return@"); return alctoken(RETURN_AT, yytext); }
"continue@"     { update_last_token("continue@"); return alctoken(CONTINUE_AT, yytext); }
"break@"        { update_last_token("break@"); return alctoken(BREAK_AT, yytext); }
"override"      { update_last_token("override"); return alctoken(OVERRIDE, yytext); }
"in"            { update_last_token("in"); return alctoken(IN, yytext); }
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
        exit(1);
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
        
        // Restore line number to the start of the multiline string
        yylineno = multiline_start_line;
        
        return alctoken(CharacterLiteral, yytext); 
    }

    \\\"\"\"  { 
        // Escaped triple quote within multiline string
        string_buffer[string_pos++] = '"';
        string_buffer[string_pos++] = '"';
        string_buffer[string_pos++] = '"';
    }

    \\\n    { 
        // Line continuation - ignore newline and continue 
        // This allows multiline strings to span multiple lines without adding newline
    }

    \n      { 
        // Preserve actual newlines in the string
        yylineno++; 
        string_buffer[string_pos++] = '\n';
    }

    \\n     { 
        // Explicit newline escape sequence
        string_buffer[string_pos++] = '\n';
    }

    \\r     { 
        // Carriage return escape sequence
        string_buffer[string_pos++] = '\r';
    }

    \\t     { 
        // Tab escape sequence
        string_buffer[string_pos++] = '\t';
    }

    \\.     { 
        // Other escape sequences 
        string_buffer[string_pos++] = yytext[1];
    }

    .       { 
        // Any other character in multiline string
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
