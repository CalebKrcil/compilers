#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "k0gram.tab.h"
#include "semantics.h"
#include "tree.h"
#include "symtab.h"
#include "codegen.h"
#define EXTENSION ".kt"

extern int yylex();
extern int yyparse();
extern char *yytext;
extern FILE *yyin;
extern int yylineno;
extern char *current_filename;
extern struct tree *root;

char *current_filename = NULL;

int error_count = 0;
#define MAX_ERROR_MSG_LENGTH 1024
char last_token[256] = "";

SymbolTable globalSymtab;

struct tokenlist {
    struct token *t;
    struct tokenlist *next;
};

struct tokenlist *head = NULL;
struct tokenlist *tail = NULL;

static void finish_and_emit(const char *stem, bool emit_asm, bool emit_obj) {
    char sfile[512], ofile[512], cmd[2048];
    size_t need;

    /* build names "<stem>.s" and "<stem>.o" */
    snprintf(sfile, sizeof sfile, "%s.s", stem);
    snprintf(ofile, sizeof ofile, "%s.o", stem);

    /* if user only wanted the .s, stop here */
    if (emit_asm) return;

    /* 1) assemble → .o */
    need = strlen(sfile) + strlen(ofile) + sizeof("as  -o ") + 1;
    if (need > sizeof cmd) {
        fprintf(stderr, "error: command line too long\n");
        exit(1);
    }
    snprintf(cmd, sizeof cmd, "as %s -o %s", sfile, ofile);
    if (system(cmd) != 0) {
        fprintf(stderr, "error: assembler failed\n");
        exit(1);
    }

    /* if user only wanted the .o, stop here */
    if (emit_obj) return;

    /* 2) link → executable named "stem" */
    need = strlen(ofile) + strlen(stem) + sizeof("cc  -o ") + 1;
    if (need > sizeof cmd) {
        fprintf(stderr, "error: command line too long\n");
        exit(1);
    }
    snprintf(cmd, sizeof cmd, "cc %s -o %s", ofile, stem);
    if (system(cmd) != 0) {
        fprintf(stderr, "error: linker failed\n");
        exit(1);
    }
}

void yyerror(const char *s) {
    error_count++;
    char *near_text = (*yytext != '\0') ? yytext : "end of file";
    fprintf(stderr, "\nError #%d at line %d: %s\n", error_count, yylineno, s);
    fprintf(stderr, "Near token: '%s'\n", near_text);

    FILE *f = fopen(current_filename, "r");
    if (f) {
        char line[1024];
        int current_line = 1;
        while (fgets(line, sizeof(line), f) && current_line <= yylineno) {
            if (current_line == yylineno) {
                fprintf(stderr, "Line %d: %s", yylineno, line);
                int i;
                for (i = 0; i < strlen("Line ") + floor(log10(yylineno)) + 2; i++) {
                    fprintf(stderr, " ");
                }
                fprintf(stderr, "^\n");
                break;
            }
            current_line++;
        }
        fclose(f);
    }

    if (strstr(s, "syntax error")) {
        if (strcmp(last_token, "fun") == 0) {
            fprintf(stderr, "Note: Function declarations require a name, parameter list, and body\n");
        }
        else if (strcmp(last_token, "BITWISE") == 0) {
            fprintf(stderr, "Note: Bitwise Unsupported in K0\n");
        }
        else {
            fprintf(stderr, "Hint: Check for missing semicolons, brackets, or incorrect token ordering\n");
        }

        exit(2);
    } else {
        exit(1);
    }
}

void add_extension_if_needed(char *filename) {
    size_t len = strlen(filename);

    if (len >= 3 && strcmp(filename + len - 3, EXTENSION) == 0) {
        return;
    }

    strcat(filename, EXTENSION);
}

char *process_escape_sequences(const char *input) {
    size_t len = strlen(input);
    
    if (len < 2 || (input[0] != '"' && input[0] != '\'')) {
        return strdup(input);
    }

    char *output = malloc(len - 1);
    if (!output) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    char *dest = output;
    const char *src = input + 1;

    while (*src && *(src + 1)) {
        if (*src == '\\' && *(src + 1)) {
            src++;
            switch (*src) {
                case 't': *dest = '\t'; break;
                case 'n': *dest = '\n'; break;
                case 'r': *dest = '\r'; break;
                case '\\': *dest = '\\'; break;
                case '"': *dest = '\"'; break;
                case '\'': *dest = '\''; break;
                default: *dest = *src; break;
            }
        } else {
            *dest = *src;
        }
        dest++;
        src++;
    }

    *dest = '\0';
    return output;
}

void update_last_token(const char *token_text) {
    strncpy(last_token, token_text, sizeof(last_token) - 1);
    last_token[sizeof(last_token) - 1] = '\0';
}

void add_token(int category, char *text, int lineno, const char *filename) {
    update_last_token(text);  
    
    struct token *new_token = malloc(sizeof(struct token));
    struct tokenlist *new_node = malloc(sizeof(struct tokenlist));

    new_token->category = category;
    new_token->text = strdup(text);
    new_token->lineno = lineno;
    new_token->filename = strdup(filename);

    new_token->value.ival = 0;
    new_token->value.dval = 0.0;
    new_token->value.sval = NULL;
    if (category == IntegerLiteral) {
        new_token->value.ival = atoi(text);
    } 
    else if (category == RealLiteral) {
        new_token->value.dval = atof(text);
    }
    else if (category == StringLiteral) {
        new_token->value.sval = process_escape_sequences(text);
    }

    new_node->t = new_token;
    new_node->next = NULL;

    if (tail) {
        tail->next = new_node;
        tail = new_node;
    } else {
        head = tail = new_node;
    }
}

void free_tokens() {
    struct tokenlist *current = head;
    while (current) {
        struct tokenlist *next = current->next;
        free(current->t->text);
        free(current->t->filename);
        if (current->t->category == StringLiteral) free(current->t->value.sval);
        free(current->t);
        free(current);
        current = next;
    }
    head = tail = NULL; 
}

void assign_first(struct tree *t)
{
    if (!t) return;
    
    for(int i = 0; i < t->nkids; i++) {
        assign_first(t->kids[i]);
    }
    
    if (t->symbolname) {
        if (strcmp(t->symbolname, "while_statement") == 0 ||
            strcmp(t->symbolname, "if_statement") == 0 ||
            strcmp(t->symbolname, "else_clause") == 0 ||
            strcmp(t->symbolname, "for_statement") == 0) {
            
            struct addr *label = genlabel();
            t->first = *label;
            t->first_used = 1;
            free(label); 
            
            printf("Assigned first label %d to %s node\n", 
                        t->first.u.offset, t->symbolname);
        }
    }
}

void assign_follow(struct tree *t)
{
    if (!t) return;
    
    if (t->symbolname) {
        if (strcmp(t->symbolname, "while_statement") == 0 && t->nkids >= 2) {

            struct tree *cond = t->kids[0];
            struct tree *body = t->kids[1];
            
            if (cond && body) {
                if (cond->first_used) {
                    body->follow = cond->first;
                    body->follow_used = 1;
                } else {
                    struct addr *loop_label = genlabel();
                    body->follow = *loop_label;
                    body->follow_used = 1;
                    free(loop_label);
                }
                
                if (t->follow_used) {
                    cond->follow = t->follow;
                    cond->follow_used = 1;
                }
            }
        }
        else if (strcmp(t->symbolname, "if_statement") == 0 && t->nkids >= 2) {

            struct tree *cond = t->kids[0];
            struct tree *then_clause = t->kids[1];
            struct tree *else_clause = t->nkids > 2 ? t->kids[2] : NULL;
            
            if (cond && then_clause) {
                if (then_clause->first_used) {
                    cond->follow = then_clause->first;
                    cond->follow_used = 1;
                }
                
                if (t->follow_used) {
                    then_clause->follow = t->follow;
                    then_clause->follow_used = 1;
                    
                    if (else_clause) {
                        else_clause->follow = t->follow;
                        else_clause->follow_used = 1;
                    }
                }
                
                if (else_clause && else_clause->first_used) {
                    cond->onFalse = else_clause->first;
                    cond->onFalse_used = 1;
                } else if (t->follow_used) {
                    cond->onFalse = t->follow;
                    cond->onFalse_used = 1;
                }
            }
        }
        else if (strcmp(t->symbolname, "statement_sequence") == 0 && t->nkids >= 2) {
            for (int i = 0; i < t->nkids - 1; i++) {
                if (t->kids[i] && t->kids[i+1] && t->kids[i+1]->first_used) {
                    t->kids[i]->follow = t->kids[i+1]->first;
                    t->kids[i]->follow_used = 1;
                }
            }
            
            if (t->nkids > 0 && t->kids[t->nkids-1] && t->follow_used) {
                t->kids[t->nkids-1]->follow = t->follow;
                t->kids[t->nkids-1]->follow_used = 1;
            }
        }
    }
    
    for(int i = 0; i < t->nkids; i++) {
        assign_follow(t->kids[i]);
    }
}

void assign_conditional_labels(struct tree *t)
{
    if (!t) return;
    
    if (t->symbolname) {
        if (strcmp(t->symbolname, "comparison") == 0 || 
            strcmp(t->symbolname, "logical_and") == 0 ||
            strcmp(t->symbolname, "logical_or") == 0) {
            
            if (!t->onTrue_used) {
                struct addr *true_label = genlabel();
                t->onTrue = *true_label;
                t->onTrue_used = 1;
                free(true_label);
            }
            
            if (!t->onFalse_used) {
                struct addr *false_label = genlabel();
                t->onFalse = *false_label;
                t->onFalse_used = 1;
                free(false_label);
            }
            
            if (strcmp(t->symbolname, "logical_and") == 0 && t->nkids >= 2) {

                t->kids[0]->onFalse = t->onFalse;
                t->kids[0]->onFalse_used = 1;
                
                struct addr *second_label = genlabel();
                t->kids[0]->onTrue = *second_label;
                t->kids[0]->onTrue_used = 1;
                free(second_label);
                
                t->kids[1]->onTrue = t->onTrue;
                t->kids[1]->onTrue_used = 1;
                t->kids[1]->onFalse = t->onFalse;
                t->kids[1]->onFalse_used = 1;
            }
            else if (strcmp(t->symbolname, "logical_or") == 0 && t->nkids >= 2) {
                t->kids[0]->onTrue = t->onTrue;
                t->kids[0]->onTrue_used = 1;
                
                struct addr *second_label = genlabel();
                t->kids[0]->onFalse = *second_label;
                t->kids[0]->onFalse_used = 1;
                free(second_label);
                
                t->kids[1]->onTrue = t->onTrue;
                t->kids[1]->onTrue_used = 1;
                t->kids[1]->onFalse = t->onFalse;
                t->kids[1]->onFalse_used = 1;
            }
        }
    }
    
    for(int i = 0; i < t->nkids; i++) {
        assign_conditional_labels(t->kids[i]);
    }
}

int process_file(char *filename, int print_tree, int print_symtab, int generate_dot) {
    char filepath[256];
    strncpy(filepath, filename, sizeof(filepath) - 1);
    filepath[sizeof(filepath) - 1] = '\0';

    add_extension_if_needed(filepath);
    current_filename = strdup(filepath);

    yyin = fopen(filepath, "r");
    if (!yyin) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    printf("Processing file: %s\n", filepath);

    globalSymtab = mksymtab(50, NULL);
    set_package_scope_name(globalSymtab, "global");
    SymbolTable packageSymtab = mksymtab(50, globalSymtab);
    set_package_scope_name(packageSymtab, "main"); 
    add_predefined_symbols(globalSymtab);

    error_count = 0;
    yylineno = 1;

    int parse_result = yyparse();
    if (parse_result == 0) {

        FuncSymbolTableList func_symtabs = printsyms(root, packageSymtab);
        
        if (error_count == 0) {
            check_semantics(root);
        }

        if (error_count == 0) {
            // printf("No errors\n");

            if (print_symtab) {
                print_symbols(globalSymtab);
                print_symbols(packageSymtab);
                
                FuncSymbolTableList current = func_symtabs;
                while (current) {
                    print_symbols(current->symtab);
                    current = current->next;
                }
            }

            if (print_tree) {
                printf("Syntax tree for %s:\n", filepath);
                printtree(root, 0);
            }
        } else {
            fprintf(stderr, "\nParsing completed with %d semantic error(s)\n", error_count);
            parse_result = 3;  
        }


        assign_first(root);
        
        assign_follow(root);
        
        assign_conditional_labels(root);
        generate_code(root);
        if (generate_dot) {
            char dot_filename[300];
            snprintf(dot_filename, sizeof(dot_filename), "%s.dot", filepath);
            print_graph(root, dot_filename);
            printf("DOT file generated: %s\n", dot_filename);
        
            char tac_dot_filename[300];
            snprintf(tac_dot_filename, sizeof(tac_dot_filename), "%sTAC.dot", filepath);
            print_graph_TAC(root, tac_dot_filename);
            printf("TAC DOT file generated: %s\n", tac_dot_filename);
        }
        write_asm_file(current_filename, root->code);
        write_ic_file(current_filename, root->code);
        
        free_func_symtab_list(func_symtabs);
    } else {
        fprintf(stderr, "\nParsing failed with %d error(s)\n", error_count);
    }

    fclose(yyin);
    free(current_filename);
    free_symbol_table(packageSymtab);
    free_symbol_table(globalSymtab);
    freetree(root);
    root = NULL;
    free_tokens();

    return parse_result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage: %s <input_file.kt> [-tree] [-symtab] [-dot] [-s] [-c]\n",
                argv[0]);
        return 1;
    }

    int print_tree   = 0;
    int print_symtab = 0;
    int generate_dot = 0;
    bool flag_s      = false;  /* -s: stop after emitting .s */
    bool flag_c      = false;  /* -c: stop after emitting .o */
    int  exit_code   = 0;

    /* scan flags */
    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-tree")   == 0) print_tree   = 1;
        else if (strcmp(argv[i], "-symtab") == 0) print_symtab = 1;
        else if (strcmp(argv[i], "-dot")    == 0) generate_dot = 1;
        else if (strcmp(argv[i], "-s")      == 0) flag_s       = true;
        else if (strcmp(argv[i], "-c")      == 0) flag_c       = true;
    }

    /* for each non-flag argument */
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') continue;

        /* 1) parse/semantic/generate code + write out `stem.s` */
        int r = process_file(argv[i],
                             print_tree,
                             print_symtab,
                             generate_dot);
        if (r) {
            exit_code = r;
            continue;
        }

        /* 2) strip extension: "foo.kt" → "foo" */
        char stem[256];
        strncpy(stem, argv[i], sizeof(stem)-1);
        stem[sizeof(stem)-1] = '\0';
        char *dot = strrchr(stem, '.');
        if (dot != NULL) {
            *dot = '\0';
        }

        /* 3) assemble/link as needed */
        finish_and_emit(stem, flag_s, flag_c);
    }

    return exit_code;
}