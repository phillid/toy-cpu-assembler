#ifndef TOK_UTIL
#define TOK_UTIL

#include "lex.h"

const char * get_token_description(enum TOKEN_TYPE t);
void indicate_file_area(FILE* fd, size_t line, size_t column, size_t span);

#endif /* TOK_UTIL */
