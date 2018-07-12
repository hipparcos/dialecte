#include "lval.h"

#include <math.h>

/* Create a new number type lval. */
struct lval lval_num(long x) {
    struct lval v;
    v.type = LVAL_NUM;
    v.data.num = x;
    return v;
}

/* Create a new double type lval. */
struct lval lval_dbl(double x) {
    struct lval v;
    v.type = LVAL_DBL;
    v.data.dbl = x;
    return v;
}

/* Create a new error type lval. */
struct lval lval_err(enum lerr err) {
    struct lval v;
    v.type = LVAL_ERR;
    v.data.err = err;
    return v;
}

bool lval_is_zero(struct lval v) {
    return (v.type == LVAL_NUM && v.data.num == 0)
           || (v.type == LVAL_DBL && fpclassify(v.data.dbl) == FP_ZERO);
}

double lval_as_dbl(struct lval v) {
    switch (v.type) {
    case LVAL_NUM:
        return (double) v.data.num;
    case LVAL_DBL:
        return v.data.dbl;
    default:
        return .0;
    }
}

bool lval_equals(struct lval x, struct lval y) {
    if (x.type == LVAL_ERR) return false;
    if (y.type == LVAL_ERR) return false;
    if (x.type != y.type) return false;

    static const double epsilon = 0.000001;

    if (x.type == LVAL_NUM && x.data.num == y.data.num)
        return true;
    if (x.type == LVAL_DBL && x.data.dbl - y.data.dbl < epsilon)
        return true;

    return false;
}

bool lval_err_equals(struct lval x, struct lval y) {
    if (x.type != y.type) return false;
    if (x.type != LVAL_ERR) return false;
    return x.data.err == y.data.err;
}

/* val is a lval, func is fprintf style func, out is a compatible pointer */
#define WRITER(val, func, out) \
    switch (val.type) { \
    case LVAL_NUM: \
        func(out, "%li", val.data.num); \
        break; \
    case LVAL_DBL: \
        func(out, "%g", val.data.dbl); \
        break; \
    case LVAL_ERR: \
        switch (val.data.err) { \
        case LERR_DIV_ZERO: \
            func(out, "Error: division by 0."); \
            break; \
        case LERR_BAD_OP: \
            func(out, "Error: invalid operator."); \
            break; \
        case LERR_BAD_NUM: \
            func(out, "Error: invalid number."); \
            break; \
        } \
        break; \
    }

void lval_print_to(struct lval v, FILE* out) {
    WRITER(v, fprintf, out);
}

void lval_to_string(struct lval v, char* out) {
    WRITER(v, sprintf, out);
}

/* inline functions */
void lval_print(struct lval v);
void lval_println(struct lval v);