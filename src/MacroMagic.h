#pragma once /* MacroMagic.h */
// WARNING, Black Magic ahead.
// Courtesy of https://github.com/pfultz2/Cloak/wiki/C-Preprocessor-tricks,-tips,-and-idioms
//  and this https://github.com/cormacc/va_args_iterators
// It was NOT just copy n pasted, I verified and tested every line to properly understand it.
// Afterwards, I still say, black magic, but now I know how it works.

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define IIF(c) PRIMITIVE_CAT(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t

#define COMPL(b) PRIMITIVE_CAT(COMPL_, b)
#define COMPL_0 1
#define COMPL_1 0

#define BITAND(x) PRIMITIVE_CAT(BITAND_, x)
#define BITAND_0(y) 0
#define BITAND_1(y) y

#define INC(x) PRIMITIVE_CAT(INC_, x)
#include "DecMacros.h" // IWYU pragma: export

#define DEC(x) PRIMITIVE_CAT(DEC_, x)
#include "IncMacros.h" // IWYU pragma: export

#define CHECK_N(x, n, ...) n
#define CHECK_(...) CHECK_N(__VA_ARGS__, 0, )
#define PROBE(x) x, 1,

#define IS_PAREN(x) CHECK_(IS_PAREN_PROBE x)
#define IS_PAREN_PROBE(...) PROBE(~)

#define NOT(x) CHECK_(PRIMITIVE_CAT(NOT_, x))
#define NOT_0 PROBE(~)

#define BOOL(x) COMPL(NOT(x))
#define IF(c) IIF(BOOL(c))

#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN_(c) IF(c)(EXPAND, EAT)

#define EMPTY()
#define DEFER(id) id EMPTY()
#define DEFER2(...) __VA_ARGS__ DEFER(EMPTY)()
#define DEFER3(...) __VA_ARGS__ DEFER2(EMPTY)()
#define EXPAND(...) __VA_ARGS__

#define EVAL(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__

#define EVAL_(...) EVAL_1(EVAL_1(EVAL_1(__VA_ARGS__)))
#define EVAL_1(...) EVAL_2(EVAL_2(EVAL_2(__VA_ARGS__)))
#define EVAL_2(...) EVAL_3(EVAL_3(EVAL_3(__VA_ARGS__)))
#define EVAL_3(...) EVAL_4(EVAL_4(EVAL_4(__VA_ARGS__)))
#define EVAL_4(...) EVAL_5(EVAL_5(EVAL_5(__VA_ARGS__)))
#define EVAL_5(...) __VA_ARGS__

#define REPEAT(count, macro, ...)                                                                                                          \
    WHEN_(count)                                                                                                                           \
    (DEFER2(REPEAT_INDIRECT)()(DEC(count), macro, __VA_ARGS__)DEFER2(macro)(DEC(count), __VA_ARGS__))
#define REPEAT_INDIRECT() REPEAT

#define WHILE(pred, op, ...)                                                                                                               \
    IF(pred(__VA_ARGS__))                                                                                                                  \
    (DEFER2(WHILE_INDIRECT)()(pred, op, op(__VA_ARGS__)), __VA_ARGS__)
#define WHILE_INDIRECT() WHILE

#define WHILE_RET_2ND(pred, op, a, b)                                                                                                      \
    IF(pred(a, b))                                                                                                                         \
    (DEFER2(WHILE_RET_2ND_INDIRECT)()(pred, op, op(a, b)), b)
#define WHILE_RET_2ND_INDIRECT() WHILE_RET_2ND

#define COMPARE_foo(x) x
#define COMPARE_bar(x) x

#define PRIMITIVE_COMPARE(x, y) IS_PAREN(COMPARE_##x(COMPARE_##y)(()))

#define IS_COMPARABLE(x) IS_PAREN(CAT(COMPARE_, x)(()))

#define NOT_EQUAL(x, y)                                                                                                                    \
    IIF(BITAND(IS_COMPARABLE(x))(IS_COMPARABLE(y)))                                                                                        \
    (PRIMITIVE_COMPARE, 1 EAT)(x, y)

#define EQUAL(x, y) COMPL(NOT_EQUAL(x, y))

#define NARG(...) NARG_(__VA_ARGS__, RSEQ_N())
#define NARG_(...) ARG_N(__VA_ARGS__)
#define ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26,     \
              _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, \
              _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...)                                                          \
    N
#define RSEQ_N()                                                                                                                           \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31,    \
        30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define OP(i, n) DEC(i), DEC(DEC(INC(n)))
#define PRED(i, n) i
#define DEC_N(n, i) EVAL_(WHILE_RET_2ND(PRED, OP, i, n))
// #define F(i, n) CAT(reflectMember, DEC_N(n, INC(i)))();
#define F(i, n) CAT(reflectMember, i)();
#define GEN_REFLECT(clsId, n)                                                                                                              \
    void reflect() {                                                                                                                       \
        _class = clsId;                                                                                                                    \
        EVAL(REPEAT(n, F, __LINE__))                                                                                                       \
    }
#define CONCAT(x, ...)                                                                                                                     \
    IF(DEC(NARG(__VA_ARGS__)))                                                                                                             \
    (x; DEFER2(CONCAT_INDIRECT)()(__VA_ARGS__), IF(NARG(__VA_ARGS__))(x; __VA_ARGS__, x))
#define CONCAT_INDIRECT() CONCAT

#define HEAD_(FIRST, ...) FIRST
#define HEAD(...) HEAD_(__VA_ARGS__)

#define HEAD2_(FIRST, ...) FIRST, HEAD(__VA_ARGS__)
#define HEAD2(...) HEAD2_(__VA_ARGS__)

#define TAIL_(FIRST, ...) __VA_ARGS__
#define TAIL(...) TAIL_(__VA_ARGS__)

#define TAIL2_(FIRST, ...) TAIL(__VA_ARGS__)
#define TAIL2(...) TAIL2_(__VA_ARGS__)

#define PRIMITIVE_IS_EMPTY(...) BITAND(IS_PAREN(HEAD(__VA_ARGS__)()))(NOT(DEC(NARG(__VA_ARGS__))))
#define IS_EMPTY_PAREN(x) IF(IS_PAREN(x))(PRIMITIVE_IS_EMPTY x, 0)
#define IS_EMPTY(...) BITAND(BITAND(PRIMITIVE_IS_EMPTY(__VA_ARGS__))(NOT(IS_PAREN(HEAD(__VA_ARGS__)))))(NOT(DEC(NARG(__VA_ARGS__))))
#define NOT_EMPTY(...) NOT(IS_EMPTY(__VA_ARGS__))

#define DEPAREN(...) DEPAREN_ __VA_ARGS__
#define DEPAREN_(...) __VA_ARGS__
#define IS_ENCLOSED(...) IS_EMPTY(EAT __VA_ARGS__)
#define OPT_DEPAREN(...) IF(IS_ENCLOSED(__VA_ARGS__))(DEPAREN(__VA_ARGS__), __VA_ARGS__)

#define FOR_EACH(TRANSFORM, ...) EVAL(FOR_EACH_(TRANSFORM, __VA_ARGS__))
#define FOR_EACH_(TF, ...)                                                                                                                 \
    IF(NOT_EMPTY(__VA_ARGS__))                                                                                                             \
    (DEFER(TF)(OPT_DEPAREN(HEAD(__VA_ARGS__))) DEFER2(FOR_EACH_INDIRECT)()(TF, TAIL(__VA_ARGS__)), )
#define FOR_EACH_INDIRECT() FOR_EACH_

#define FOR_EACH_ONE(TRANSFORM, ARG1, ...) EVAL(FOR_EACH_ONE_(TRANSFORM, ARG1, __VA_ARGS__))
#define FOR_EACH_ONE_(TF, ARG1, ...)                                                                                                       \
    IF(NOT_EMPTY(__VA_ARGS__))                                                                                                             \
    (DEFER(TF)(ARG1, OPT_DEPAREN(HEAD(__VA_ARGS__))) DEFER2(FOR_EACH_ONE_INDIRECT)()(TF, ARG1, TAIL(__VA_ARGS__)), )
#define FOR_EACH_ONE_INDIRECT() FOR_EACH_ONE_

#define FOR_EACH_TWO(TRANSFORM, ARG1, ARG2, ...) EVAL(FOR_EACH_TWO_(TRANSFORM, ARG1, ARG2, __VA_ARGS__))
#define FOR_EACH_TWO_(TF, ARG1, ARG2, ...)                                                                                                 \
    IF(NOT_EMPTY(__VA_ARGS__))                                                                                                             \
    (DEFER(TF)(ARG1, ARG2, OPT_DEPAREN(HEAD(__VA_ARGS__))) DEFER2(FOR_EACH_TWO_INDIRECT)()(TF, ARG1, ARG2, TAIL(__VA_ARGS__)), )
#define FOR_EACH_TWO_INDIRECT() FOR_EACH_TWO_

#define FOR_EACH_PAIR(TRANSFORM, ...) EVAL(FOR_EACH_PAIR_(TRANSFORM, __VA_ARGS__))
#define FOR_EACH_PAIR_(TF, ...)                                                                                                            \
    IF(NOT_EMPTY(__VA_ARGS__))                                                                                                             \
    (DEFER(TF)(OPT_DEPAREN(HEAD2(__VA_ARGS__))) DEFER2(FOR_EACH_PAIR_INDIRECT)()(TF, TAIL2(__VA_ARGS__)), )
#define FOR_EACH_PAIR_INDIRECT() FOR_EACH_PAIR_

#define FOR_EACH_IDX(TRANSFORM, ...) EVAL(FOR_EACH_IDX_(0, TRANSFORM, __VA_ARGS__))
#define FOR_EACH_IDX_(IDX, TF, ...)                                                                                                        \
    IF(NOT_EMPTY(__VA_ARGS__))                                                                                                             \
    (DEFER(TF)(IDX, OPT_DEPAREN(HEAD(__VA_ARGS__))) DEFER2(FOR_EACH_IDX_INDIRECT)()(INC(IDX), TF, TAIL(__VA_ARGS__)), )
#define FOR_EACH_IDX_INDIRECT() FOR_EACH_IDX_

#define DECL_N(n, type, name, ...)                                                                                                         \
    void CAT(reflectMember, n)() { reflectMember((ReprClosure) & Self::CAT(reprClosure, n), #name, &name); }                               \
    string CAT(reprClosure, n)() const { return ::repr(name); }                                                                            \
    type name DEFER(IF)(NOT_EMPTY(__VA_ARGS__))(= __VA_ARGS__, );

#define REFLECT(clsId, ...)                                                                                                                \
    typedef clsId Self;                                                                                                                    \
    FOR_EACH_IDX(DECL_N, __VA_ARGS__)                                                                                                      \
    GEN_REFLECT(#clsId, NARG(__VA_ARGS__))
