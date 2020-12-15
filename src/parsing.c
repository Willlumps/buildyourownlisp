#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"
#include "parsing.h"

// TODO: Make builtin_op less ugly

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

    lval *result = builtin(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_eval(lval *v) {
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

    return v;
}

// Extracts a single element from an expression at index i and shifts the
// rest of the list backwards so it no longer contains that lval
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

// Similar to lval_pop() but it deletes the list it has extracted the element
// from.
lval* lval_take(lval *v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* builtin_op(lval *v, char* op) {
    // Make sure all arguments are numbers
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            lval_del(v);
            return lval_err("Error, non-number in expression");
        }
    }

    // Pop first element
    lval *x = lval_pop(v, 0);

    // If no arguments and sub then perform unary negation
    if ((strcmp(op, "-") == 0 && v->count == 0)) {
        if (x->type == LVAL_NUM_LONG) {
            x->num.num_long = -x->num.num_long;
        } else {
            x->num.num_double = -x->num.num_double;
        }
    }

    // While there are still elements remaining
    while (v->count > 0) {
        lval *y = lval_pop(v, 0);
        #define MAX(x, y) (((x) > (y)) ? (x) : (y))
        #define MIN(x, y) (((x) < (y)) ? (x) : (y))

        // If one of the numbers is a double, do double stuff
        if (x->type == LVAL_NUM_DOUBLE || y->type == LVAL_NUM_DOUBLE) {
            // Cast them to double if they are a long
            if (x->type == LVAL_NUM_LONG) {
                double x_num = x->num.num_long;
                x->num.num_double = x_num;
            }
            if (y->type == LVAL_NUM_LONG) {
                double y_num = y->num.num_long;
                y->num.num_double = y_num;
            }
            if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
                x->num.num_double += y->num.num_double;
            }
            if (strcmp(op, "-") == 0) { x->num.num_double -= y->num.num_double; }
            if (strcmp(op, "*") == 0) { x->num.num_double *= y->num.num_double; }
            if (strcmp(op, "/") == 0) {
                if (y->num.num_double == 0) {
                    lval_del(x);
                    lval_del(y);
                    return lval_err("Error: Divison by zero.");
                }
                x->num.num_double /= y->num.num_double;
            }
            if (strcmp(op, "%") == 0) {
                return lval_err("Error: Non-integer modulo.");
            }
            if (strcmp(op, "^") == 0) {
                x->num.num_double = (pow(x->num.num_double, y->num.num_double));
            }
            if (strcmp(op, "min") == 0) {
                return lval_num_double(MIN(x->num.num_double, y->num.num_double));
            }
            if (strcmp(op, "max") == 0) {
                return lval_num_double(MAX(x->num.num_double, y->num.num_double));
            }
        }
        // If neither are a double, they must be of type long
        else {

            if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
                x->num.num_long += y->num.num_long;
            }
            if (strcmp(op, "-") == 0) { x->num.num_long -= y->num.num_long; }
            if (strcmp(op, "*") == 0) { x->num.num_long *= y->num.num_long; }
            if (strcmp(op, "/") == 0) {
                if (y->num.num_long == 0) {
                    lval_del(x);
                    lval_del(y);
                    return lval_err("Error: Divison by zero.");
                }
                x->num.num_long /= y->num.num_long;
            }
            if (strcmp(op, "%") == 0) {
                x->num.num_long = x->num.num_long % y->num.num_long;
            }
            if (strcmp(op, "^") == 0) {
                x->num.num_long = (pow(x->num.num_long, y->num.num_long));
            }
            if (strcmp(op, "min") == 0) {
                x->num.num_long = MIN(x->num.num_long, y->num.num_long);
            }
            if (strcmp(op, "max") == 0) {
                x->num.num_long = MAX(x->num.num_long, y->num.num_long);
            }
        }

        lval_del(y);
    }

    lval_del(v);
    return x;
}


// Long type lval
lval* lval_num_long(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM_LONG;
    v->num.num_long = x;
    return v;
}

// Double type lval
lval* lval_num_double(double x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM_DOUBLE;
    v->num.num_double = x;
    return v;
}

// Error type lval
lval* lval_err(char* m) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->num.err = malloc(strlen(m) + 1);
    strcpy(v->num.err, m);
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

// Construct a new qexpr lval
lval* lval_qexpr() {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

#define LASSERT(args, cond, err) \
    if (!(cond)) { lval_del(args); return lval_err(err); }

lval* builtin_head(lval *v) {
    // Check error conditions
    LASSERT(v, v->count == 1,
        "Function 'head' passed too many arguments");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
        "Function 'head' passed incorrect type");
    LASSERT(v, v->cell[0]->count != 0,
        "Function 'head' passed empty expression");

    // Otherwise take the first argument
    lval *a = lval_take(v, 0);
    while (a->count > 1) {
        lval_del(lval_pop(a, 1));
    }
    return a;
}

lval* builtin_tail(lval *v) {
    LASSERT(v, v->count == 1,
        "Function 'tail' passed too many arguments");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
        "Function 'tail' passed incorrect type");
    LASSERT(v, v->cell[0]->count != 0,
        "Function 'tail' passed empty expression");

    lval *a = lval_take(v, 0);
    lval_del(lval_pop(a, 0));
    return a;
}

// Converts the input S-Expression into a Q-Expression
lval* builtin_list(lval *v) {
    v->type = LVAL_QEXPR;
    return v;
}

// Takes as input a single Q-Expression, which it converts into an
// S-Expression and evaluates using 'lval_eval'
lval* builtin_eval(lval *v) {
    LASSERT(v, v->count == 1,
        "Function 'eval' passed too many arguments");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
        "Function 'eval' passed incorrect type");

    lval *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(a);
}

lval* builtin_join(lval *v) {
    for (int i = 0; i < v->count; i++) {
    LASSERT(v, v->cell[i]->type == LVAL_QEXPR,
        "Function 'join' passed incorrect type");
    }

    lval *x = lval_pop(v, 0);

    while (v->count) {
        x = lval_join(x, lval_pop(v, 0));
    }

    lval_del(v);
    return x;
}

lval* lval_join(lval *x, lval *y) {
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
    return x;
}

// Returns an expression minus the last element
lval* builtin_init(lval *v) {
    LASSERT(v, v->count == 1,
        "Function 'tail' passed too many arguments");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
        "Function 'tail' passed incorrect type");
    LASSERT(v, v->cell[0]->count != 0,
        "Function 'tail' passed empty expression");

    lval *x = lval_pop(v, 0);
    lval_del(lval_pop(x, x->count - 1));
    return x;
}

// Appends a value to the front of a Q-Expression
lval* builtin_cons(lval *v) {
    lval *a = lval_pop(v, 0); // Value
    lval *b = lval_pop(v, 0); // Expression
    lval *c = lval_qexpr();
    c = lval_add(c, a);
    c = lval_join(c, b);
    lval_del(v);
    return c;
}

lval* builtin(lval *v, char* func) {
    if (strcmp("list", func) == 0) { return builtin_list(v); }
    if (strcmp("head", func) == 0) { return builtin_head(v); }
    if (strcmp("tail", func) == 0) { return builtin_tail(v); }
    if (strcmp("join", func) == 0) { return builtin_join(v); }
    if (strcmp("eval", func) == 0) { return builtin_eval(v); }
    if (strcmp("init", func) == 0) { return builtin_init(v); }
    if (strcmp("cons", func) == 0) { return builtin_cons(v); }
    if (strstr("+-/*^%", func)) { return builtin_op(v, func); }
    lval_del(v);
    return lval_err("Unknown Function");
}

// Delete an lval
void lval_del(lval *v) {
    switch (v->type) {
        case LVAL_NUM_LONG:
        case LVAL_NUM_DOUBLE:
            break;
        case LVAL_ERR:
            free(v->num.err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_QEXPR:
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
    if (strstr(t->contents, ".")) {
        double x = strtod(t->contents, NULL);
        return errno != ERANGE ? lval_num_double(x) : lval_err("invalid number");
    } else {
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num_long(x) : lval_err("invalid number");
    }

}

lval *lval_read(mpc_ast_t *t) {
    // If symbol or number, return conversion to that type
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    // if root or sexpr then create an empty list
    lval *x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

    // Fill this list with any valid expression contained within
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
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
        case LVAL_NUM_LONG:
            printf("%li", v->num.num_long);
            break;
        case LVAL_NUM_DOUBLE:
            printf("%g", v->num.num_double);
            break;
        case LVAL_ERR:
            printf("Error: %s", v->num.err);
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
        default:
            break;
    }
}

void lval_println(lval *v) {
    lval_print(v);
    printf("\n");
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
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");   // Symbolic Expression
    mpc_parser_t *Qexpr = mpc_new("qexpr");   // Quoted Expression (Macros)
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define them with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                            \
            number   : /-?[0-9]+(\\.[0-9])*/;                        \
            symbol : '+' | '-' | '*' | '/' | '%' | '^' |             \
                       \"add\" | \"sub\" | \"mul\" | \"div\" |       \
                       \"max\" | \"min\" |                           \
                       \"list\" | \"head\" | \"tail\" | \"join\" |   \
                       \"eval\" | \"cons\" | \"len\" | \"init\" ;    \
            sexpr    : '(' <expr>* ')' ;                             \
            qexpr    : '{' <expr>* '}' ;                             \
            expr     : <number> | <symbol> | <sexpr> | <qexpr> ;     \
            lispy    : /^/ <expr>* /$/ ;                             \
        ",
        Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

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
            //mpc_ast_delete(r.output);
        } else {
            // Otherwise print the error
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        add_history(input);
        free(input);
    }

    // Undefine and delete our parsers
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}
