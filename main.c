#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
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
            printf("No errors\n");

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
            if (generate_dot) {
                char dot_filename[300];
                snprintf(dot_filename, sizeof(dot_filename), "%s.dot", filepath);
                print_graph(root, dot_filename);
                printf("DOT file generated: %s\n", dot_filename);
            }
        } else {
            fprintf(stderr, "\nParsing completed with %d semantic error(s)\n", error_count);
            parse_result = 3;  
        }
        
        free_func_symtab_list(func_symtabs);
    } else {
        fprintf(stderr, "\nParsing failed with %d error(s)\n", error_count);
    }

    generate_code(root);
    write_ic_file(current_filename, root->code);

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
        fprintf(stderr, "Usage: %s <input_file> [-tree] [-symtab] [-dot]\n", argv[0]);
        return 1;
    }

    int print_tree = 0, print_symtab = 0, generate_dot = 0;
    int highest_error_code = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-tree") == 0) {
            print_tree = 1;
        } else if (strcmp(argv[i], "-symtab") == 0) {
            print_symtab = 1;
        } else if (strcmp(argv[i], "-dot") == 0) {
            generate_dot = 1;
        }
    }

    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {  
            int result = process_file(argv[i], print_tree, print_symtab, generate_dot);
            if (result > highest_error_code) {
                highest_error_code = result;
            }
        }
    }

    return highest_error_code;
}