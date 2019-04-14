#include <string.h>
#include <ctype.h>

#include "lex.h"

const char *tok_to_desc[] = {
	[TOKEN_REGISTER] = "register",
	[TOKEN_NUMERIC] = "numeric literal",
	[TOKEN_KEYWORD] = "keyword",
	[TOKEN_STRING] = "string literal",
	[TOKEN_COMMA] = "comma",
	[TOKEN_LABEL] = "label",
	[TOKEN_IDENT] = "identifier",
	[TOKEN_DOT] = "assembler directive",
	[TOKEN_EOL] = "end of line",
};

const char * get_token_description(enum TOKEN_TYPE t)
{
	if (t < 0 || t >= sizeof(tok_to_desc)/sizeof(*tok_to_desc)) {
		return "[internal error]";
	} else {
		return tok_to_desc[t];
	}
}

void indicate_file_area(FILE* fd, size_t line, size_t column, size_t span)
{
	size_t i = 0;
	const char margin[] = "  ";

	char buf[1024] = { '\0' };
	char *s = buf;
	char c = '\0';

	rewind(fd);
	while (line && !feof(fd) && fgets(buf, sizeof(buf), fd)) {
		s = buf;
		while (*s) {
			if (*(s++) == '\n') {
				line--;
			}
		}
	}

	/* trim leading whitespace */
	s = buf;
	while (*s == '\t' || *s == ' ') {
		s++;
	}

	/* filter non-printables to spaces to keep alignment correct */
	for (i = 0; i < strlen(s); i++) {
		if (!isprint(s[i]) && s[i] != '\n') {
			s[i] = ' ';
		}
	}

	fputs(margin, stderr);
	fputs(s, stderr);

	/* corner case (still needed?) - buf was just return */
	if (strlen(buf) == 1 && buf[0] == '\n') {
		fputc('\n', stderr);
	}

	fputs(margin, stderr);
	column -= (s - buf);
	for (column--; column; column--) {
		fputc(' ', stderr);
	}

	c = span == 1 ? '^' : '"';
	for (; span; span--) {
		fputc(c, stderr);
	}
	fputc('\n', stderr);
}
