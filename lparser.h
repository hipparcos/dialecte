#ifndef _H_LPARSER_
#define _H_LPARSER_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "llexer.h"

enum ltag {
    LTAG_ERR,
    LTAG_PROG,
    LTAG_EXPR,
    LTAG_NUM,
    LTAG_DBL,
    LTAG_SYM,
    LTAG_STR,
    LTAG_SEXPR,
    LTAG_QEXPR,
};
extern const char* ltag_string[9];

struct last {
    enum ltag tag;
    char* content;
    int line;
    int col;
    /* tree */
    struct last** children;
    size_t childrenc;
};

/** lisp_parse transforms a list of tokens into an ast.
 ** Returns the root of the ast.
 ** error is set to the node containing an error or to NULL.
 ** Caller is responsible for calling last_free() on ast. */
struct last* lisp_parse(struct ltok* tokens, struct last** error);
/** last_free clears ast.
 ** ast must not be used afterwards. */
void last_free(struct last* ast);

/** last_are_equal tells if two ast nodes are equal. */
bool last_are_equal(const struct last* left, const struct last* right);
/** last_are_all_equal tells if two ast are equal. */ 
bool last_are_all_equal(const struct last* left, const struct last* right);

/** last_print_to prints the ast node to the file out. */
void last_print_to(const struct last* ast, FILE* out);
/** last_println prints the ast node to stdout. */
#define last_print(ast) last_print_to(ast, stdout)
/** last_print_all_to prints the ast to the file out. */
void last_print_all_to(const struct last* ast, FILE* out);
/** last_print_all prints the ast to stdout. */
#define last_print_all(ast) last_print_all_to(ast, stdout)

#endif
