#ifndef parsing_h
#define parsing_h

#include "mpc.h"

typedef struct lval {
    int type;
    union {
        long num_long;
        double num_double;
        char* err;
    } num;
    //char* err;
    char* sym;
    int count;
    struct lval **cell;
} lval;

enum {
    LVAL_NUM_LONG,
    LVAL_NUM_DOUBLE,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR
};

enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM,
    LERR_MOD_FLOAT
};

lval* eval(mpc_ast_t *t);
lval* eval_op(lval x, char *op, lval y);
lval* lval_eval_sexpr(lval *v);
lval* lval_eval(lval *v);
lval* lval_num_long(long x);
lval* lval_num_double(double x);
lval* lval_err(char* x);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
lval* lval_read_num(mpc_ast_t *t);
lval* lval_read(mpc_ast_t *t);
lval* lval_add(lval *v, lval *x);
lval* lval_pop(lval *v, int i);
lval* lval_take(lval *v, int i);
lval* builtin_op(lval* a, char* op);

lval* lval_qexpr(void);
lval* builtin_head(lval *v);
lval* builtin_tail(lval *v);
lval* builtin_list(lval *v);
lval* builtin_eval(lval *v);
lval* builtin_join(lval *v);
lval* lval_join(lval *x, lval *y);
lval* builtin_init(lval *v);
lval* builtin_cons(lval *v);

lval *builtin(lval *v, char* func);
void lval_expr_print(lval *v, char open, char close);
void lval_del(lval *v);
void lval_print(lval *v);
void lval_println(lval *v);
int numLeaves(mpc_ast_t *t);
int numBranches(mpc_ast_t *t);

#endif
