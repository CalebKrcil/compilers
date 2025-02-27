#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "k0gram.tab.h"
#define EXTENSION ".kt"

extern int yylex();
extern int yyparse();
extern char *yytext;
extern FILE *yyin;
extern int yylineno;
extern char *current_filename;  // Add this to track current file

char *current_filename = NULL;

// Add these globals to track parsing state
int error_count = 0;
#define MAX_ERROR_MSG_LENGTH 1024
char last_token[256] = "";

struct token {
    int category;
    char *text;
    int lineno;
    char *filename;
    int ival;
    double dval;
    char *sval;
};

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

    // Print the line where the error occurred
    FILE *f = fopen(current_filename, "r");
    if (f) {
        char line[1024];
        int current_line = 1;
        while (fgets(line, sizeof(line), f) && current_line <= yylineno) {
            if (current_line == yylineno) {
                fprintf(stderr, "Line %d: %s", yylineno, line);
                // Print an arrow pointing to the error position
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

    // Provide hint based on error type
    if (strstr(s, "syntax error")) {
        // More specific hints
        if (strcmp(last_token, "fun") == 0) {
            fprintf(stderr, "Note: Function declarations require a name, parameter list, and body\n");
        }
        else if (strcmp(last_token, "BITWISE") == 0) {
            fprintf(stderr, "Note: Bitwise Unsupported in K0\n");
        }
        else {
            fprintf(stderr, "Hint: Check for missing semicolons, brackets, or incorrect token ordering\n");
        }

        // Return exit code 2 for syntax errors
        exit(2);
    } else {
        // Return exit code 1 for lexical errors
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

// Add this function to track the last token seen
void update_last_token(const char *token_text) {
    strncpy(last_token, token_text, sizeof(last_token) - 1);
    last_token[sizeof(last_token) - 1] = '\0';
}

// Modified add_token function
void add_token(int category, char *text, int lineno, const char *filename) {
    update_last_token(text);  // Track the token
    
    struct token *new_token = malloc(sizeof(struct token));
    struct tokenlist *new_node = malloc(sizeof(struct tokenlist));

    new_token->category = category;
    new_token->text = strdup(text);
    new_token->lineno = lineno;
    new_token->filename = strdup(filename);

    new_token->ival = 0;
    new_token->dval = 0.0;
    new_token->sval = NULL;
    if (category == IntegerLiteral) {
        new_token->ival = atoi(text);
    } 
    else if (category == RealLiteral) {
        new_token->dval = atof(text);
    }
    else if (category == CharacterLiteral) {
        printf("%s", text);
        new_token->sval = process_escape_sequences(text);
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

void print_tokens() {
    printf("Category\tText\t\tLinenum\tFilename\tIval/Sval\n");
    printf("-------------------------------------------------------------------------\n");

    struct tokenlist *current = head;
    while (current) {
        struct token *t = current->t;
        printf("%-8d\t%-8s\t%-4d\t%-12s", t->category, t->text, t->lineno, t->filename);
        if (t->category == IntegerLiteral) {
            printf("\t%d", t->ival);
        } 
        else if (t->category == RealLiteral) {
            printf("\t%lf", t->dval);
        }
        else if (t->category == CharacterLiteral && t->sval) {
            printf("\t%s", t->sval);
        }
        printf("\n");
        current = current->next;
    }
}

void free_tokens() {
    struct tokenlist *current = head;
    while (current) {
        struct tokenlist *next = current->next;
        free(current->t->text);
        free(current->t->filename);
        if (current->t->category == CharacterLiteral) free(current->t->sval);
        free(current->t);
        free(current);
        current = next;
    }
    head = tail = NULL;  // Ensure the list is reset
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char filename[256];
    strncpy(filename, argv[1], sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    add_extension_if_needed(filename);
    current_filename = strdup(filename);  // Store filename globally

    yyin = fopen(filename, "r");
    if (!yyin) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    int parse_result = yyparse();
    if (parse_result == 0) {
        printf("Parsing completed successfully!\n");
    } else {
        printf("\nParsing failed with %d error(s)\n", error_count);
    }

    fclose(yyin);
    free(current_filename);

    print_tokens();
    free_tokens();
    return parse_result;
}
