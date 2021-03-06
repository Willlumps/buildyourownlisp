#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"
#include "parsing.h"

// TODO: Make builtin_op less ugly


// Evalutes a passed S-Expression
// Returns the evaluated expression
lval* lval_eval_sexpr(lenv *e, lval *v) {
  // Evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  // Error checking
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  // Empty expression
  if (v->count == 0) { return v; }

  // Ensure first element is a function after evaluation
  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval *err = lval_err("S-Expression starts with incorrect type. "
     "Got %s, expected %s.",
     ltype_name(f->type), ltype_name(LVAL_FUN));
    lval_del(f);
    lval_del(v);
    return err;
  }

  lval *result = lval_call(e, f, v);
  lval_del(f);
  return result;
}


// Evaluate an expression
lval* lval_eval(lenv *e, lval *v) {
  if (v->type == LVAL_SYM) {
    lval *x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
  return v;
}

// Extracts a single element from an expression at index i and shifts the
// rest of the list backwards so it no longer contains that element
// Returns the extracted element
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
// Returns the extracted element.
lval* lval_take(lval *v, int i) {
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}

// Takes a single lval representing a list of all the arguments to operate on
// And performs said operation until it exhuasts the list of arguments or
// Encounters an error.
// Returns the evaluated expression.
lval* builtin_op(lenv *e, lval *v, char* op) {
  // Make sure all arguments are numbers
  for (int i = 0; i < v->count; i++) {
    LASSERT_NUM(op, v, i);
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
        x->type = LVAL_NUM_DOUBLE;
      }
      if (y->type == LVAL_NUM_LONG) {
        double y_num = y->num.num_long;
        y->num.num_double = y_num;
        y->type = LVAL_NUM_DOUBLE;
      }
      //LASSERT_TYPE(op, v, v->count, LVAL_NUM_LONG);
      LASSERT_TYPE(op, v, v->count, LVAL_NUM_DOUBLE);
      if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
        x->num.num_double += y->num.num_double;
      }
      if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
        x->num.num_double -= y->num.num_double;
      }
      if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
        x->num.num_double *= y->num.num_double;
      }
      if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
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
      if (strcmp(op, "pow") == 0) {
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
      if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
        x->num.num_long -= y->num.num_long;
      }
      if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
        x->num.num_long *= y->num.num_long;
      }
      if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
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
      if (strcmp(op, "pow") == 0) {
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

lval* builtin_add(lenv *e, lval *a) {
  return builtin_op(e, a, "+");
}

lval* builtin_add_full(lenv *e, lval *a) {
  return builtin_op(e, a, "add");
}

lval* builtin_sub(lenv *e, lval *a) {
  return builtin_op(e, a, "-");
}

lval* builtin_sub_full(lenv *e, lval *a) {
  return builtin_op(e, a, "sub");
}

lval* builtin_mul(lenv *e, lval *a) {
  return builtin_op(e, a, "*");
}

lval* builtin_mul_full(lenv *e, lval *a) {
  return builtin_op(e, a, "mul");
}

lval* builtin_div(lenv *e, lval *a) {
  return builtin_op(e, a, "/");
}

lval* builtin_div_full(lenv *e, lval *a) {
  return builtin_op(e, a, "div");
}

lval* builtin_pow(lenv *e, lval *a) {
  return builtin_op(e, a, "pow");
}

lval* builtin_mod(lenv *e, lval *a) {
  return builtin_op(e, a, "%");
}

lval* builtin_min(lenv *e, lval *a) {
  return builtin_op(e, a, "min");
}

lval* builtin_max(lenv *e, lval *a) {
  return builtin_op(e, a, "max");
}

lval* builtin_print(lenv *e, lval *a) {
  return lenv_print(e);
}

void builtin_exit(lenv *e, lval *a) {
  return exit(2);
}

lval* builtin_def(lenv *e, lval *a) {
  return builtin_var(e, a, "def");
}

lval* builtin_put(lenv *e, lval *a) {
  return builtin_var(e, a, "=");
}

// Builtin for lambda function
lval* builtin_lambda(lenv* e, lval *a) {
  // Check for two arguments, both should be of type Q-Expression
  LASSERT_NUM_ARGS("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  // Check that first expression only contains symbols
  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
      "Cannot define non-symbol. Got %s, expected %s.",
      ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
  }

  // Pop first two arguments and pass them to lval_lambda
  lval *formals = lval_pop(a, 0);
  lval *body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

// Register all of our builtins into an environment.
// Splits the task into lenv_add_builtin and lenv_add_builtins
void lenv_add_builtin(lenv *e, char* name, lbuiltin func) {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv *e) {
  // List functions
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "init", builtin_init);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "min", builtin_min);
  lenv_add_builtin(e, "max", builtin_max);
  lenv_add_builtin(e, "def", builtin_def);

  // Mathematical functions
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "pow", builtin_pow);
  lenv_add_builtin(e, "%", builtin_mod);
  lenv_add_builtin(e, "add", builtin_add_full);
  lenv_add_builtin(e, "sub", builtin_sub_full);
  lenv_add_builtin(e, "mul", builtin_mul_full);
  lenv_add_builtin(e, "div", builtin_div_full);
  lenv_add_builtin(e, "print", builtin_print);
  lenv_add_builtin(e, "exit", (lbuiltin)builtin_exit);

  // User functions
  lenv_add_builtin(e, "\\", builtin_lambda);
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "=", builtin_put);

}

// Constructs an lval that contains an integer data type
lval* lval_num_long(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM_LONG;
  v->num.num_long = x;
  return v;
}

// Constructs an lval that contains a double/float data type
lval* lval_num_double(double x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM_DOUBLE;
  v->num.num_double = x;
  return v;
}

// Constructs an lval for when an error has been encountered
lval* lval_err(char* fmt, ...) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  // Create a new va list and initialize
  va_list va;
  va_start(va, fmt);

  // Allocate 512 bytes of space
  v->num.err = malloc(512);

  // printf the error
  vsnprintf(v->num.err, 511, fmt, va);

  // Cleanup
  va_end(va);

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

// Construct a new function lval
lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

// Copies an lval and returns the copied version
lval* lval_copy(lval *v) {
  lval *x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
    case LVAL_NUM_LONG:
      x->num.num_long = v->num.num_long;
      break;
    case LVAL_NUM_DOUBLE:
      x->num.num_double = v->num.num_double;
      break;
    case LVAL_FUN:
      if (v->builtin) {
        x->builtin = v->builtin;
      } else {
        x->builtin = NULL;
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      }
      break;
    case LVAL_ERR:
      x->num.err = malloc(sizeof(strlen(v->num.err) + 1));
      strcpy(x->num.err, v->num.err);
      break;
    case LVAL_SYM:
      x->sym = malloc(sizeof(strlen(v->sym) + 1));
      strcpy(x->sym, v->sym);
      break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for (int i = 0; i < x->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }
      break;
  }
  return x;
}

// Creates a new environment
lenv* lenv_new(void) {
  lenv *e = malloc(sizeof(lenv));
  e->par = NULL;
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

// Delete an environment
void lenv_del(lenv *e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

// Copies an environment and returns the copy
lenv* lenv_copy(lenv *e) {
  lenv *a = malloc(sizeof(lenv));
  a->par = e->par;
  a->count = e->count;
  a->syms = malloc(sizeof(char*) * a->count);
  a->vals = malloc(sizeof(lval*) * a->count);
  for (int i = 0; i < e->count; i++) {
    a->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(a->syms[i], e->syms[i]);
    a->vals[i] = lval_copy(e->vals[i]);
  }

  return a;
}

// Retrieve a value from an environment
lval* lenv_get(lenv *e, lval *k) {
  // Iterate over all the items in the environment
  for (int i = 0; i < e->count; i++) {
    // Check if the stored string matches the symbol string
    // If a match is found, return a copy of the value
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }

  // If no symbol, check in parent
  if (e->par) {
    return lenv_get(e->par, k);
  }
  // Otherwise return an error.
  else {
    return lval_err("Unbound Symbol '%s'", k->sym);
  }
}

// Insert a variable into the environment
void lenv_put(lenv *e, lval *k, lval *v) {
  // Iterate over all the items in the environment
  for (int i = 0; i < e->count; i++) {
    // Check if the stored string matches the symbol string
    // If a match is found, replace the value by deleting that
    // element and inserting a new, updated value.
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  // If no value is found with said name, allocate some space and insert it
  // into the environment.
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(lenv*) * e->count);

  e->vals[e->count - 1] = lval_copy(v);
  e->syms[e->count - 1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count - 1], k->sym);
}

// Takes an lval as input and returns the first argument in the list, discarding
// the rest.
lval* builtin_head(lenv *e, lval *v) {
  // Check error conditions
  LASSERT_TYPE("head", v, 0, LVAL_QEXPR);
  LASSERT_EMPTY_ARGS("head", v, 0);
  LASSERT_NUM_ARGS("head", v, 1)

  // Otherwise take the first argument
  lval *a = lval_take(v, 0);
  while (a->count > 1) {
    lval_del(lval_pop(a, 1));
  }
  return a;
}

// Takes an lval as input and removes the first element, returning a list
// containing the rest.
lval* builtin_tail(lenv *e, lval *v) {
  LASSERT_TYPE("tail", v, 0, LVAL_QEXPR);
  LASSERT_EMPTY_ARGS("tail", v, 0);
  LASSERT_NUM_ARGS("tail", v, 1)

  lval *a = lval_take(v, 0);
  lval_del(lval_pop(a, 0));
  return a;
}

// Converts the input S-Expression into a Q-Expression
lval* builtin_list(lenv *e, lval *v) {
  v->type = LVAL_QEXPR;
  return v;
}

// Takes as input a single Q-Expression, which it converts into an
// S-Expression and evaluates using 'lval_eval'
lval* builtin_eval(lenv *e, lval *v) {
  LASSERT_TYPE("eval", v, 0, LVAL_QEXPR);
  LASSERT_NUM_ARGS("eval", v, 1)

  lval *a = lval_take(v, 0);
  a->type = LVAL_SEXPR;
  return lval_eval(e, a);
}

// Joins two Q-Expressions together
lval* builtin_join(lenv *e, lval *v) {
  for (int i = 0; i < v->count; i++) {
    LASSERT_TYPE("join", v, 0, LVAL_QEXPR);
  }

  lval *x = lval_pop(v, 0);

  while (v->count) {
    x = lval_join(x, lval_pop(v, 0));
  }

  lval_del(v);
  return x;
}

// Helper function for builtin_join()
lval* lval_join(lval *x, lval *y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }
  lval_del(y);
  return x;
}

// Returns an expression minus the last element
lval* builtin_init(lenv *e, lval *v) {
  LASSERT_TYPE("init", v, 0, LVAL_QEXPR);
  LASSERT_EMPTY_ARGS("init", v, 0);
  LASSERT_NUM_ARGS("init", v, 1)

  lval *x = lval_pop(v, 0);
  lval_del(lval_pop(x, x->count - 1));
  return x;
}

// Appends a value to the front of a Q-Expression
lval* builtin_cons(lenv *e, lval *v) {
  LASSERT_NUM("cons", v, 0);
  LASSERT_TYPE("cons", v, 1, LVAL_QEXPR);
  LASSERT_NUM_ARGS("cons", v, 2)

  lval *a = lval_pop(v, 0); // Value
  lval *b = lval_pop(v, 0); // Expression
  lval *c = lval_qexpr();
  c = lval_add(c, a);
  c = lval_join(c, b);
  lval_del(v);
  return c;
}

// Takes some enum and returns the string representation
char* ltype_name(int t) {
  switch(t) {
    case LVAL_ERR:
      return "Error";
    case LVAL_FUN:
      return "Function";
    case LVAL_NUM_DOUBLE:
      return "Double";
    case LVAL_NUM_LONG:
      return "Integer";
    case LVAL_QEXPR:
      return "Q-Expression";
    case LVAL_SEXPR:
      return "S-Expression";
    case LVAL_SYM:
      return "Symbol";
    default:
      return "Unknown";
  }
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
    case LVAL_FUN:
      if (!v->builtin) {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
      break;
    default:
      break;
  }
  free(v);
}

// Reads input from the AST and returns an lval of the correct data type
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

// Reads input from the AST and stores it in the correct lval
lval *lval_read(mpc_ast_t *t) {
  // If symbol or number, return conversion to that type
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  // if root or sexpr then create an empty list
  lval *x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

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

// Print an expression
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
      break;
    case LVAL_SYM:
      printf("%s", v->sym);
      break;
    case LVAL_SEXPR:
      lval_expr_print(v, '(', ')');
      break;
    case LVAL_QEXPR:
      lval_expr_print(v, '{', '}');
      break;
    case LVAL_FUN:
      if (v->builtin) {
        printf("<builtin>");
      } else {
        printf("(\\ ");
        lval_print(v->formals);
        putchar(' ');
        lval_print(v->body);
        putchar(')');
      }
      break;
    default:
      break;
  }
}

// Print a new line
void lval_println(lval *v) {
  lval_print(v);
  printf("\n");
}

// Calculate the number of leaves on an AST
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

// Calculate the number of branches on an AST
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

// Prints all elements in a given environment
lval* lenv_print(lenv *e) {
  for (int i = 0; i < e->count; i++) {
    printf("Key: %s\n", e->syms[i]);
  }
  return lval_sexpr();
}

// Constructor for user defined lval functions
lval* lval_lambda(lval *formals, lval *body) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUN;

  // Set builtin to null
  v->builtin = NULL;

  // Built new environment
  v->env = lenv_new();

  // Set formals and body
  v->formals = formals;
  v->body = body;

  return v;
}

// Defines a function in the global environment
void lenv_def(lenv *e, lval *k, lval *v) {
  // Iterate until e has no parent, we will call it batman
  while (e->par) {
    e = e->par;
  }
  // Put value in e
  lenv_put(e, k, v);
}

lval* builtin_var(lenv *e, lval *a, char* func) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

  lval *syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
      "Function '%s' cannot define non-symbol. "
      "Got %s, expected %s.",
      func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
  }

  LASSERT(a, (syms->count == a->count - 1),
    "Function '%s' passed too many arguments for symobls. "
    "Got %d, expected %d.",
    func, syms->count, a->count - 1);

  for (int i = 0; i < syms->count; i++) {
    // If 'def' define globally, if 'put' define locally
    if (strcmp(func, "def") == 0) {
      lenv_def(e, syms->cell[i], a->cell[i+1]);
    }

    if (strcmp(func, "=") == 0) {
      lenv_put(e, syms->cell[i], a->cell[i+1]);
    }
  }

  lval_del(a);
  return lval_sexpr();
}

// Call a function
lval* lval_call(lenv *e, lval *f, lval *a) {
  // If builtin then simply apply that
  if (f->builtin) {
    return f->builtin(e, a);
  }

  // Record argument counts
  int given = a->count;
  int total = f->formals->count;

  // While arguments remain to be processed
  while(a->count) {
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed too many arguments. "
        "Got %d, expected %d.", given, total);
    }
    // Pop first symbol
    lval *sym = lval_pop(f->formals, 0);

    // Special case to deal with '&'
    if (strcmp(sym->sym, "&") == 0) {
      // Ensure it is followed by another symobl
      if (f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid. "
          "Symbol '&' not followe by single symobl.");
      }

      // Next formal should be bound to remaining arguments
      lval *nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym);
      lval_del(nsym);
      break;
    }

    // Pop the next argument
    lval *val = lval_pop(a, 0);

    // Bind a copy into the function environment
    lenv_put(f->env, sym, val);

    // Delete syumobld and value
    lval_del(sym);
    lval_del(val);
  }

  // List is now bound, clean up
  lval_del(a);

  // If '&' remians in formal list bind to empty list
  if (f->formals->count > 0 &&
    strcmp(f->formals->cell[0]->sym, "&") == 0) {

    // Check to ensure that & is not passed invalidly
    if (f->formals->count != 2) {
      return lval_err("Function format invalid. "
        "Symbol '&' not followed by single symbol.");
    }

    // Pop and delete '&'
    lval_del(lval_pop(f->formals, 0));

    // Pop next symbol and create an empy list
    lval *sym = lval_pop(f->formals, 0);
    lval *val = lval_qexpr();

    // Bind and delete
    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  // If all formals havev been bound, evaluate
  if (f->formals->count == 0) {
    // Set environment parent to eval environment
    f->env->par = e;

    // Eval and return
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
  } else {
    // Otherwise return partially evaluated function
    return lval_copy(f);
  }
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
    "                                                         \
      number : /-?[0-9]+(\\.[0-9])*/;                         \
      symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;             \
      sexpr  : '(' <expr>* ')' ;                              \
      qexpr  : '{' <expr>* '}' ;                              \
      expr   : <number> | <symbol> | <sexpr> | <qexpr> ;      \
      lispy  : /^/ <expr>* /$/ ;                              \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  printf("Lisp version 0.0.0.1\n");
  printf("Type Ctrl-C or 'exit' to exit\n");

  lenv *e = lenv_new();
  lenv_add_builtins(e);

  while(1) {
    char *input = readline("Lisp> ");

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      // On success print the result of the evaluation
      lval *x = lval_eval(e, lval_read(r.output));
      lval_println(x);
      lval_del(x);
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
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  lenv_del(e);
  return 0;
}
