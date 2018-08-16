#ifndef _H_BUILTIN_FUNC_
#define _H_BUILTIN_FUNC_

#include "lval.h"
#include "lenv.h"

int lbi_cond_qexpr(const struct lenv* env, const struct lval* arg);
int lbi_cond_qexpr_or_nil(const struct lenv* env, const struct lval* arg);
int lbi_cond_list(const struct lenv* env, const struct lval* arg);
int lbi_cond_qexpr_all(const struct lenv* env, const struct lval* args);
int lbi_cond_list_all(const struct lenv* env, const struct lval* args);

/** lbi_func_head returns the first element of a Q-Expression. */
int lbi_func_head(struct lenv* env, struct lval* acc, const struct lval* args);
/** lbi_func_tail returns a Q-Expression without its head. */
int lbi_func_tail(struct lenv* env, struct lval* acc, const struct lval* args);
/** lbi_func_init returns a Q-Expression without its last element. */
int lbi_func_init(struct lenv* env, struct lval* acc, const struct lval* args);
/** lbi_func_cons adds an element x at the beginning of acc. */
int lbi_func_cons(struct lenv* env, struct lval* acc, const struct lval* args);
/** lbi_func_len returns the len of a Q-Expression. */
int lbi_func_len(struct lenv* env, struct lval* acc, const struct lval* args);
/** lbi_func_join push all children of x in acc. */
int lbi_func_join(struct lenv* env, struct lval* acc, const struct lval* args);
/** lbi_func_list push x in to acc. acc is mutated into a qexpr. */
int lbi_func_list(struct lenv* env, struct lval* acc, const struct lval* args);
/** lbi_func_eval evaluates x and put the result into acc. */
int lbi_func_eval(struct lenv* env, struct lval* acc, const struct lval* args);

#endif