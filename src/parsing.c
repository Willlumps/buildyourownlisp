#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"
#include "parsing.h"
/*
lval eval(mpc_ast_t *t) {
    // Check if there is some error in conversion
    if (strstr(t->tag, "number")) {
        errno = 0;
        double x = strtod(t->contents, NULL);
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
    if (strcmp(op, "%") == 0) {
        // Only allow modulo if both are "integers"
        double xInt, yInt;
        if (modf(x.num, &xInt) == 0 && modf(y.num, &yInt) == 0) {
            return lval_num((int)x.num % (int)y.num);
        }
        return lval_err(LERR_MOD_FLOAT);
    }
    if (strcmp(op, "^") == 0) {
        return lval_num(pow(x.num, y.num));
    }
    if (strcmp(op, "min") == 0) { return lval_num(MIN(x.num, y.num)); }
    if (strcmp(op, "max") == 0) { return lval_num(MAX(x.num, y.num)); }

    return lval_err(LERR_BAD_OP);
}
*/

lval* lval_eval_sexpr(lval *v) {
    // Evaluate children
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    // Error checking
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    // Empty expression
    if (v->count == 0) { return v; }

    // Single expression
    if (v->count == 1) { return lval_take(v, 0); }

    // Ensure first element is symbol
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f);
        lval_del(v);
        return lval_err("Expression does not start with symbol");
    }

    lval *result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_eval(lval *v) {
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

    return v;
}

lval* lval_pop(lval *v, int i) {
    // Get item at index i
    lval *x = v->cell[i];

    // Shift memory after the item at "i" over the top
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - i - 1));

    // Decrease count
    v->count--;

    // Reallocate memory used
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval *v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* builtin_op(lval *v, char* op) {
    // Make sure all arguments are numbers
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type != LVAL_NUM) {
            lval_del(v);
            return lval_err("Error, non-number in expression");
        }
    }

    // Pop first element
    lval *x = lval_pop(v, 0);

    // If no arguments and sub then perform unary negation
    if ((strcmp(op, "-") == 0 && v->count == 0)) {
        x->num = -x->num;
    }

    // While there are still elements remaining
    while (v->count > 0) {
        lval *y = lval_pop(v, 0);
        #define MAX(x, y) (((x) > (y)) ? (x) : (y))
        #define MIN(x, y) (((x) < (y)) ? (x) : (y))

        if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
            x->num += y->num;
        }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                return lval_err("Error: Divison by zero.");
            }
            x->num /= y->num;
        }
        if (strcmp(op, "%") == 0) {
            // Only allow modulo if both are "integers"
            double xInt, yInt;
            if (modf(x->num, &xInt) == 0 && modf(y->num, &yInt) == 0) {
                x->num = (int)x->num % (int)y->num;
            } else {
                return lval_err("Error: Non-integer modulo.");
            }
        }
        if (strcmp(op, "^") == 0) {
            x->num = (pow(x->num, y->num));
        }
        if (strcmp(op, "min") == 0) { return lval_num(MIN(x->num, y->num)); }
        if (strcmp(op, "max") == 0) { return lval_num(MAX(x->num, y->num)); }

        lval_del(y);
    }

    lval_del(v);
    return x;
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
lval* lval_num(double x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Error type lval
lval* lval_err(char* m) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

// Construct a new symbol lval
lval* lval_sym(char* s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

// Construct a new sexpr lval
lval* lval_sexpr() {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// Delete an lval
void lval_del(lval *v) {
    switch (v->type) {
        case LVAL_NUM:
            break;
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
        default:
            break;
    }
    free(v);
}

lval* lval_read_num(mpc_ast_t *t) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval *lval_read(mpc_ast_t *t) {
    // If symbol or number, return conversion to that type
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    // if root or sexpr then create an empty list
    lval *x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }

    // Fill this list with any valid expression contained within
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

// Add an lval to the end of this and update fields.
lval* lval_add(lval *v, lval *x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

void lval_expr_print(lval *v, char open, char close) {
    putchar(open);

    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);

        if (i != (v->count-1)) {
            printf(" ");
        }
    }
    putchar(close);
}
// Print an "lval"
void lval_print(lval *v) {
    switch(v->type) {
        case LVAL_NUM:
            printf("%g", v->num);
            break;
        case LVAL_ERR:
            printf("Error: %s", v->err);
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
        default:
            break;
    }
}

void lval_println(lval *v) {
    lval_print(v);
    printf("\n");
}

int main(int argc, char** argv) {

    // Create some parsers
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define them with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                      \
            number   : /-?[0-9]+(\\.[0-9])*/;                  \
            symbol : '+' | '-' | '*' | '/' | '%' | '^' |       \
                       /add/ | /sub/ | /mul/ | /div/ |         \
                       /max/ | /min/ ;                         \
            sexpr    : '(' <expr>* ')' ;                       \
            expr     : <number> | <symbol> | <sexpr> ;        \
            lispy    : /^/ <expr>* /$/ ;            \
        ",
        Number, Symbol, Sexpr, Expr, Lispy);

    printf("Lisp version 0.0.0.1\n");
    printf("Ctrl-C to exit\n");

    while(1) {
        char *input = readline("Lisp> ");

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            // On success print the result of the evaluation
            lval *x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
        } else {
            // Otherwise print the error
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        add_history(input);
        free(input);
    }

    // Undefine and delete our parsers
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}
