#ifndef parsing_h
#define parsing_h

#include "mpc.h"

typedef struct {
    int type;
    double num;
    int err;
} lval;

enum {
    LVAL_NUM,
    LVAL_ERR
};

enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM,
    LERR_MOD_FLOAT
};

lval eval(mpc_ast_t *t);
lval eval_op(lval x, char *op, lval y);
int numLeaves(mpc_ast_t *t);
int numBranches(mpc_ast_t *t);
lval lval_num(double x);
lval lval_err(int x);
void lval_print(lval v);
void lval_println(lval v);

#endif
