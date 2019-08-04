#ifndef LEX_H
#define LEX_H

#include <stdio.h>

enum TOKEN_TYPE {
	TOKEN_COMMA = 1,
	TOKEN_DOT, /* starts an assembler directive */
	TOKEN_LABEL, /* label declaration */
	TOKEN_IDENT, /* identifier (not label decl) or instruction */
	TOKEN_KEYWORD, /* keyword used to tell the assembler special information */
	TOKEN_STRING, /* string literal */
	TOKEN_NUMERIC, /* numeric literal, incl literal chars */
	TOKEN_REGISTER, /* $0, $H, $1 */
	TOKEN_EOL /* end of line */
};

struct token {
	enum TOKEN_TYPE type;
	/* line and column of the source file this token occurs at. 1-based. */
	size_t line;
	size_t column;
	size_t span;
	char *s_val;
	int i_val;
};

struct token* lex(const char *filename_local, FILE *fin, size_t *len);
void lex_free(struct token *ts, size_t t_count);

#endif /* LEX_H */
