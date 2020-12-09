#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"

long eval(mpc_ast_t *t);
long eval_op(long x, char *op, long y);
int numLeaves(mpc_ast_t *t);
int numBranches(mpc_ast_t *t);

long eval(mpc_ast_t *t) {
    // If number, return that number
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    // Operator will be the second child since '(' is always the first in
    // an expression.
    char *op = t->children[1]->contents;

    // Store the third child in x
    long x = eval(t->children[2]);

    // Allow for a single argument for a '-' sign allowing negation
    if (strcmp(op, "-") == 0 && t->children_num < 5) { return 0 - x; }

    // Iterate over the remaining children
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

// Use operator string to see which operator to perform
long eval_op(long x, char *op, long y) {
    #define MAX(x, y) (((x) > (y)) ? (x) : (y))
    #define MIN(x, y) (((x) < (y)) ? (x) : (y))
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) { return (long)pow((double)x, (double)y); }
    if (strcmp(op, "min") == 0) { return MIN(x, y); }
    if (strcmp(op, "max") == 0) { return MAX(x, y); }
    return 0;
}

int numLeaves(mpc_ast_t *t) {
    int count = 0;

    if (t->children_num == 0) {
        return 1;
    } else {
        for (int i = 0; i < t->children_num; i++) {
            if (strstr(t->children[i]->tag, "expr") == NULL) {
                continue;
            } else {
                count += numLeaves(t->children[i]);
            }
        }
    }

    return count;
}

int numBranches(mpc_ast_t *t) {
    // Root
    int count = 0;
    if (t->children_num > 0) {
        count += 1;
        for (int i = 0; i < t->children_num; i++) {
            count += numBranches(t->children[i]);
        }
    }
    return count;
}

int main(int argc, char** argv) {

    // Create some parsers
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define them with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                      \
            number   : /-?[0-9]+(\\.[0-9])*/;                  \
            operator : '+' | '-' | '*' | '/' | '%' | '^' |     \
                       /add/ | /sub/ | /mul/ | /div/ |         \
                       /max/ | /min/ ;                         \
            expr     : <number> | '(' <operator> <expr>+ ')' ; \
            lispy    : /^/ <operator> <expr>+ /$/ ;            \
        ",
        Number, Operator, Expr, Lispy);

    printf("Lisp version 0.0.0.1\n");
    printf("Ctrl-C to exit\n");

    while(1) {
        char *input = readline("Lisp> ");

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            // On success print the result of the evaluation
            long result = eval(r.output);
            printf("Evaluation: %li\n", result);
            printf("Number of leaves: %d\n", numLeaves(r.output));
            printf("Number of branches: %d\n", numBranches(r.output));
            mpc_ast_delete(r.output);
        } else {
            // Otherwise print the error
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        add_history(input);
        free(input);
    }

    // Undefine and delete our parsers
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}
