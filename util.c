#include <string.h>
#include <ctype.h>

#include "instruction.h"
#include "lex.h"

/**
 * Keywords
 */
static struct {
	int look;
	const char *str;
} keywords[] = {
	{ .look = 0, .str = "declare" },
	{ .look = 0, .str = "byte"    },
	{ .look = 0, .str = "bytes"   },
	{ .look = 0, .str = "word"    },
	{ .look = 0, .str = "words"   },
	{ .look = 0, .str = "base"    },
	{ .str = NULL },
};

/**
  * Human-readable descriptions for tokens
  */
static struct {
	enum TOKEN_TYPE look;
	const char *str;
} token_to_desc[] = {
	{ .look = TOKEN_REGISTER, .str = "register"            },
	{ .look = TOKEN_NUMERIC , .str = "numeric literal"     },
	{ .look = TOKEN_KEYWORD , .str = "keyword"             },
	{ .look = TOKEN_STRING  , .str = "string literal"      },
	{ .look = TOKEN_COMMA   , .str = "comma"               },
	{ .look = TOKEN_LABEL   , .str = "label"               },
	{ .look = TOKEN_IDENT   , .str = "identifier"          },
	{ .look = TOKEN_DOT     , .str = "assembler directive" },
	{ .look = TOKEN_EOL     , .str = "end of line"         },
	{ .str = NULL },
};

/**
 * ALU operation to assembly instruction
 */
static struct {
	enum OPER look;
	const char *str;
} oper_to_asm[] = {
	{ .look = OPER_ADD, .str = "add" },
	{ .look = OPER_SUB, .str = "sub" },
	{ .look = OPER_SHL, .str = "shl" },
	{ .look = OPER_SHR, .str = "shr" },
	{ .look = OPER_AND, .str = "and" },
	{ .look = OPER_OR , .str = "or"  },
	{ .look = OPER_XOR, .str = "xor" },
	{ .look = OPER_MUL, .str = "mul" },
	{ .str = NULL },
};

/**
 * Jump condition to jump assembly instruction
 */
static struct {
	enum JCOND look;
	const char *str;
} j_to_asm[] = {
	{ .look = JB_UNCOND , .str = "jmp"  },
	{ .look = JB_NEVER  , .str = "jn"   },
	{ .look = JB_ZERO   , .str = "jz"   },
	{ .look = JB_NZERO  , .str = "jnz"  },
	{ .look = JB_CARRY  , .str = "jc"   },
	{ .look = JB_NCARRY , .str = "jnc"  },
	{ .look = JB_CARRYZ , .str = "jcz"  },
	{ .look = JB_NCARRYZ, .str = "jncz" },
	{ .str = NULL },
};

/**
 * Jump condition to branch assembly instruction
 */
static struct {
	enum JCOND look;
	const char *str;
} b_to_asm[] = {
	{ .look = JB_UNCOND , .str = "bra"  },
	{ .look = JB_NEVER  , .str = "bn"   },
	{ .look = JB_ZERO   , .str = "bz"   },
	{ .look = JB_NZERO  , .str = "bnz"  },
	{ .look = JB_CARRY  , .str = "bc"   },
	{ .look = JB_NCARRY , .str = "bnc"  },
	{ .look = JB_CARRYZ , .str = "bcz"  },
	{ .look = JB_NCARRYZ, .str = "bncz" },
	{ .str = NULL },
};

/**
 * Register number to assembly representation
 */
static struct {
	enum REG look;
	const char *str;
} reg_to_asm[] = {
	{ .look = REG_0, .str = "$0" },
	{ .look = REG_1, .str = "$1" },
	{ .look = REG_2, .str = "$2" },
	{ .look = REG_3, .str = "$3" },
	{ .look = REG_4, .str = "$4" },
	{ .look = REG_5, .str = "$5" },
	{ .look = REG_6, .str = "$6" },
	{ .look = REG_H, .str = "$H" },
	{ .str = NULL },
};

/* Generates a function that takes an enum value from the given type and looks
 * it up in the given lookup table, returning a string that matches it from
 * the table, or NULL if no such string exists */
#define GENERATE_STR_LOOKUP_FUNC(name, lookup, type) \
const char* name(type x) { \
	size_t i = 0; \
	for (i = 0; lookup[i].str; i++) \
		if (lookup[i].look == x) \
			return lookup[i].str; \
	return NULL; \
}

/* Inverse of GENERATE_STR_LOOKUP_FUNC - this generates a function that takes
 * a string and places in *res an enum value matching that string as entered
 * in the given lookup table.
 * Returns zero on match
 * Returns non-zero on no match */
#define GENERATE_NUM_LOOKUP_FUNC(name, lookup, type) \
int name(const char *x, type *res) { \
	size_t i = 0; \
	for (i = 0; lookup[i].str; i++) \
		if (strcmp(lookup[i].str, x) == 0) { \
			if (res) \
				*res = lookup[i].look; \
			return 0; \
		} \
	return 1; \
}

GENERATE_STR_LOOKUP_FUNC(get_asm_from_oper, oper_to_asm, enum OPER);
GENERATE_STR_LOOKUP_FUNC(get_asm_from_j, j_to_asm, enum JCOND);
GENERATE_STR_LOOKUP_FUNC(get_asm_from_b, b_to_asm, enum JCOND);
GENERATE_STR_LOOKUP_FUNC(get_asm_from_reg, reg_to_asm, enum REG);
GENERATE_STR_LOOKUP_FUNC(get_token_description, token_to_desc, enum TOKEN_TYPE);

GENERATE_NUM_LOOKUP_FUNC(get_keyword, keywords, int);
GENERATE_NUM_LOOKUP_FUNC(get_oper_from_asm, oper_to_asm, enum OPER);
GENERATE_NUM_LOOKUP_FUNC(get_j_from_asm, j_to_asm, enum JCOND);
GENERATE_NUM_LOOKUP_FUNC(get_b_from_asm, b_to_asm, enum JCOND);
GENERATE_NUM_LOOKUP_FUNC(get_reg_from_asm, reg_to_asm, enum REG);


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
