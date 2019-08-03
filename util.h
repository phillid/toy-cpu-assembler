#ifndef TOK_UTIL
#define TOK_UTIL

#include "instruction.h"
#include "lex.h"

#define GENERATE_PROTO_STR_LOOKUP_FUNC(name, type) \
const char* name(type);

#define GENERATE_PROTO_NUM_LOOKUP_FUNC(name, type) \
int name(const char *x, type *res);

GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_oper, enum OPER)
GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_j, enum JCOND)
GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_b, enum JCOND)
GENERATE_PROTO_STR_LOOKUP_FUNC(get_asm_from_reg, enum REG)
GENERATE_PROTO_STR_LOOKUP_FUNC(get_token_description, enum TOKEN_TYPE)

GENERATE_PROTO_NUM_LOOKUP_FUNC(get_keyword, int)
GENERATE_PROTO_NUM_LOOKUP_FUNC(get_oper_from_asm, enum OPER)
GENERATE_PROTO_NUM_LOOKUP_FUNC(get_j_from_asm, enum JCOND)
GENERATE_PROTO_NUM_LOOKUP_FUNC(get_b_from_asm, enum JCOND)
GENERATE_PROTO_NUM_LOOKUP_FUNC(get_reg_from_asm, enum REG)

const char * get_token_description(enum TOKEN_TYPE t);
void indicate_file_area(FILE* fd, size_t line, size_t column, size_t span);

#endif /* TOK_UTIL */
