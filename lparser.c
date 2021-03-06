#include "lparser.h"

#include <string.h>


#include "lerr.h"

const char* last_tag_string(enum ltag tag) {
    switch (tag) {
        case LTAG_ERR:   return "error";
        case LTAG_PROG:  return "program";
        case LTAG_EXPR:  return "expr";
        case LTAG_NUM:   return "num";
        case LTAG_DBL:   return "double";
        case LTAG_SYM:   return "symbol";
        case LTAG_STR:   return "string";
        case LTAG_SEXPR: return "sexpr";
        case LTAG_QEXPR: return "qexpr";
    }
    return NULL;
}

static struct last* last_alloc(enum ltag tag, const char* content, const struct ltok* tok) {
    struct last* ast = calloc(1, sizeof(struct last));
    ast->tag = tag;
    size_t len = strlen(content);
    ast->content = malloc(len+1);
    memcpy(ast->content, content, len);
    ast->content[len] = '\0';
    if (tok) {
        ast->line = tok->line;
        ast->col = tok->col;
    }
    return ast;
}

static struct last* last_error(enum lerr_code error, const struct ltok* tok) {
    struct last* err = last_alloc(LTAG_ERR, "", tok);
    err->err = error;
    return err;
}

static bool last_attach(struct last* child, struct last* parent) {
    if (!child || !parent) {
        return NULL;
    }
    parent->children = realloc(parent->children, ++parent->childrenc * sizeof(struct last*));
    parent->children[parent->childrenc-1] = child;
    return true;
};

static struct last* lparse_symbol(struct ltok* tokens) {
    if (tokens->type != LTOK_SYM) {
        return NULL;
    }
    return last_alloc(LTAG_SYM, tokens->content, tokens);
}

static struct last* lparse_number(struct ltok* tokens) {
    if (tokens->type != LTOK_NUM) {
        return NULL;
    }
    return last_alloc(LTAG_NUM, tokens->content, tokens);
}

static struct last* lparse_double(struct ltok* tokens) {
    if (tokens->type != LTOK_DBL) {
        return NULL;
    }
    return last_alloc(LTAG_DBL, tokens->content, tokens);
}

static struct last* lparse_string(struct ltok* tokens) {
    if (tokens->type != LTOK_STR) {
        return NULL;
    }
    char* content = tokens->content;
    content++; // skip opening ".
    size_t len = strlen(content);
    len--; // skip closing ".
    char* buffer = malloc(len+1);
    /* Remove opening and closing " and escape \. */
    size_t lencpy = 0;
    char *curr = buffer, *end = NULL;
    while (NULL != (end = strstr(content, "\\\""))) {
        lencpy = end - content;
        memcpy(curr, content, lencpy);
        curr += lencpy;
        content += lencpy + 1; // skip \.
        len -= lencpy + 1;
    }
    memcpy(curr, content, len);
    curr[len] = '\0';
    struct last* node = last_alloc(LTAG_STR, buffer, tokens);
    free(buffer);
    return node;
}

static struct last* lparse_atom(struct ltok* curr, struct ltok** last) {
    if (curr->type == LTOK_EOF) {
        return NULL;
    }
    struct last* atom = NULL;
    switch (curr->type) {
    case LTOK_NUM:
        atom = lparse_number(curr);
        curr = curr->next;
        break;
    case LTOK_DBL:
        atom = lparse_double(curr);
        curr = curr->next;
        break;
    case LTOK_STR:
        atom = lparse_string(curr);
        curr = curr->next;
        break;
    case LTOK_SYM:
        atom = lparse_symbol(curr);
        curr = curr->next;
        break;
    default: break;
    }
    *last = curr;
    return atom;
}

static struct last* lparse_sexpr(struct ltok* first, struct ltok** last, bool skip);
static struct last* lparse_qexpr(struct ltok* first, struct ltok** last);

static struct last* lparse_list(struct ltok* curr, struct ltok** last) {
    if (curr->type == LTOK_EOF) {
        return NULL;
    }
    struct last* list = NULL;
    switch (curr->type) {
    case LTOK_OPAR:
        list = lparse_sexpr(curr, &curr, true);
        break;
    case LTOK_DOLL:
        list = lparse_sexpr(curr, &curr, false);
        break;
    case LTOK_OBRC:
        list = lparse_qexpr(curr, &curr);
        break;
    default: break;
    }
    *last = curr;
    return list;
}

static struct last* lparse_expr(struct ltok* first, struct ltok** last) {
    struct ltok* curr = first;
    struct last* expr = NULL;
    struct last* sexpr = NULL;
    /* An expression start with a symbol or a S-Expression. */
    switch (curr->type) {
    case LTOK_OPAR: // Start of SEXPR.
        sexpr = lparse_sexpr(curr, &curr, true);
        if (sexpr->tag == LTAG_ERR) {
            expr = sexpr;
            break;
        }
        expr = last_alloc(LTAG_EXPR, "", curr);
        last_attach(sexpr, expr);
        break;
    case LTOK_SYM:
        expr = last_alloc(LTAG_EXPR, "", curr);
        /* Symbol. */
        last_attach(lparse_symbol(curr), expr);
        curr = curr->next;
        break;
    default:
        expr = last_error(LERR_PARSER_BAD_EXPR, curr);
    case LTOK_EOF:
        *last = curr;
        return expr;
    }
    /* Operands. */
    while (curr->type != LTOK_CPAR && curr->type != LTOK_CBRC && curr->type != LTOK_EOF) {
        struct last* operand = NULL;
        if (!(operand = lparse_atom(curr, &curr)) &&
            !(operand = lparse_list(curr, &curr))) {
              operand = last_error(LERR_PARSER_BAD_OPERAND, curr);
        }
        // Error = break.
        if (operand->tag == LTAG_ERR) {
            last_attach(expr, operand);
            expr = operand;
            break;
        }
        last_attach(operand, expr);
    }
    *last = curr;
    return expr;
}

static struct last* lparse_sexpr(struct ltok* first, struct ltok** last, bool skip_par) {
    struct ltok* curr = first;
    struct last* sexpr;
    struct last* expr = NULL;
    if (curr->type == LTOK_EOF) {
        *last = curr;
        return NULL;
    }
    // (.
    if (curr->type != LTOK_OPAR && curr->type != LTOK_DOLL) {
        sexpr = last_error(LERR_PARSER_MISSING_OPAR, curr);
        *last = curr;
        return sexpr;
    }
    curr = curr->next; // Skip ( or $.
    // Inner expr.
    expr = lparse_expr(curr, &curr);
    // Error = break.
    if (expr && expr->tag == LTAG_ERR) {
        return expr;
    }
    // ) or error.
    if (skip_par && curr->type != LTOK_CPAR) {
        sexpr = last_error(LERR_PARSER_MISSING_CPAR, curr);
    } else {
        sexpr = last_alloc(LTAG_SEXPR, "", curr);
        if (skip_par) curr = curr->next; // Skip ).
    }
    last_attach(expr, sexpr);
    *last = curr;
    return sexpr;
}

static struct last* lparse_qexpr(struct ltok* first, struct ltok** last) {
    struct ltok* curr = first;
    if (curr->type == LTOK_EOF) {
        *last = curr;
        return NULL;
    }
    curr = curr->next; // Skip {.
    // Inner list.
    struct last* qexpr = last_alloc(LTAG_QEXPR, "", curr);
    // LTOK_CPAR needed to detect missing `}`.
    while (curr->type != LTOK_CBRC && curr->type != LTOK_CPAR && curr->type != LTOK_EOF) {
        struct last* operand = NULL;
        if (!(operand = lparse_atom(curr, &curr)) &&
            !(operand = lparse_list(curr, &curr))) {
              operand = last_error(LERR_PARSER_BAD_OPERAND, curr);
        }
        // Error = break.
        if (operand->tag == LTAG_ERR) {
            last_attach(qexpr, operand);
            qexpr = operand;
            break;
        }
        last_attach(operand, qexpr);
    }
    // Error = break.
    if (qexpr && qexpr->tag == LTAG_ERR) {
        return qexpr;
    }
    // } or error.
    if (curr->type != LTOK_CBRC) {
        struct last* err = last_error(LERR_PARSER_MISSING_CBRC, curr);
        last_attach(qexpr, err);
        qexpr = err;
    }
    curr = curr->next; // Skip }.
    *last = curr;
    return qexpr;
}

static struct last* lparse_program(struct ltok* tokens, struct last** error) {
    if (tokens->type == LTOK_EOF) {
        return NULL;
    }
    struct last* prg = last_alloc(LTAG_PROG, "", NULL);
    struct ltok* curr = tokens;
    struct last* expr = NULL;
    *error = NULL;
    while ((expr = lparse_sexpr(curr, &curr, true))) {
        last_attach(expr, prg);
        if (expr->tag == LTAG_ERR) {
            *error = expr;
            break;
        }
    }
    return prg;
}

struct last* lisp_parse(struct ltok* tokens, struct lerr** err) {
    if (!tokens) {
        return NULL;
    }
    struct last* ast_error = NULL;
    struct last* ast = lparse_program(tokens, &ast_error);
    /* Error handling. */
    if (ast_error) {
        switch (ast_error->err) {
        case LERR_PARSER_MISSING_OPAR:
            *err = lerr_throw(ast_error->err, "missing opening parenthesis");
            break;
        case LERR_PARSER_MISSING_CPAR:
            *err = lerr_throw(ast_error->err, "missing closing parenthesis");
            break;
        case LERR_PARSER_MISSING_CBRC:
            *err = lerr_throw(ast_error->err, "missing closing brace");
            break;
        case LERR_PARSER_BAD_OPERAND:
            *err = lerr_throw(ast_error->err,
                    "operands must be of types num|double|string|symbol|sexpr|qexpr");
            break;
        case LERR_PARSER_BAD_EXPR:
            *err = lerr_throw(ast_error->err, "an expression must start with a symbol or a `(`");
            break;
        default:
            *err = lerr_throw(LERR_PARSER, "unknown error");
            break;
        }
        lerr_set_location(*err, ast_error->line, ast_error->col);
    }
    return ast;
}

void last_free(struct last* ast) {
    if (!ast) {
        return;
    }
    if (ast->children) {
        size_t childrenc = ast->childrenc;
        for (size_t i = 0; i < childrenc; i++) {
            last_free(ast->children[i]);
        }
        free(ast->children);
    }
    if (ast->content) {
        free(ast->content);
    }
    free(ast);
}

bool last_are_equal(const struct last* left, const struct last* right) {
    if (!left || !right) {
        return false;
    }
    return left->tag == right->tag && strcmp(left->content, right->content) == 0;
}

bool last_are_all_equal(const struct last* left, const struct last* right) {
    if (!left || !right) {
        return false;
    }
    if (left->childrenc != right->childrenc) {
        return false;
    }
    if (!last_are_equal(left, right)) {
        return false;
    }
    for (size_t i = 0; i < left->childrenc; i++) {
        if (!last_are_all_equal(left->children[i], right->children[i])) {
            return false;
        }
    }
    return true;
}

static void last_print_to_indent(const struct last* ast, FILE* out, unsigned int indent) {
    if (!ast) {
        return;
    }
    while (indent-- > 0) {
        fputs("  ", out);
    }
    if (!ast->content) {
        fprintf(out, "tag: %si, %d:%d",
                last_tag_string(ast->tag), ast->line, ast->col);
    } else {
        fprintf(out, "tag: %s, %d:%d, content: \"%s\"",
                last_tag_string(ast->tag), ast->line, ast->col, ast->content);
    }
    fputc('\n', out);
}

void last_print_to(const struct last* ast, FILE* out) {
    if (!ast) {
        return;
    }
    last_print_to_indent(ast, out, 0);
}

static void last_print_all_to_rec(const struct last* ast, FILE* out, unsigned int level) {
    if (!ast) {
        return;
    }
    last_print_to_indent(ast, out, level);
    for (size_t i = 0; i < ast->childrenc; i++) {
        last_print_all_to_rec(ast->children[i], out, level + 1);
    }
}

void last_print_all_to(const struct last* ast, FILE* out) {
    if (!ast) {
        return;
    }
    last_print_all_to_rec(ast, out, 0);
}
