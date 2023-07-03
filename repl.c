#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

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

void add_history(char *unused) {}

// these things are called preprocessors
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

// this is called a function pointer
typedef lval *(* lbuiltin)(lenv *, lval *);

//you can leave lval out of the first line and just use the alias
//except you want to reference the struct in its definition
// like on line +10
typedef struct lval { 
    int type;
    long num;

    char *err;
    char *sym;

    lbuiltin func;
    lenv *env;
    lval *formals;
    lval *body;

    // list of pointers to 'lval *' and counter of lists 
    int count;
    struct lval ** cell;
} lval;


typedef struct lenv {
    lenv *par;
    int count;
    char **syms;
    lval **vals;
} lenv;

// enums for the int fields of out lval type
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUNC, LVAL_SEXPR, LVAL_QEXPR };

char *ltype_name(int t) {
    switch (t) {
    case LVAL_FUNC: return "Function";
    case LVAL_ERR: return "Error";
    case LVAL_NUM: return "Number";
    case LVAL_SYM: return "Symbol";
    case LVAL_QEXPR: return "Q Expression";
    case LVAL_SEXPR: return "S Expression";
    default: return "Unknown";
    }
}

// was worried about the prime?exponential thing but since we 
// flag anything too large to be processes internally as a bad 
// num then we should be fine. This was a nice addition.

// enums for our possible error types: division by zero, 
// unknown operators or numbers bigger than `long`
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval *lenv_get(lenv *e, lval *k);
lenv *lenv_new(void);
lenv *lenv_copy(lenv *e);
void lenv_add_builtin(lenv *e, char *name, lbuiltin func);
void lenv_add_builtins(lenv *e);
void lenv_put(lenv *e, lval *k, lval *v);
void lenv_def(lenv *e, lval *k, lval *v);
void lenv_delete(lenv *v);
lval *lval_num(long x);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *s);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
lval *lval_call(lenv *e, lval *v, lval *k);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_add(lval *v, lval *x);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *lval_eval(lenv *e, lval *v);
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *builtin(lenv *e, lval *a, char *func);
lval *builtin_op(lenv *e, lval* a, char* op);
lval *builtin_head(lenv *e, lval* a);
lval *builtin_tail(lenv *e, lval* a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_def(lenv *e, lval *a);
lval *builtin_var(lenv *e, lval *v, char *func);
lval *builtin_lambda(lenv *e, lval *v);
lval *builtin_put(lenv *e, lval *a);
lval *lval_join(lval *x, lval *y);
lval *lval_fun(lbuiltin func);
lval *lval_copy(lval *v);
lval *lval_constructor(lval *formals, lval *body);
void lval_delete(lval *v);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);


lval *lval_eval_sexpr(lenv *e, lval *v) {
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    if (v->count == 0) { return v; }
    if (v->count == 1) { return lval_eval(e, lval_take(v, 0)); }

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUNC) {
        lval *err = lval_err(
            "S-Expression starts with incorrect type." \
            "Got %s, Expected %s.", ltype_name(f->type), ltype_name(LVAL_FUNC));
        lval_delete(f);
        lval_delete(v);
        // return lval_err("S-expression Does not start with a symbol!");
        return err;
    }

    lval *result = lval_call(e, f, v);
    lval_delete(f);
    return result;
}

lval *lval_eval(lenv *e, lval *v) {
    if (v->type == LVAL_SYM) {
        lval *x = lenv_get(e, v);
        lval_delete(v);
        return x;
    }

    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    return v;
}
 
int count_leaves(mpc_ast_t *t) {
    int count = 0;

    if (strstr(t->tag, "number")) {
        count++;
    }
    
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        count_leaves(t->children[i]);
        i++;
    }
    return 0;
}

int main(int argc, char** argv) {

    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Qexpr = mpc_new("qexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Crisp = mpc_new("crisp");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                                                          \
        number   : /-?[0-9]+/ ;                                                                                \
        symbol   : '+' | '-' | '*' | '/' | '%' | '^' | /add/ | /sub/ | /mul/ | /div/ | /rem/ | /exp/           \
                 | /list/ | /head/ | /tail/ | /join/ | /eval/ | /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;             \
        sexpr    : '(' <expr>* ')' ;                                                                           \
        qexpr    : '{' <expr>* '}';                                                                            \
        expr     : <number> | <symbol> | <sexpr> | <qexpr> ;                                                   \
        crisp    : /^/ <expr>* /$/ ;                                                                           \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Crisp);

    puts("Crisp Version 0.0.0.0.2\n");
    puts("Press Ctrl+C to Exit\n");

    lenv *e = lenv_new();
    lenv_add_builtins(e);

    while (1) {
        // init prompt and read input
        char *prompt = "crisp>>> \n";
        char *input = readline(prompt);
        add_history(input);

        // parse input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Crisp, &r)) {
            // ast
            mpc_ast_t *a = r.output;
            printf("Tag: %s\n", a->tag);
            printf("Contents: %p\n", (void *) a->children);
            printf("Number of children: %i\n", a->children_num);
            // printf("State: %s", a->state[0]);
            mpc_ast_print(r.output);
            
            // ast to s-expr
            // lval *x = lval_read(r.output);
            // lval_println(x);
            // lval_delete(x);

            lval *result = lval_eval(e, lval_read(r.output));
            lval_println(result);
            lval_delete(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        // printf("I do not understand: %s\n", input);
        free(input);
    }

    lenv_delete(e);

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Crisp);

    return 0;
}

lval *lval_num(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;

    return v;
}

lval *lval_err(char *fmt, ...) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    // v->err = malloc(strlen(fmt) + 1);

    // create list va and initialize
    va_list va;
    va_start(va, fmt);

    // allocate 512 bytes of space
    v->err = malloc(512);

    // printf err of max 512 chars 
    vsnprintf(v->err, 511, fmt, va);
    
    // reallocate to num of bytes used
    v->err = realloc(v->err, strlen(v->err)+1);

    // cleanup
    va_end(va);

    return v;
}

lval *lval_sym(char *s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    // why do we do this instead of just assigning
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);

    return v;
}

lval *lval_sexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;

    return v;
}

lval *lval_qexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;

    return v;
}

lval *lval_read_num(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? 
        lval_num(x) : lval_err("invalid number '%s'", t->contents);
}

lval *lval_read(mpc_ast_t *t) {
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    lval *x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

lval *lval_call(lenv *e, lval *v, lval *k) {
    if (v->func) { return v->func(e, k); }

    int given = k->count;
    int total = v->formals->count;

    while (k->count) {
        if (v->formals->count == 0) {
            lval_delete(k);
            return lval_err(
                "Function passed too many arguments."
                "Got %i, Expected %i.", given, total);
        }

        lval *sym = lval_pop(v->formals, 0);

        if (strcmp(sym->sym, "&") == 0) {
            if (v->formals->count != 1) {
                lval_delete(k);
                return lval_err("Function format invalid. "
                "Symbol '&' not followed by single symbol.");
            }

            lval *nsym = lval_pop(v->formals, 0);
            lenv_put(v->env, nsym, builtin_list(e, k));
            lval_delete(sym);
            lval_delete(nsym);
            break;
        }

        lval *val = lval_pop(k, 0);

        lenv_put(v->env, sym, val);
        lval_delete(sym);
        lval_delete(val);
    }

    lval_delete(k);

    if (v->formals->count > 0 && 
        strcmp(v->formals->cell[0]->sym, "&") == 0) {
            if (v->formals->count != 2) {
                return lval_err("Function format invalid"
                "Symbol '&' not followed by a single symbol.");
            }

            lval_delete(lval_pop(v->formals, 0));

            lval *sym = lval_pop(v->formals, 0);
            lval *val = lval_qexpr();

            lenv_put(v->env, sym, val);
            lval_delete(sym);
            lval_delete(val);
    } 

    if (v->formals->count == 0) {
        v->env->par = e;
        return builtin_eval(v->env, lval_add(lval_sexpr(), lval_copy(v->body)));
    } else {
        return lval_copy(v);
    }
}

lval *lval_add(lval *v, lval *x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count-1] = x;

    return v;
}

lval *lval_pop(lval *v, int i) {
    lval *x = v->cell[i];

    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval *) * (v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);

    return x;
}

lval *lval_take(lval *v, int i) {
    lval *x = lval_pop(v, i);
    lval_delete(v);
    return x;
}

lval *builtin(lenv *e, lval *a, char *func) {
    if (strcmp("list", func) == 0) { return builtin_list(e, a); }
    if (strcmp("head", func) == 0) { return builtin_head(e, a); }
    if (strcmp("tail", func) == 0) { return builtin_tail(e, a); }
    if (strcmp("join", func) == 0) { return builtin_join(e, a); }
    if (strcmp("eval", func) == 0) { return builtin_eval(e, a); }
    if (strstr("+-/*^\%", func)) { return builtin_op(e, a, func); }
    if (strstr("addsubmulremexp", func)) { return builtin_op(e, a, func); }

    lval_delete(a);

    return lval_err("Unknown operation '%s'", func);
}

lval *builtin_op(lenv *e, lval *a, char *op) {
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_delete(a);
            return lval_err("Cannot operate on non-number '%s'", a->cell[i]->sym);
        }
    }

    lval *x = lval_pop(a, 0);

    if (strcmp(op, "-") == 0 && a->count == 0) {
        x->num = -x->num;
    }

    while (a->count > 0) {
        lval *y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0 || (strcmp(op, "add") == 0)) { x->num += y->num; }
        if (strcmp(op, "-") == 0 || (strcmp(op, "sub") == 0)) { x->num -= y->num; }
        if (strcmp(op, "*") == 0 || (strcmp(op, "mul") == 0)) { x->num *= y->num; }
        if (strcmp(op, "%") == 0 || (strcmp(op, "rem") == 0)) { x->num %= y->num; }
        if (strcmp(op, "^") == 0 || (strcmp(op, "exp") == 0)) { 
            if (y->num == 0) { x->num = 1; }
            if (y->num == 1) { x->num; }
            for (int i = 1; i < y->num ; i++) {
                x->num *= x->num;
            }
            x->num; 
        }
        if (strcmp(op, "/") == 0 || (strcmp(op, "div") == 0)) { 
            if (y->num == 0) {
                lval_delete(x);
                lval_delete(y);
                x = lval_err("Cannot divide by Zero!");
                break;
            }
            x->num /= y ->num;
        }

        lval_delete(y);
        }

    lval_delete(a);
    return x;
}

//macros or preprocessors are like functions that generate some code
// before compile time. makes code easier to read?
// todo: why are these necessary? seem like functions alts.
#define LASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        lval *err = lval_err(fmt, ##__VA_ARGS__); \
        lval_delete(args); \
        return err; \
    }


#define LASSERT_TYPE(syms, args, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i." \
    "Got %i, Expected %i.", syms, index, ltype_name(args->cell[index]->type), ltype_name(expect));

#define LASSERT_NUM(syms, args, num) \
    LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments."\
    "Got %i, Expected %i.", syms, args->count, num);

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);


// todo: preprocessor that takes operation
lval *builtin_add(lenv *e, lval *a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function '+' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e, lval *a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function '-' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    return builtin_op(e, a, "-");
}

lval *builtin_mul(lenv *e, lval *a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function '*' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    return builtin_op(e, a, "*");
}

lval *builtin_div(lenv *e, lval *a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function '/' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    return builtin_op(e, a, "/");
}

lval *builtin_exp(lenv *e, lval *a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function '^' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    return builtin_op(e, a, "^");
}

lval *builtin_mod(lenv *e, lval *a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function '\%' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    return builtin_op(e, a, "%");
}


lval *builtin_head(lenv *e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed empty braces {}!", "Got %i, Expected %i", a->cell[0]->count, 0);
    

    lval *v = lval_take(a, 0);
    while (v->count > 1) {
        lval_delete(lval_pop(v, 1));
    }
    return v;
}

lval *builtin_tail(lenv *e, lval* a) {
    LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!", "Got %i, Expected %i", a->count, 1);
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed empty braces {}!", "Got %i, Expected %i", a->cell[0]->count, 0);

    lval *v = lval_take(a, 0);
    lval_delete(lval_pop(v, 0));
    return v;
}

lval *builtin_list(lenv *e, lval *a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lenv *e, lval *a) {
    LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments!", "Got %i, Expected %i", a->count, 1);
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

    // todo: why are we calling lval_take here?
    // take calls pops for the last element in a->cell
    // pop copies vals from cell[0] to cell[0+1] and resizes a->cell to a->cell-1
    // a->cell = [{1 2 3}] wont this be empty if we pop?
    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval *builtin_join(lenv *e, lval *a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type.", "Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
    }

    lval *x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_delete(a);
    return x;
}

lval *builtin_var(lenv *e, lval *v, char *func) {
    LASSERT_TYPE(func, v, 0, LVAL_QEXPR);

    lval *syms = v->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(v, (syms->cell[i]->type == LVAL_SYM), \
        "Function '%s' cannot define non-symbol. " \
        "Got %s, Expected %s.", func, 
        ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    LASSERT(v, (syms->count == v->count-1), \
    "Function '%s' passed too many arguments for symbols. " \
    "Got %i, Expected %i", func, 
    syms->count, v->count-1);

    for (int i = 0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], v->cell[i+1]);
        }
        if (strcmp(func, "=") == 0) {
            lenv_put(e, syms->cell[i], v->cell[i+1]);
        }
    }

    lval_delete(v);
    return lval_sexpr();
}

lval *builtin_lambda(lenv *e, lval *v) {
    LASSERT_NUM("\\", v, 2);
    LASSERT_TYPE("\\", v, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", v, 1, LVAL_QEXPR);

    for (int i = 0; i < v->cell[0]->count; i++) {
        LASSERT(v, (v->cell[0]->cell[i]->type == LVAL_SYM),
        "Cannot define non-symbol. Got %s expected %s.",
        ltype_name(v->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }

    lval *formals = lval_pop(v, 0);
    lval *body = lval_pop(v, 0);
    lval_delete(v);

    return lval_constructor(formals, body);
}


lval *builtin_def(lenv *e, lval *a) {
    return builtin_var(e, a, "def");
}

lval *builtin_put(lenv *e, lval *a) {
    return builtin_var(e, a, "=");
}


lval *lval_join(lval *x, lval *y) {
    // squeeze out vals from y->cell and add to x
    for (int i = 0; i < y->count; i++) {
        x = lval_add(x, lval_pop(y, 0));
    }
    free(y->cell);
    free(y);
    // while (y->count) {
    // }

    // lval_delete(y);
    return x;
}

lval *lval_fun(lbuiltin func) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUNC;
    v->func = func;
    return v;
}

lval *lenv_get(lenv *e, lval *k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    if (e->par) {
        return lenv_get(e->par, k);
    } else {
        return lval_err("unbound symbol: '%s'", k->sym);
    }
}

lenv *lenv_new(void) {
    lenv *e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

lenv *lenv_copy(lenv *e) {
    lenv *n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char *) * n->count);
    n->vals = malloc(sizeof(lval *) * n->count);

    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
    lval *k = lval_sym(name);
    lval *v = lval_fun(func);
    lenv_put(e, k, v);
    lval_delete(k);
    lval_delete(v);
}

void lenv_add_builtins(lenv *e) {
    // list operations
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    // num operations
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "^", builtin_exp);
    lenv_add_builtin(e, "%", builtin_mod);

    // variable functions
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);

    // userdefined functions
    lenv_add_builtin(e, "\\", builtin_lambda);
}   

void lenv_put(lenv *e, lval *k, lval *v) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_delete(e->vals[i]);
            e->vals[i] = lval_copy(v);
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval *) * e->count);
    e->syms = realloc(e->syms, sizeof(char *) * e->count);

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count -1], k->sym);
}

void lenv_def(lenv *e, lval *k, lval *v) {
    while (e->par) {
        e = e->par;
    }

    lenv_put(e, k, v);
}

void lenv_delete(lenv *e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_delete(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e); // todo: why not just free(e) instead of everything else?
}

void lval_delete(lval *v) {
    switch (v->type) {
        case LVAL_NUM: 
            break;
        case LVAL_SYM: 
            free(v->sym); 
            break;
        case LVAL_FUNC:
            if (!v->func) {
                lenv_delete(v->env);
                lval_delete(v->formals);
                lval_delete(v->body);
            }
            break;
        case LVAL_ERR: 
            free(v->err); 
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_delete(v->cell[i]);    
            }
            free(v->cell);
        break;
    }

    free(v);
}

void lval_expr_print(lval *v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);

        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

// Q: if i do not assign enums to values do they act as numbers?
// like 0, 1, 2, 3, ..., 9, enum_val, how does this work? 
// how does the err case work?
void lval_print(lval *v) {
    switch (v->type) {
        case LVAL_NUM:
            printf("%li", v->num);
            break;

        case LVAL_ERR:
            printf("Error: %s", v->err);
            break;
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_FUNC:
            if (v->func) {
                printf("<inbuilt function>");
            } else {
                printf("(\\ "); lval_print(v->formals);
                putchar(' '); lval_print(v->body); putchar(')');
            }
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
    }
}

void lval_println(lval *v) {
    lval_print(v);
    putchar('\n');
}

lval *lval_copy(lval *v) {

    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        case LVAL_FUNC:
            if (v->func) {
                x->func = v->func; 
            } else {
                x->func = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_NUM: 
            x->num = v->num;
            break;
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->err) + 1);
            strcpy(x->sym, v->sym);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval *) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]); 
            }
        break;
    }
    return x;
}

lval *lval_constructor(lval *formals, lval *body) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUNC;

    // we want user defined functions to be set to NULL so we can
    // tell them apart from inbuilt ones
    v->func = NULL;
    v->env = lenv_new();

    v->formals = formals;
    v->body = body;

    return v;
}

// ch6. parsing
// [a]+ matches "a"s in aababa, [b]+

// ab matches consecutive "ab"s or (ab) 

// [.p]+o?i?[t] matches pot, pit, respite but also matches spit

// tried <add> "add" none worked
// to add string eqv operator: /add/ | \'mul\' | \'sub\' | \'div\'

// -?[0-9]+.?[0-9]* this works for decimals
// todo: improve lookahead to include .4 -> -?([0-9]+)?.[0-9]*

// to accept operations like so 1 + 1 or 1 + (5 - 1):
    // expr: <number> | '(' <expr> <operator> <expr> ')' ; 
    // crisp: /^/ <expr> <operator> <expr> /$/ ; 

// qf. I do not even care enough to understand the doge syntax
// ++ it looks regexed already ^<phrase>*$

// ch7. evaluation

// ch8. error handling
// can't. only using one codebase

// give enum a name by prepending the curly braces with a name
// enum MAGICBEANS { JACK, OLDMAN, CAPITALISM };

// unions work sorta like structs but only let you use one field at a time.
// all fields in a union share the same amout of memory, whatever the largest field is
// and only one field can be accessed at a time. 
// union foo {
    // int a, b, c;
    // float d, e, f;
    // char g, h, i;
// };

// we would be able to use this if we did not need the `num` field
// typedef union { 
//     int type;
////     long num;
//     int err;
// } lval_union;

// tried changing instances of the operand from long to double and type casting 
// our modulus operands from double to int. does not work. Q: Ask

// ch9. S-expressions
// open and close in lval_expr_print(); live on the stack;

// llval *result in main() points to some space in global memory.

// strcpy copies strings from a src to a dest

// realloc reallocates the size of an object pointed to by a pointer. 
// it acc deallocates and points to a bigger heap

// memmove copies n chars from one src to a dest and is best when working 
// with overlapping memory blocks. else comparable to memcpy. 
// See: https://stackoverflow.com/questions/4415910/memcpy-vs-memmove

// todo: Q. double decimal

// ch10. Q-expression
// add rule for the syntax, add new layer (or type or sugar) above core language,
// parse from ast, add functionality for new feature


// ch11. Variables