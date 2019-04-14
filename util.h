#ifndef TOK_UTIL
#define TOK_UTIL

#include "lex.h"

#define GENERATE_PROTO_STR_LOOKUP_FUNC(name, lookup, type) \
const char* name(type x);

#define GENERATE_PROTO_NUM_LOOKUP_FUNC(name, lookup, type) \
int name(const char *x, type *res);

GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_oper, oper_to_asm, enum OPER);
GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_j, j_to_asm, enum JCOND);
GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_b, b_to_asm, enum JCOND);
GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_reg, reg_to_asm, enum REG);
GENERATE_PROTO_STR_LOOKUP_FUNC(get_token_description, token_to_desc, enum TOKEN_TYPE);

GENERATE_PROTO_NUM_LOOKUP_FUNC(get_oper_from_asm, oper_to_asm, enum OPER);
GENERATE_PROTO_NUM_LOOKUP_FUNC(get_j_from_asm, j_to_asm, enum JCOND);
GENERATE_PROTO_NUM_LOOKUP_FUNC(get_b_from_asm, b_to_asm, enum JCOND);
GENERATE_PROTO_NUM_LOOKUP_FUNC(get_reg_from_asm, reg_to_asm, enum REG);

const char * get_token_description(enum TOKEN_TYPE t);
void indicate_file_area(FILE* fd, size_t line, size_t column, size_t span);

#endif /* TOK_UTIL */
