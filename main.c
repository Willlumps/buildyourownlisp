#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>


int main(int argc, char** argv) {

    printf("Lisp version 0.0.0.1\n");
    printf("Ctrl-C to exit\n");

    while(1) {
        char *input = readline("Lisp> ");
        add_history(input);
        printf("%s\n", input);
        free(input);
    }
    return 0;
}
