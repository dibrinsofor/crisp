#include <stdio.h>
#include <stdlib.h>

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

    puts("Crisp Version 0.0.0.0.1\n");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        // init prompt and read input
        char *prompt = "crisp> \n";
        char *input = readline(prompt);
        add_history(input);

        // dummy output
        printf("I do not understand: %s\n", input);
        free(input);
    }

    return 0;
}