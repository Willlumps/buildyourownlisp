#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"
#include "parsing.h"

lval eval(mpc_ast_t *t) {
    // Check if there is some error in conversion
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    // Operator will be the second child since '(' is always the first in
    // an expression.
    char *op = t->children[1]->contents;

    // Store the third child in x
    lval x = eval(t->children[2]);

    // Allow for a single argument for a '-' sign allowing negation
    if (strcmp(op, "-") == 0 && t->children_num < 5) { return lval_num(0 - x.num); }

    // Iterate over the remaining children
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

// Use operator string to see which operator to perform
lval eval_op(lval x, char *op, lval y) {
    #define MAX(x, y) (((x) > (y)) ? (x) : (y))
    #define MIN(x, y) (((x) < (y)) ? (x) : (y))

    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
        return lval_num(x.num + y.num);
    }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }
    if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
    if (strcmp(op, "^") == 0) {
        return lval_num((long)pow((double)x.num, (double)y.num));
    }
    if (strcmp(op, "min") == 0) { return lval_num(MIN(x.num, y.num)); }
    if (strcmp(op, "max") == 0) { return lval_num(MAX(x.num, y.num)); }

    return lval_err(LERR_BAD_OP);
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

// Number type lval
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

// Error type lval
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

// Print an "lval"
void lval_print(lval v) {
    switch(v.type) {
        case LVAL_NUM:
            printf("%li", v.num);
            break;
        case LVAL_ERR:
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division by zero.");
            }
            if (v.err == LERR_BAD_OP) {
                printf("Error: Invalid operator.");
            }
            if (v.err == LERR_BAD_NUM) {
                printf("Error: Invalid Number.");
            }
        default:
            break;
    }
}

void lval_println(lval v) {
    lval_print(v);
    printf("\n");
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
            lval result = eval(r.output);
            lval_println(result);
            //printf("Number of leaves: %d\n", numLeaves(r.output));
            //printf("Number of branches: %d\n", numBranches(r.output));
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
