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

//you can leave lval out of the first line and just use the alias
//except you want to reference the struct in its definition
// like on line +10
typedef struct lval { 
    int type;
    long num;

    char *err;
    char *sym;

    // list of pointers to 'lval *' and counter of lists 
    int count;
    struct lval ** cell;
} lval;

// enums for the int fields of out lval type
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

// was worried about the prime?exponential thing but since we 
// flag anything too large to be processes internally as a bad 
// num then we should be fine. This was a nice addition.

// enums for our possible error types: division by zero, 
// unknown operators or numbers bigger than `long`
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval *lval_num(long x);
lval *lval_err(char *m);
lval *lval_sym(char *s);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_add(lval *v, lval *x);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *lval_eval(lval *v);
lval *lval_eval_sexpr(lval *v);
lval *builtin(lval *a, char *func);
lval *builtin_op(lval* a, char* op);
lval *builtin_head(lval* a);
lval *builtin_tail(lval* a);
lval *builtin_list(lval *a);
lval *builtin_eval(lval *a);
lval *builtin_join(lval *a);
lval *lval_join(lval *x, lval *y);
void lval_delete(lval *v);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);


lval *lval_eval_sexpr(lval *v) {
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    if (v->count == 0) { return v; }

    if (v->count == 1) { return lval_take(v, 0); }

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_delete(f);
        lval_delete(v);
        return lval_err("S-expression Does not start with a symbol!");
    }

    lval *result = builtin(v, f->sym);
    lval_delete(f);
    return result;
}

lval *lval_eval(lval *v) {
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

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
                 | /list/ | /head/ | /tail/ | /join/ | /eval/ ;                                                \
        sexpr    : '(' <expr>* ')' ;                                                                           \
        qexpr    : '{' <expr>* '}';                                                                            \
        expr     : <number> | <symbol> | <sexpr> | <qexpr> ;                                                   \
        crisp    : /^/ <expr>* /$/ ;                                                                           \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Crisp);

    puts("Crisp Version 0.0.0.0.2\n");
    puts("Press Ctrl+C to Exit\n");

    while (1) {
        // init prompt and read input
        char *prompt = "crisp>>> \n";
        char *input = readline(prompt);
        add_history(input);

        // parse input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Crisp, &r)) {
            // ast
            // mpc_ast_t *a = r.output;
            // printf("Tag: %s\n", a->tag);
            // printf("Contents: %p\n", (void *) a->children);
            // printf("Number of children: %i\n", a->children_num);
            // printf("State: %", a->state[0]);
            // mpc_ast_print(r.output);
            
            // ast to s-expr
            // lval *x = lval_read(r.output);
            // lval_println(x);
            // lval_delete(x);

            lval *result = lval_eval(lval_read(r.output));
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

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Crisp);

    return 0;
}

lval *lval_num(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;

    return v;
}

lval *lval_err(char *m) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);

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
        lval_num(x) : lval_err("invalid number");
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

lval *builtin(lval *a, char *func) {
    if (strcmp("list", func) == 0) { return builtin_list(a); }
    if (strcmp("head", func) == 0) { return builtin_head(a); }
    if (strcmp("tail", func) == 0) { return builtin_tail(a); }
    if (strcmp("join", func) == 0) { return builtin_join(a); }
    if (strcmp("eval", func) == 0) { return builtin_eval(a); }
    if (strstr("+-/*^\%", func)) { return builtin_op(a, func); }
    if (strstr("addsubmulremexp", func)) { return builtin_op(a, func); }

    lval_delete(a);

    return lval_err("Unknown operation");
}

lval *builtin_op(lval *a, char *op) {
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_delete(a);
            return lval_err("Cannot operate on non-number!");
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
                x = lval_err("Division by Zero!");
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
#define LASSERT(args, cond, err) \
    if (!(cond)) { lval_delete(args); return lval_err(err); }

lval *builtin_head(lval* a) {
    LASSERT(a, a->count == 1, "Function 'head' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'head' passed incorrect type, expected Q expression!");
    LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed empty braces {}!");
    

    lval *v = lval_take(a, 0);
    while (v->count > 1) {
        lval_delete(lval_pop(v, 1));
    }
    return v;
}

lval *builtin_tail(lval* a) {
    LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed incorrect type, expected Q expression!");
    LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed empty braces {}!");

    lval *v = lval_take(a, 0);
    lval_delete(lval_pop(v, 0));
    return v;
}

lval *builtin_list(lval *a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lval *a) {
    LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type, expected Q expression!");

    // todo: why are we calling lval_take here?
    // take calls pops for the last element in a->cell
    // pop copies vals from cell[0] to cell[0+1] and resizes a->cell to a->cell-1
    // a->cell = [{1 2 3}] wont this be empty if we pop?
    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(x);
}

lval *builtin_join(lval *a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type, expected Q expression!");
    }

    lval *x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_delete(a);
    return x;
}

lval *lval_join(lval *x, lval *y) {
    // squeeze out vals from y->cell and add to x
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_delete(y);
    return x;
}

void lval_delete(lval *v) {
    switch (v->type) {
        case LVAL_NUM: 
            break;
        case LVAL_SYM: 
            free(v->sym); 
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
