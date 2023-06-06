#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#ifdef _WIN32
#include <string.h>

#define bufsize 2048
static char buffer[bufsize];

char *readline(char * prompt) {
    fputs(prompt, stdout);

    // read input of max bufsize
    fgets(buffer, bufsize, stdin);

    char *cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';

    return cpy;
}

void add_history(char * unused) {}

// these things are called preprocessors
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif
 
int main(int argc, char** argv) {

    mpc_parser_t * Number = mpc_new("number");
    mpc_parser_t * Operator = mpc_new("operator");
    mpc_parser_t * Expr = mpc_new("expr");
    mpc_parser_t * Crisp = mpc_new("crisp");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
        number   : /-?[0-9]+/ ;                             \
        operator : '+' | '-' | '*' | '/' ;                  \
        expr     : <number> | '(' <operator> <expr>+ ')' ;  \
        crisp    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Crisp);

    puts("Crisp Version 0.0.0.0.1\n");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        // init prompt and read input
        char *prompt = "crisp> \n";
        char *input = readline(prompt);
        add_history(input);

        // parse input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Crisp, &r)) {
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        // printf("I do not understand: %s\n", input);
        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Crisp);

    return 0;
}