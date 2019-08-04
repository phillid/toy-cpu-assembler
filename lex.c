#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "lex.h"
#include "util.h"

static const char *filename = NULL;
static FILE *fd;
static size_t line;
static size_t column;
static int context_comment;
static struct token* tokens;
static size_t tokens_count;
static char buffer[1024]; /* XXX limitation: sources must have lines < 1024 bytes */

static void emit(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "%s at (%zd,%zd): ", filename, 1 + line, 1 + column);
	vfprintf(stderr, fmt, args);
	indicate_file_area(fd, 1 + line, 1 + column, 1);
	va_end(args);
}

static int expect(const char c) {
	if (buffer[column] != c) {
		emit("Expected '%c', got '%c'\n", c, buffer[column]);
		return 1;
	}
	column++;
	return 0;
}

static void store_location(struct token *t) {
	t->column = column + 1;
	t->line = line + 1;
}

static void eat_whitespace(void) {
	size_t len = strlen(buffer);
	while (column < len && strchr(" \t", buffer[column])) {
		column++;
	}
}

static int add_token(struct token t) {
	struct token *old_tok = tokens;

	tokens_count++;
	tokens = realloc(tokens, sizeof(struct token) * tokens_count);

	if (!tokens) {
		perror("realloc");
		free(old_tok);
		return 1;
	}

	tokens[tokens_count - 1] = t;
	return 0;
}

static struct token last_token(void) {
	if (tokens_count)
		return tokens[tokens_count - 1];
	else
		return (struct token){ .type = TOKEN_EOL };
}

static int lex_comma(struct token *t) {
	if (expect(','))
		return 1;

	t->span = 1;
	t->type = TOKEN_COMMA;
	return 0;
}

static int lex_dot(struct token *t) {
	if (expect('.'))
		return 1;

	t->span = 1;
	t->type = TOKEN_DOT;
	return 0;
}

static int lex_register(struct token *t) {
	int i = 0;
	if (expect('$'))
		return 1;

	for (i = column; isalnum(buffer[i]); i++) {
		;
	}

	/* -1, +1 to include $ */
	t->s_val = strndup(&buffer[column - 1], i - column + 1);
	if (!t->s_val) {
		perror("strndup");
		return 1;
	}

	t->span = i - column + 1;
	t->type = TOKEN_REGISTER;
	column = i;
	return 0;
}

static int lex_string(struct token *t) {
	int i = 0;
	if (expect('"'))
		return 1;

	for (i = column; buffer[i] != '\0' && buffer[i] != '"'; i++) {
		;
	}

	t->s_val = strndup(&buffer[column], i - column);
	if (!t->s_val) {
		perror("strndup");
		return 1;
	}

	t->span = i - column + 2; /* +2 to include "" */
	t->type = TOKEN_STRING;
	column = i;
	if (expect('"'))
		return 1;

	return 0;
}

static int lex_char_escaped(struct token *t) {
	if (expect('\\'))
		return 1;

	switch (buffer[column]) {
		case 'a': t->i_val = '\a'; break;
		case 'b': t->i_val = '\b'; break;
		case 'f': t->i_val = '\f'; break;
		case 'n': t->i_val = '\n'; break;
		case 'r': t->i_val = '\r'; break;
		case 't': t->i_val = '\t'; break;
		case 'v': t->i_val = '\v'; break;

		case '\\': t->i_val = '\\'; break;
		case '\'': t->i_val = '\''; break;
		default:
			emit("Unknown escape sequence '\\%c'\n", buffer[column]);
			break;
	}
	column++;
	t->type = TOKEN_NUMERIC;
	t->span = 4; /* len '\x' == 4 */
	return 0;
}

static int lex_char(struct token *t) {
	if (expect('\''))
		return 1;

	if (buffer[column] == '\\') {
		lex_char_escaped(t);
	} else {
		t->type = TOKEN_NUMERIC;
		t->span = 3; /* len 'x' == 3 */
		t->i_val = buffer[column++];
	}
	if (expect('\''))
		return 1;

	return 0;
}

static int lex_num(struct token *t)
{
	char *num_s = NULL;
	char *end = NULL;
	size_t span = 0;
	size_t prefix_span = 0;
	int value = 0;
	int base = 0;
	int neg = 0;

	/* shave off a leading '-' now to make handling easier */
	if (buffer[column] == '-') {
		neg = 1;
		if (expect('-'))
			return 1;
		prefix_span++;
	}

	if (!isdigit(buffer[column])) {
		emit("Error: '%c' cannot start a numerical literal\n", buffer[column]);
		return 1;
	}

	/* check if hex */
	if (   column <= strlen(buffer) - 2
	    && buffer[column] == '0'
	    && buffer[column + 1] == 'x') {
		base = 16;
	}

	span = strcspn(&buffer[column], " \n\t,");
	if (span == 0) {
		emit("Error: malformed numerical literal\n");
		return 1;
	}
	num_s = strndup(&buffer[column], span);
	if (!num_s) {
		perror("malloc");
		return 1;
	}

	/* if base still unknown, determine if from the last char of constant */
	char *suffix = &num_s[span - 1];
	if (base == 0) {
		switch (*suffix) {
			case 'h': base = 16; break;
			case 'd': base = 10; break;
			case 'o': base = 8;  break;
			case 'b': base = 2;  break;
			default:
				if (!isdigit(*suffix)) {
					emit("Error: '%c' is an invalid base suffix in numerical literal\n", *suffix);
					free(num_s);
					return 1;
				}
				break;
		}
		if (!isdigit(*suffix)) {
			*suffix = '\0';
		}
	}

	value = strtol(num_s, &end, base);
	if (*end != '\0') {
		emit("Error: malformed numerical literal\n", *end, base);
		free(num_s);
		return 1;
	}
	free(num_s);

	column += span;

	t->type = TOKEN_NUMERIC;
	t->span = prefix_span + span;
	t->i_val = (neg ? -value : value);
	return 0;
}

static int lex_misc(struct token *t) {
	int i = 0;

	for (i = column; isalnum(buffer[i]); i++) {
		;
	}

	if (buffer[i] == ':') {
		t->type = TOKEN_LABEL;
	} else {
		t->type = TOKEN_IDENT;
	}

	t->s_val = strndup(&buffer[column], i - column);
	if (!t->s_val)
		return 1;

	if (get_keyword(t->s_val, NULL) == 0)
		t->type = TOKEN_KEYWORD;

	t->span = i - column;
	column = i;
	/* skip over colon, but don't have included it in the name */
	if (t->type == TOKEN_LABEL) {
		column++;
	} else {
		/* non-labels must start with an alpha */
		if (!isalpha(t->s_val[0])) {
			emit("Error: '%c' cannot start an identifier\n", t->s_val[0]);
			return 1;
		}
	}
	return 0;
}

static int lex_eol(struct token *t) {
	column++;
	t->type = TOKEN_EOL;
	t->span = 1;
	return 0;
}

int lex_line(void) {
	int ret = 0;
	size_t len = strlen(buffer);
	struct token tok;

	while (column < len) {
		/* special case: are we still inside a block comment? */
		if (context_comment) {
			if (   column + 1 < len
			    && buffer[column] == '*'
			    && buffer[column + 1] == '/') {
				context_comment = 0;
				column += 2;
				continue;
			}
			column++;
			continue;
		}

		/* fallthrough: outside a comment */
		memset(&tok, 0, sizeof(tok));
		store_location(&tok);
		switch (buffer[column]) {
			case ';':
			case '#':
			case '!':
			case '\n':
				ret = lex_eol(&tok);
				return add_token(tok);
			case ' ':
			case '\t':
				eat_whitespace();
				continue;
			case '/':
				if (column + 1 < len && buffer[column + 1] == '*') {
					column += 2;
					context_comment = 1;
					continue;
				} else if (column + 1 < len && buffer[column + 1] == '/') {
					ret = lex_eol(&tok);
					column += 2;
					return add_token(tok);
				} else {
					emit("Unexpected '/'\n");
					return 1;
				}
				break;
			case ',':
				ret = lex_comma(&tok);
				break;
			case '.':
				ret = lex_dot(&tok);
				break;
			case '$':
				ret = lex_register(&tok);
				break;
			case '"':
				ret = lex_string(&tok);
				break;
			case '\'':
				ret = lex_char(&tok);
				break;
			case '-':
				ret = lex_num(&tok);
				break;
			/* FIXME add support for expressions like `addi $0, $0, (1+2*3) */
			default:
				/* allow numerical to start label only when at start of line */
				if (isdigit(buffer[column]) && last_token().type != TOKEN_EOL) {
					ret = lex_num(&tok);
				} else {
					ret = lex_misc(&tok);
				}
				break;
		}
		if (ret)
			return ret;

		if (add_token(tok))
			return 1;
	}
	return 0;
}

void lex_free_tok(struct token t)
{
	if (t.s_val)
		free(t.s_val);

}

void lex_free(struct token *ts, size_t t_count)
{
	size_t i = 0;

	for (i = 0; i < t_count; i++) {
		lex_free_tok(ts[i]);
	}

	free(ts);
}

struct token* lex(const char *filename_local, FILE *fin, size_t *len)
{
	filename = filename_local;
	fd = fin;
	line = 0;
	tokens = NULL;
	tokens_count = 0;
	context_comment = 0;

	while (fgets(buffer, sizeof(buffer), fin)) {
		column = 0;
		if (lex_line()) {
			return NULL;
		}
		line++;
	}
	if (!feof(fin)) {
		perror("fgets");
		return NULL;
	}

	if (context_comment) {
		emit("Error: unexpected EOF while still inside block comment\n");
		return NULL;
	}

	/* no tokens? just an EOL then */
	if (tokens_count == 0) {
		struct token eol = {0};
		lex_eol(&eol);
		add_token(eol);
	}

	*len = tokens_count;
	return tokens;
}
