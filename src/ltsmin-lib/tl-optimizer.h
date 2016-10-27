#ifndef TL_OPTIMIZER_H
#define TL_OPTIMIZER_H

#include <ltsmin-lib/ltsmin-syntax.h>

ltsmin_expr_t optimize_atom(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate);

ltsmin_expr_t optimize_pred(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate);

ltsmin_expr_t optimize_CTL(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate);

#endif

