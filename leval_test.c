#include "leval.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include "vendor/mini-gmp/mini-gmp.h"

#include "lval.h"
#include "lenv.h"

#include "vendor/snow/snow/snow.h"

#define test_pass(input, ouput, ...) \
    it("passes: "input" => "ouput, { \
        struct lval *expected = lval_alloc(); \
        defer(lval_free(expected)); \
        __VA_ARGS__ \
        struct lval *result = lval_alloc(); \
        defer(lval_free(result)); \
        struct lenv* env = lenv_alloc(); \
        defer(lenv_free(env)); \
        lenv_default(env); \
        struct lerr* err = leval_from_string(env, input, result); \
        assert(err == NULL); \
        defer(lerr_free(err)); \
        assert(lval_are_equal(result, expected)); \
    })

#define test_fail(input, err) \
    it("fails: "input" => "#err, { \
        struct lval *expected = lval_alloc(); \
        defer(lval_free(expected)); \
        lval_mut_err_code(expected, err); \
        struct lval *result = lval_alloc(); \
        defer(lval_free(result)); \
        struct lenv* env = lenv_alloc(); \
        defer(lenv_free(env)); \
        lenv_default(env); \
        struct lerr* err = leval_from_string(env, input, result); \
        assert(err != NULL); \
        defer(lerr_free(err)); \
        assert(lval_are_equal(result, expected)); \
    })

#define push_num(args, num) \
    do { \
        struct lval* x = lval_alloc(); \
        lval_mut_num(x, num); \
        lval_push(args, x); \
        lval_free(x); \
    } while (0);

describe(lisp_eval, {
    /* Happy path. */
    test_pass("", "nil", {
            lval_mut_nil(expected);
        });
    test_pass("+ 1 1", "2", {
            lval_mut_num(expected, 2);
        });
    test_pass("+ 1", "1", {
            lval_mut_num(expected, 1);
        });
    test_pass("- 1", "-1", {
            lval_mut_num(expected, -1);
        });
    test_pass("+ 1 1 1 1 1 1", "6", {
            lval_mut_num(expected, 6);
        });
    test_pass("+ 1 1.0", "2.0", {
            lval_mut_dbl(expected, 2.0);
        });
    test_pass("! 21", "2432902008176640000", {
            const long fac20 = 2432902008176640000;
            mpz_t bn_fac21;
            mpz_init_set_ui(bn_fac21, fac20);
            mpz_mul_si(bn_fac21, bn_fac21, 21);
            lval_mut_bignum(expected, bn_fac21);
            mpz_clear(bn_fac21);
        });
    test_pass("* 10 (- 20 10)", "100", {
            lval_mut_num(expected, 100);
        });
    /* S-Expressions. */
    test_pass("(+ 1 1)", "2", {
            lval_mut_num(expected, 2);
        });
    test_pass("(+ 1 1)(+ 2 2)", "4", {
            lval_mut_num(expected, 4);
        });
    /* Nested S-Expressions. */
    test_pass("- (! 21) (! 21) 2.0 1", "-3.0", {
            lval_mut_dbl(expected, -3.0);
        });
    test_pass("- 2.0 1 (- (! 21) (! 21))", "1.0", {
            lval_mut_dbl(expected, 1.0);
        });
    /* Q-Expressions */
    test_pass("(head {1 2 3})", "1", {
            lval_mut_num(expected, 1);
        });
    test_pass("((eval $ head {+ -}) 2 1)", "3", {
            lval_mut_num(expected, 3);
        });
    test_pass("(tail {1 2 3})", "{2 3}", {
            struct lval* two = lval_alloc(); defer(lval_free(two));
            lval_mut_num(two, 2);
            struct lval* thr = lval_alloc(); defer(lval_free(thr));
            lval_mut_num(thr, 3);
            lval_mut_qexpr(expected);
            lval_push(expected, two);
            lval_push(expected, thr);
        });
    test_pass("eval {* 2 21}", "42", {
            lval_mut_num(expected, 42);
        });
    /* Variables definition. */
    test_pass("(def {x y} 1 2)(+ x y)", "3", {
            lval_mut_num(expected, 3);
        });
    test_pass("(= {x} 3)(+ x)", "3", {
            lval_mut_num(expected, 3);
        });
    test_pass("(^ 2 8)(+ . .)", "512", {
            lval_mut_num(expected, 512);
        });
    /* Lamdba definition. */
    test_pass("(\\ {x} {* 2 x}) 4", "8", {
            lval_mut_num(expected, 8);
        });
    test_pass("(def {double} (lambda {x} {* 2 x}))(double 4)", "8", {
            lval_mut_num(expected, 8);
        });
    test_pass("(fun {double x} {* 2 x})(double 4)", "8", {
            lval_mut_num(expected, 8);
        });
    /* Partial function application. */
    test_pass("(fun {mul x y} {* x y})(def {mul2} (mul 2))(mul2 256)", "512", {
            lval_mut_num(expected, 512);
        });
    /* User defined accumulator. */
    test_pass("(fun {joinall x & xs} {join x xs})(joinall {1} 2 3 4 5)", "{1 2 3 4 5}", {
            lval_mut_qexpr(expected);
            push_num(expected, 1);
            push_num(expected, 2);
            push_num(expected, 3);
            push_num(expected, 4);
            push_num(expected, 5);
        });
    /* Control flow. */
    test_pass("if (> 42 0) {+ 21 21} {0}", "42", {
            lval_mut_num(expected, 42);
        });
    test_pass("(= {r} 0)(loop {!= r 42} {(= {r} (+ r 1))})(r)", "42", {
            lval_mut_num(expected, 42);
        });
    /* List functions. */
    test_pass("map (\\ {x} {* 2 x}) {1 2 3 4}", "{2 4 6 8}", {
            lval_mut_qexpr(expected);
            push_num(expected, 2);
            push_num(expected, 4);
            push_num(expected, 6);
            push_num(expected, 8);
        });

    /* Errors. */
    test_fail("/ 10 0", LERR_DIV_ZERO);
    test_fail("1 + 1", LERR_EVAL);
    test_fail("!1", LERR_BAD_SYMBOL);
    test_fail("gibberish", LERR_BAD_SYMBOL);
    test_fail("+ 1 +", LERR_BAD_OPERAND);
    test_fail("+ 1 \"string\"", LERR_BAD_OPERAND);
    test_fail("+ 1 (!1)", LERR_BAD_SYMBOL);
    test_fail("- (", LERR_EVAL);

});

snow_main();
