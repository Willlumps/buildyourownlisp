#ifndef parsing_h
#define parsing_h

#include "mpc.h"

// Forward Declarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

// Lisp Values
enum {
  LVAL_NUM_LONG,
  LVAL_NUM_DOUBLE,
  LVAL_ERR,
  LVAL_SYM,
  LVAL_FUN,
  LVAL_SEXPR,
  LVAL_QEXPR
};

typedef lval*(*lbuiltin)(lenv*, lval*);

// Value struct
struct lval {
  int type;
  union {
    long num_long;
    double num_double;
    char* err;
  } num;
  char* sym;
  lbuiltin fun;
  int count;
  struct lval **cell;
};

// Enviornment struct
struct lenv {
  int count;
  char** syms;
  lval** vals;
};

enum {
  LERR_DIV_ZERO,
  LERR_BAD_OP,
  LERR_BAD_NUM,
  LERR_MOD_FLOAT
};

// Builtin asserts
#define LASSERT(args, cond, fmt, ...)         \
  if (!(cond)) {                              \
    lval* err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args);                           \
    return err;                               \
  }

#define LASSERT_TYPE(func, args, index, expect)            \
  LASSERT(args, args->cell[index]->type == expect,         \
    "Function %s passeed incorrect type for argument %i. " \
    "Got %s, expected %s.",                                \
    func, index, ltype_name(args->cell[index]->type),      \
    ltype_name(expect))

#define LASSERT_NUM(func, args, index)                          \
  if(args->cell[index]->type != LVAL_NUM_LONG) {                \
    if(args->cell[index]->type != LVAL_NUM_DOUBLE) {            \
      LASSERT(args, args->cell[index]->type == LVAL_NUM_DOUBLE, \
        "Function %s passed incorrect type for argument %d. "   \
        "Got %s, expected Number.",                             \
        func, index, ltype_name(args->cell[index]->type)); }}    \

#define LASSERT_EMPTY_ARGS(func, args, index)               \
  LASSERT(args, args->cell[0]->count != 0,                  \
    "Function %s passed empty expression for argument %d.", \
    func, index);                                           \

#define LASSERT_NUM_ARGS(func, args, num)                \
  LASSERT(args, args->count == num,                      \
    "Function %s passed incorrect number of arguments. " \
    "Got %d, expected %d.",                              \
    func, args->count, num);                             \

lval* eval(mpc_ast_t *t);
lval* eval_op(lval x, char *op, lval y);
lval* lval_eval_sexpr(lenv *e, lval *v);
lval* lval_eval(lenv *e, lval *v);
lval* lval_num_long(long x);
lval* lval_num_double(double x);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
lval* lval_read_num(mpc_ast_t *t);
lval* lval_read(mpc_ast_t *t);
lval* lval_add(lval *v, lval *x);
lval* lval_pop(lval *v, int i);
lval* lval_take(lval *v, int i);
lval* lval_qexpr(void);
lval* builtin_op(lenv *e, lval* a, char* op);

lval* builtin_add(lenv *e, lval *a);
lval* builtin_sub(lenv *e, lval *a);
lval* builtin_multadd(lenv *e, lval *a);
lval* builtin_div(lenv *e, lval *a);
lval* builtin_pow(lenv *e, lval *a);
lval* builtin_mod(lenv *e, lval *a);

lval* builtin_list(lenv *e, lval *v);
lval* builtin_head(lenv *e, lval *v);
lval* builtin_tail(lenv *e, lval *v);
lval* builtin_eval(lenv *e, lval *v);
lval* builtin_join(lenv *e, lval *v);
lval* builtin_init(lenv *e, lval *v);
lval* builtin_cons(lenv *e, lval *v);
lval* builtin(lenv *e, lval *v, char* func);
lval* lval_join(lval *x, lval *y);
lval* lval_fun(lbuiltin func);
lval* lval_copy(lval *v);
void lval_expr_print(lval *v, char open, char close);
void lval_del(lval *v);
void lval_print(lval *v);
void lval_println(lval *v);

lenv* lenv_new(void);
lval* lenv_get(lenv *e, lval *k);
void lenv_put(lenv *e, lval *k, lval* v);
void lenv_del(lenv *e);
void lenv_add_builtin(lenv *e, char* name, lbuiltin func);
void lenv_add_builtins(lenv *e);

lval* builtin_def(lenv* e, lval* a);
char* ltype_name(int t);
int numLeaves(mpc_ast_t *t);
int numBranches(mpc_ast_t *t);

#endif
