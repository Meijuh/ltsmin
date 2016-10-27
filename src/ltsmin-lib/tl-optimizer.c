#include <hre/config.h>

#include <ltsmin-lib/ltsmin-standard.h>
#include <ltsmin-lib/ltsmin-syntax.h>

#include "ltsmin-tl.h"

typedef ltsmin_expr_t(*optimizer_t)(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate);

//typedef int(*dual_t)(int token, int *negate);
//
//int dual_shared(int token, int *negate) {
//    switch (token) {
//        case S_OR: {
//            *negate = 1;
//            return S_AND;
//        }
//        case S_AND: {
//            *negate = 1;
//            return S_OR;
//        }
//        case S_EQ: {
//            *negate = 1;
//            return S_NEQ;
//        }
//        default: return -1;
//    }
//}
//
//int dual_atom(int token, int *negate) {
//    switch (token) {
//        case S_TRUE: {
//            *negate = 0;
//            return S_FALSE;
//        }
//        case S_FALSE: {
//            *negate = 0;
//            return S_TRUE;
//        }
//        case S_MAYBE: {
//            *negate = 0;
//            return S_MAYBE;
//        }
//        case S_LT: {
//            *negate = 0;
//            return S_GEQ;
//        }
//        case S_LEQ: {
//            *negate = 0;
//            return S_GT;
//        }
//        case S_GT: {
//            *negate = 0;
//            return S_LEQ;
//        }
//        case S_GEQ: {
//            *negate = 0;
//            return S_LT;
//        }
//        case S_NEQ: {
//            *negate = 1;
//            return S_EQ;
//        }
//        default:        return -1;
//    }
//}
//
//int dual_pred(int token, int *negate) {
//    return dual_shared(token, negate);
//}
//
//int dual_CTL(int token, int *negate) {
//    (void) negate;
//    switch (token) {
//        case CTL_ALL:       return CTL_EXIST;
//        case CTL_EXIST:     return CTL_ALL;
//        case CTL_FUTURE:    return CTL_GLOBALLY;
//        case CTL_GLOBALLY:  return CTL_FUTURE;
//        case CTL_NEXT:      return CTL_NEXT;
//        default:            return -1;
//    }
//}
//
//int
//binary_pre(ltsmin_expr_t e, ltsmin_parse_env_t env, optimizer_t o, int *negate, dual_t dual, ltsmin_expr_t *left, ltsmin_expr_t *right)
//{
//    int t;
//    if (*negate) {
//        t = dual(e->token, negate);
//        if (t == -1) {
//            t = e->token;
//            *left = o(e->arg1, env, 0);
//            *right = o(e->arg2, env, 0);
//        } else {
//            *left = o(e->arg1, env, *negate);
//            *right = o(e->arg2, env, *negate);
//            *negate = 0;
//        }
//    } else {
//        t = e->token;
//        *left = o(e->arg1, env, 0);
//        *right = o(e->arg2, env, 0);
//    }
//    return t;
//}
//
//ltsmin_expr_t
//binary_post(ltsmin_expr_t e, ltsmin_parse_env_t env, ltsmin_expr_t left, ltsmin_expr_t right, int token, const char *name, int negate)
//{
//    Warning(info, "must negate: %d", negate);
//    ltsmin_expr_t n;
//    if (e->arg1 != left) {
//        LTSminLogExpr(info, "destroy left: ", e->arg1, env);
//        LTSminLogExpr(info, "other left: ", left, env);
//        LTSminExprDestroy(e->arg1, 1);
//    }
//    if (e->arg2 != right) {
//        LTSminLogExpr(info, "destroy right: ", e->arg2, env);
//        LTSminLogExpr(info, "other right: ", right, env);
//        LTSminExprDestroy(e->arg2, 1);
//    }
//    if (e->arg1 != left || e->arg2 != right || e->token != token) {
//        LTSminExprDestroy(e, 0);
//        n = LTSminExpr(BINARY_OP, token, LTSminBinaryIdx(env, name), left, right);
//        LTSminLogExpr(info, "create left: ", left, env);
//        LTSminLogExpr(info, "create right: ", right, env);
//        LTSminLogExpr(info, "created: ", n, env);
//    } else {
//        LTSminLogExpr(info, "reusing: ", e, env);
//        n = e;
//    }
//
//    if (negate) {
//        if (n == e) {
//            Warning(info, "reusing negating");
//            HREassert(n->parent != NULL && n->parent->token == S_NOT);
//            n = n->parent;
//        } else {
//
//        LTSminLogExpr(info, "negating bin: ", n, env);
//        Abort("wut");
//            n = LTSminExpr(UNARY_OP, S_NOT, LTSminUnaryIdx(env, name), n, NULL);
//        }
//    }
//
//    return n;
//}
//
////int
////unary_pre(ltsmin_expr_t e, ltsmin_parse_env_t env, optimizer_t o, int *negate, dual_t dual, ltsmin_expr_t *child)
////{
////    int t;
////    if (*negate) {
////        t = dual(e->token, negate);
////        if (t == -1) {
////            t = e->token;
////            *child = o(e->arg1, env, 0);
////        } else {
////            *child = o(e->arg1, env, *negate);
////            *negate = 0;
////        }
////    } else {
////        t = e->token;
////        *child = o(e->arg1, env, 0);
////    }
////    return t;
////}
//
//ltsmin_expr_t
//unary_post(ltsmin_expr_t e, ltsmin_parse_env_t env, ltsmin_expr_t child, int token, const char *name, int negate)
//{
//        Warning(info, "kwdddddjkjkl");
//    HREassert(!negate || child->token != S_NOT);
//    ltsmin_expr_t n;
//    if (e->arg1 != child || e->token != token) {
//        LTSminExprDestroy(e, 1);
//        n = LTSminExpr(UNARY_OP, token, LTSminUnaryIdx(env, name), child, NULL);
//    } else n = e;
//
//    if (negate) {
//        Warning(info, "kwlwjlkj3lkj3lk3j3llkj3ljk3ljkljkjkl");
//        if (n == e) {
//            HREassert(n->parent != NULL && n->parent->token == S_NOT);
//            Warning(info, "dfjklasdjlks33lk3k3l3kjlk3j3l3");
//            n = n->parent;
//        } else {
//
//        LTSminLogExpr(info, "negating: ", n, env);
//            n = LTSminExpr(UNARY_OP, S_NOT, LTSminUnaryIdx(env, name), n, NULL);
//        }
//    }
//    return n;
//}
//
//ltsmin_expr_t
//optimize_shared_binary(ltsmin_expr_t e, ltsmin_parse_env_t env, optimizer_t o, ltsmin_expr_t left, ltsmin_expr_t right, ltsmin_expr_case token, int negate)
//{
//    switch (token) {
//        case S_AND: {
//            if (left->token == S_FALSE) { // false /\ a is false
//                LTSminExprDestroy(right, 1);
//                LTSminExprDestroy(e, 0);
//                return left;
//            } else if (right->token == S_FALSE) { // a /\ false is false
//                LTSminExprDestroy(left, 1);
//                LTSminExprDestroy(e, 0);
//                return right;
//            } else if (left->token == S_TRUE) { // true /\ a is a
//                LTSminExprDestroy(left, 1);
//                LTSminExprDestroy(e, 0);
//                return right;
//            } else if (right->token == S_TRUE) { // a /\ true is a
//                LTSminExprDestroy(right, 1);
//                LTSminExprDestroy(e, 0);
//                return left;
//            } else if (LTSminExprEq(left, right)) { // a /\ a is a
//
//                LTSminLogExpr (error, "left: ", left, env);
//                LTSminLogExpr (error, "right: ", right, env);
//
//                LTSminExprDestroy(e, 0);
//                LTSminExprDestroy(right, 1);
//                return left;
//            }
//            break;
//        }
//        case S_OR: {
//            if (left->token == S_TRUE) { // true \/ a is true
//                LTSminExprDestroy(right, 1);
//                LTSminExprDestroy(e, 0);
//                return left;
//            } else if (right->token == S_TRUE) { // a \/ true is true
//                LTSminExprDestroy(left, 1);
//                LTSminExprDestroy(e, 0);
//                return right;
//            } else if (left->token == S_FALSE) { // false \/ a is a
//                LTSminExprDestroy(left, 1);
//                LTSminExprDestroy(e, 0);
//                return right;
//            } else if (right->token == S_FALSE) { // a \/ false is a
//                LTSminExprDestroy(right, 1);
//                LTSminExprDestroy(e, 0);
//                return left;
//            } else if (LTSminExprEq(left, right)) { // a \/ a is a
//                LTSminExprDestroy(e, 0);
//                LTSminExprDestroy(right, 1);
//                return left;
//            }
//            break;
//        }
//        case S_EQ: case S_EQUIV: {
//            if (LTSminExprEq(left, right)) { // a {<->, ==} a is true
//                LTSminExprDestroy(e, 1);
//                return LTSminExpr(CONSTANT, S_TRUE, LTSminConstantIdx(env, S_NAME(S_TRUE)), 0, 0);
//            } else if (left->token == S_TRUE) { // true {<->,==} a is a
//                LTSminExprDestroy(left, 1);
//                LTSminExprDestroy(e, 0);
//                return right;
//            } else if (right->token == S_TRUE) { // a {<->,==} true is a
//                LTSminExprDestroy(right, 1);
//                LTSminExprDestroy(e, 0);
//                return left;
//            } else if (left->token == S_FALSE) { // false {<->,==} a is !a, !a can be optimized
//                LTSminExprDestroy(left, 1);
//                LTSminExprDestroy(e, 0);
//                return o(right, env, 1);
//            } else if (right->token == S_FALSE) { // a {<->,==} false is !a, !a can be optimized
//                LTSminExprDestroy(right, 1);
//                LTSminExprDestroy(e, 0);
//                return o(left, env, 1);
//            }
//            break;
//        }
//        case S_IMPLY: {
//            if (LTSminExprEq(left, right)) { // a -> a is true
//                LTSminExprDestroy(e, 1);
//                return LTSminExpr(CONSTANT, S_TRUE, LTSminConstantIdx(env, S_NAME(S_TRUE)), 0, 0);
//            } else if (left->token == S_FALSE || right->token == S_TRUE) { // false -> a is true, a -> true is true
//                LTSminExprDestroy(e, 1);
//                return LTSminExpr(CONSTANT, S_TRUE, LTSminConstantIdx(env, S_NAME(S_TRUE)), 0, 0);
//            } else if (left->token == S_TRUE) { // true -> a is a
//                LTSminExprDestroy(left, 1);
//                LTSminExprDestroy(e, 0);
//                return right;
//            } else if (right->token == S_FALSE) { // a -> false is !a, !a can be optimized
//                LTSminExprDestroy(right, 1);
//                LTSminExprDestroy(e, 0);
//                return o(left, env, 1);
//            }
//            break;
//        }
//        default: {
//#ifndef NDEBUG
//            LTSminLogExpr (error, "unrecognized expression: ", e, env);
//#endif
//            HREassert(false);
//            return NULL;
//
//        }
//    }
//
//    return binary_post(e, env, left, right, token, S_NAME(token), negate);
//
//}
//
//ltsmin_expr_t
//optimize_shared_unary(ltsmin_expr_t e, ltsmin_parse_env_t env, optimizer_t o, int negate)
//{
//    switch (e->token) {
//        case S_NOT: {
//
//        LTSminLogExpr(info, "notting: ", e->arg1, env);
//            return o(e->arg1, env, !negate);
//        }
//        default: {
//#ifndef NDEBUG
//            LTSminLogExpr (error, "unrecognized expression: ", e, env);
//#endif
//            HREassert(false);
//            return NULL;
//        }
//    }
//}
//
//ltsmin_expr_t
//optimize_atom(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate)
//{
//    fprintf(stderr, "negate: %d ,", negate);
//    LTSminLogExpr (info, "expr: ", e, env);
//    switch (e->node_type) {
//        case BINARY_OP: {
//            ltsmin_expr_t left, right;
//            const int t = binary_pre(e, env, &optimize_atom, &negate, &dual_atom, &left, &right);
//            switch (t) {
//                case S_NEQ: {
//                    if (LTSminExprEq(left, right)) { // a != a is false
//                        LTSminExprDestroy(e, 1);
//                        return LTSminExpr(CONSTANT, S_FALSE,  LTSminConstantIdx(env, S_NAME(S_FALSE)), 0, 0);
//                    } else if (left->token == S_FALSE) { // false != a is a
//                        LTSminExprDestroy(left, 1);
//                        LTSminExprDestroy(e, 0);
//                        return right;
//                    } else if (right->token == S_FALSE) { // a != false is a
//                        LTSminExprDestroy(right, 1);
//                        LTSminExprDestroy(e, 0);
//                        return left;
//                    } else if (left->token == S_TRUE) { // true != a is !a, !a can be optimized
//                        LTSminExprDestroy(left, 1);
//                        LTSminExprDestroy(e, 0);
//                        return optimize_atom(right, env, 1);
//                    } else if (right->token == S_TRUE) { // a != true is !a, !a can be optimized
//                        LTSminExprDestroy(right, 1);
//                        LTSminExprDestroy(e, 0);
//                        return optimize_atom(left, env, 1);
//                    }
//                    break;
//                }
//                case S_LEQ: case S_GEQ: {
//                    if (LTSminExprEq(left, right)) { // a {<=, >=} a is true
//                        LTSminExprDestroy(e, 1);
//                        return LTSminExpr(CONSTANT, S_TRUE, LTSminConstantIdx(env, S_NAME(S_TRUE)), 0, 0);
//                    }
//                    break;
//                }
//                case S_LT: case S_GT: {
//                    if (LTSminExprEq(left, right)) { // a {<, >} a is false
//                        LTSminExprDestroy(e, 1);
//                        return LTSminExpr(CONSTANT, S_FALSE,  LTSminConstantIdx(env, S_NAME(S_FALSE)), 0, 0);
//                    }
//                    break;
//                }
//                case S_REM: {
//                    if (right->idx == 0) {
//                        LTSminLogExpr (error, "division by zero in: ", e, env);
//                        HREabort(LTSMIN_EXIT_FAILURE);
//                    }
//                }
//                case S_SUB: {
//                    if (LTSminExprEq(left, right)) { // a {-, %} a is 0
//                        LTSminExprDestroy(e, 1);
//                        return LTSminExpr(INT, INT, 0, 0, 0);
//                    }
//                    break;
//                }
//                case S_DIV: {
//                    if (right->idx == 0) {
//                        LTSminLogExpr (error, "division by zero in: ", e, env);
//                        HREabort(LTSMIN_EXIT_FAILURE);
//                    }
//                    if (LTSminExprEq(left, right)) { // a / a is 1
//                        LTSminExprDestroy(e, 1);
//                        return LTSminExpr(INT, INT, 1, 0, 0);
//                    }
//                    break;
//                }
//                default: {
//                    return optimize_shared_binary(e, env, &optimize_atom, left, right, t, negate);
//                }
//            }
//
//                    Warning(info, "this uber default3333111eleven");
//            return binary_post(e, env, left, right, t, S_NAME(t), negate);
//        }
//        case UNARY_OP: {
//            return optimize_shared_unary(e, env, &optimize_atom, negate);
//        }
//        case SVAR:
//        case EVAR:
//        case CHUNK:
//        case INT:
//        case CONSTANT: {
//            if (negate) {
//                const int t = dual_atom(e->token, &negate);
//                if (t == -1) {
//                    HREassert(e->parent != NULL && e->parent->token == S_NOT);
//                    return e->parent;
//                } else {
//                    LTSminExprDestroy(e, 1);
//                    return LTSminExpr(CONSTANT, t, LTSminConstantIdx(env, S_NAME(t)), 0, 0);
//                }
//            } else return e;
//        }
//        default: HREassert(false);
//    }
//}
//
//ltsmin_expr_t
//optimize_pred(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate)
//{
//    switch (e->token) {
//        case PRED_AND: case PRED_OR: case PRED_EQUIV: case PRED_IMPLY: {
//            ltsmin_expr_t left, right;
//            const int t = binary_pre(e, env, &optimize_pred, &negate, &dual_pred, &left, &right);
//            return optimize_shared_binary(e, env, &optimize_pred, left, right, t, negate);
//        }
//        case PRED_NOT: {
//            return optimize_shared_unary(e, env, &optimize_pred, negate);
//        }
//        default: {
//            return optimize_atom(e, env, negate);
//        }
//    }
//}
//
//ltsmin_expr_t
//optimize_CTL(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate)
//{
//    (void) env; (void) negate;
//    return e;
////    switch (e->node_type) {
////        case BINARY_OP: {
////            ltsmin_expr_t left, right;
////            const int t = binary_pre(e, env, &optimize_CTL, &negate, &dual_CTL, &left, &right);
////            switch (t) {
////                case CTL_UNTIL: {
////                    return binary_post(e, env, left, right, t, CTL_NAME(t), negate);
////                }
////                default: {
////                    return optimize_shared_binary(e, env, &optimize_CTL, left, right, t, negate);
////                }
////            }
////        }
////        case UNARY_OP: {
////            switch (e->token) {
////                case CTL_ALL: case CTL_EXIST: case CTL_FUTURE: case CTL_GLOBALLY: case CTL_NEXT: {
////                    int t;
////                    if (negate) t = dual_CTL(e->token);
////                    else t = e->token;
////                    return unary_post(e, env, optimize_CTL(e->arg1, env, negate), t, CTL_NAME(t), 0);
////                }
////                default: {
////                    return optimize_shared_unary(e, env, &optimize_CTL, negate);
////                }
////            }
////
////        }
////        default: {
////            return optimize_atom(e, env, negate);
////        }
////    }
//}

ltsmin_expr_t
optimize_TRUE(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate)
{
    if (negate) {
        LTSminExprDestroy(e, 0);
        return LTSminExpr(CONSTANT, S_FALSE, LTSminConstantIdx(env, S_NAME(S_FALSE)), 0, 0);
    } else return e;
}

ltsmin_expr_t
optimize_FALSE(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate)
{
    if (negate) {
        LTSminExprDestroy(e, 0);
        return LTSminExpr(CONSTANT, S_TRUE, LTSminConstantIdx(env, S_NAME(S_TRUE)), 0, 0);
    } else return e;
}

ltsmin_expr_t
optimize_MAYBE(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate)
{
    if (negate) {
        LTSminExprDestroy(e, 0);
        return LTSminExpr(CONSTANT, S_MAYBE, LTSminConstantIdx(env, S_NAME(S_MAYBE)), 0, 0);
    } else return e;
}

ltsmin_expr_t
optimize_NOT(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate, optimizer_t o)
{
    LTSminExprDestroy(e, 0);
    return o(e->arg1, env, !negate);
}

ltsmin_expr_t
optimize_GEQ(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate, optimizer_t o)
{
    const ltsmin_expr_t left = o(e->arg1, env, 0);
    const ltsmin_expr_t right = o(e->arg2, env, 0);

    if (LTSminExprEq(left, right)) { // a >= a is true
        LTSminExprDestroy(e, 0);
        LTSminExprDestroy(left, 1);
        LTSminExprDestroy(right, 1);
        if (negate) {
            return LTSminExpr(CONSTANT, S_FALSE, LTSminConstantIdx(env, S_NAME(S_FALSE)), 0, 0);
        } else {
            return LTSminExpr(CONSTANT, S_TRUE, LTSminConstantIdx(env, S_NAME(S_TRUE)), 0, 0);
        }
    } else if (negate) {
        LTSminExprDestroy(e, 0);
        return LTSminExpr(BINARY_OP, S_LT, LTSminBinaryIdx(env, S_NAME(S_LT)), left, right);
    } else if (e->arg1 != left || e->arg2 != right){
        LTSminExprDestroy(e, 0);
        return LTSminExpr(BINARY_OP, S_GEQ, LTSminBinaryIdx(env, S_NAME(S_GEQ)), left, right);
    } else return e;
}

ltsmin_expr_t
optimize_LT(ltsmin_expr_t e, ltsmin_parse_env_t env, int negate, optimizer_t o)
{
    const ltsmin_expr_t left = o(e->arg1, env, 0);
    const ltsmin_expr_t right = o(e->arg2, env, 0);

    if (LTSminExprEq(left, right)) { // a < a is false
        LTSminExprDestroy(e, 0);
        LTSminExprDestroy(left, 1);
        LTSminExprDestroy(right, 1);
        if (negate) {
            return LTSminExpr(CONSTANT, S_TRUE, LTSminConstantIdx(env, S_NAME(S_TRUE)), 0, 0);
        } else {
            return LTSminExpr(CONSTANT, S_FALSE, LTSminConstantIdx(env, S_NAME(S_FALSE)), 0, 0);
        }
    } else if (negate) {
        LTSminExprDestroy(e, 0);
        return LTSminExpr(BINARY_OP, S_GEQ, LTSminBinaryIdx(env, S_NAME(S_GEQ)), left, right);
    } else if (e->arg1 != left || e->arg2 != right){
        LTSminExprDestroy(e, 0);
        return LTSminExpr(BINARY_OP, S_LT, LTSminBinaryIdx(env, S_NAME(S_LT)), left, right);
    } else return e;
}