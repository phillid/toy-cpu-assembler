#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "lex.h"
#include "parse.h"
#include "instruction.h"
#include "util.h"

static const char *filename;
static FILE *fd;
static struct token *cursor;
static struct token *tokens;
static size_t tokens_pos;
static size_t tokens_count;
static struct label *labels;
static size_t labels_count;
static struct instruction *insts;
static size_t insts_count;
static size_t byte_offset;

void emit(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if (cursor) {
		fprintf(stderr, "%s at (%zd,%zd): ", filename, cursor->line, cursor->column);
		vfprintf(stderr, fmt, args);
		indicate_file_area(fd, cursor->line, cursor->column, cursor->span);
	} else {
		fprintf(stderr, "%s: ", filename);
		vfprintf(stderr, fmt, args);
	}
	va_end(args);
}

#define EXPECT_AND_DISCARD_CRITICAL(type)\
	do {                                 \
		EXPECT_CRITICAL(type)            \
		kerchunk();                      \
	} while (0);

#define EXPECT_CRITICAL(type)\
	if (expect(type)) {  \
		return 1;        \
	}

static int expect(enum TOKEN_TYPE e)
{
	const char *expected_desc = "(internal error)";
	const char *observed_desc = "(internal error)";

	if (!cursor || cursor->type != e) {
		expected_desc = get_token_description(e);
		if (cursor) {
			observed_desc = get_token_description(cursor->type);
		} else {
			observed_desc = "end of file";
		}
		emit("Error: Expected %s, got %s\n", expected_desc, observed_desc);
		return 1;
	}

	return 0;
}

void kerchunk()
{
	if (tokens_pos < tokens_count - 1) {
		cursor = &tokens[++tokens_pos];
	} else {
		cursor = NULL;
	}
}

int parse_eol(void)
{
	EXPECT_AND_DISCARD_CRITICAL(TOKEN_EOL);
	return 0;
}

int parse_comma(void)
{
	EXPECT_AND_DISCARD_CRITICAL(TOKEN_COMMA);
	return 0;
}

int parse_imm(uint16_t *imm)
{
	EXPECT_CRITICAL(TOKEN_NUMERIC);
	/* FIXME allow identifiers? or is that job of parent */
	*imm = cursor->i_val;
	kerchunk();
	return 0;
}

int parse_ident(char **ident)
{
	EXPECT_CRITICAL(TOKEN_IDENT);
	*ident = cursor->s_val;
	kerchunk();
	return 0;
}

/**
 * FIXME move */

int add_instruction(struct instruction inst)
{
	struct instruction *old_insts = insts;
	insts = realloc(insts, (insts_count + 1) * sizeof(struct instruction));
	if (!insts) {
		free(old_insts);
		perror("realloc");
		return 1;
	}

	insts[insts_count] = inst;

	insts_count++;
	return 0;
}

int new_label(struct label *dest, const char *name)
{
	char *name_clone = strdup(name);

	if (!name_clone) {
		perror("strdup");
		return 1;
	}

	dest->name = name_clone;
	dest->byte_offset = byte_offset;

	return 0;
}

void destroy_label(struct label *l)
{
	free(l->name);
}
/**/

int parse_label()
{
	size_t i = 0;
	struct label l;
	struct label *old_labels = labels;

	EXPECT_CRITICAL(TOKEN_LABEL);

	for (i = 0; i < labels_count; i++) {
		if (strcmp(labels[i].name, cursor->s_val) == 0) {
			emit("Error: duplicate label\n");
			return 1;
		}
	}

	labels = realloc(labels, (labels_count + 1) * sizeof(struct label));
	if (!labels) {
		perror("realloc");
		free(old_labels);
		return 1;
	}

	if (new_label(&l, cursor->s_val))
		return 1;

	labels[labels_count] = l;

	labels_count++;
	kerchunk();
	return 0;
}

int parse_reg(enum REG *reg)
{
	EXPECT_CRITICAL(TOKEN_REGISTER);
	/* valid registers are: $0, $1, $2, $3, $4, $5, $6, $7, $Z, $H
	 * the latter two are aliases for $0 and $7 respectively
	 */
	if (strlen(cursor->s_val) != 1) {
		emit("Error: incorrect register name length (%d)\n", strlen(cursor->s_val));
		return 1;
	}

	switch (cursor->s_val[0])
	{
		case 'Z': /* fallthrough */
		case 'z': /* fallthrough */
		case '0': *reg = REG_0; break;
		case '1': *reg = REG_1; break;
		case '2': *reg = REG_2; break;
		case '3': *reg = REG_3; break;
		case '4': *reg = REG_4; break;
		case '5': *reg = REG_5; break;
		case '6': *reg = REG_6; break;
		case 'h': /* fallthrough */
		case 'H': /* fallthrough */
		case '7': *reg = REG_H; break;
		default:
			emit("Error: unknown register '%c'\n", cursor->s_val[0]);
			return 1;
	}
	kerchunk();
	return 0;
}

int parse_i_type(enum OPER oper, enum REG dest, enum REG left, uint16_t imm)
{
//	fprintf(stderr, "<DEBUG>: ITYPE %s <%s> <%s> <%d>\n",
//		oper_to_human[oper],
//		reg_to_human[dest],
//		reg_to_human[left],
//		imm);
	struct instruction i;
	i.type = INST_TYPE_NI;
	i.inst.i.oper = oper;
	i.inst.i.dest = dest;
	i.inst.i.left = left;
	i.inst.i.imm_is_ident = false;
	i.inst.i.imm.value = imm;

	if (add_instruction(i))
		return 1;

	/* FIXME detect narrow/wide */
	byte_offset += 2;
	return 0;
}

int parse_i_ident_type(enum OPER oper, enum REG dest, enum REG left, char *ident)
{
	struct instruction i;
	i.type = INST_TYPE_NI;
	i.inst.i.oper = oper;
	i.inst.i.dest = dest;
	i.inst.i.left = left;
	i.inst.i.imm_is_ident = true;
	i.inst.i.imm.label = ident;

	if (add_instruction(i))
		return 1;

	/* FIXME detect narrow/wide */
	byte_offset += 2;
	return 0;
}

int parse_r_type(enum OPER oper, enum REG dest, enum REG left, enum REG right)
{
//	fprintf(stderr, "<DEBUG>: RTYPE %s <%s> <%s> <%s>\n",
//		oper_to_human[oper],
//		reg_to_human[dest],
//		reg_to_human[left],
//		reg_to_human[right]);

	struct instruction i;
	i.type = INST_TYPE_R;
	i.inst.r.oper = oper;
	i.inst.r.dest = dest;
	i.inst.r.left = left;
	i.inst.r.right = right;

	if (add_instruction(i))
		return 1;

	/* FIXME #define */
	byte_offset += 2;
	return 0;
}

int parse_j_reg_type(enum JCOND cond, enum REG reg)
{
//	fprintf(stderr, "<DEBUG>: JRTYPE %s <%s>\n",
//		j_to_human[cond],
//		reg_to_human[reg]);

	struct instruction i;
	i.type = INST_TYPE_JR;
	i.inst.jr.cond = cond;
	i.inst.jr.reg = reg;

	if (add_instruction(i))
		return 1;

	/* FIXME #define */
	byte_offset += 2;
	return 0;
}

int parse_j_imm_type(enum JCOND cond, uint16_t imm)
{
//	fprintf(stderr, "<DEBUG>: JITYPE %s <0x%04x>\n",
//		j_to_human[cond],
//		imm);

	struct instruction i;

	i.type = INST_TYPE_JI;
	i.inst.ji.cond = cond;
	i.inst.ji.imm_is_ident = false;
	i.inst.ji.imm.value = imm;

	if (add_instruction(i))
		return 1;

	/* FIXME #define */
	byte_offset += 4;
	return 0;
}

int parse_j_ident_type(enum JCOND cond, char *ident)
{
//	fprintf(stderr, "<DEBUG>: JTYPE %s <%s>\n",
//		b_to_human[cond],
//		ident);
	struct instruction i;

	i.type = INST_TYPE_JI;
	i.inst.ji.cond = cond;
	i.inst.ji.imm_is_ident = true;
	i.inst.ji.imm.label = ident;

	if (add_instruction(i))
		return 1;

	/* FIXME #define */
	byte_offset += 4;
	return 0;
}

int parse_b_imm_type(enum JCOND cond, int16_t imm)
{
//	fprintf(stderr, "<DEBUG>: BTYPE %s <0x%04x>\n",
//		b_to_human[cond],
//		imm);
	struct instruction i;

	i.type = INST_TYPE_B;
	i.inst.b.cond = cond;
	i.inst.b.imm_is_ident = false;
	i.inst.b.imm.value = imm;

	if (add_instruction(i))
		return 1;

	/* FIXME #define */
	byte_offset += 2;
	return 0;
}

int parse_b_ident_type(enum JCOND cond, char *ident)
{
//	fprintf(stderr, "<DEBUG>: BTYPE %s <%s>\n",
//		b_to_human[cond],
//		ident);
	struct instruction i;

	i.type = INST_TYPE_B;
	i.inst.b.cond = cond;
	i.inst.b.imm_is_ident = true;
	i.inst.b.imm.label = ident;

	if (add_instruction(i))
		return 1;

	/* FIXME #define */
	byte_offset += 2;
	return 0;
}

int parse_instruction(void)
{
	enum REG reg_left;
	enum REG reg_right;
	enum REG reg;
	uint16_t imm;
	char *ident = NULL;
	/**
	 * Based on the operands in assembly, instructions fall into 6 categories:
	 *
	 * REG, REG, REG (verbose R-Type)
	 * REG, REG, IMM (verbose I-Type)
	 * REG, REG      (terse R-type (alias), e.g. `ld $2, $3`)
	 * REG, IMM      (terse I-Type (alias), e.g. `ldi $2, 100`)
	 * REG           (very terse R-type (alias), e.g. `not $2`, OR J-Type)
	 * IMM			 j-type
	 * (none)        (e.g. `nop` (virtual))
	 */
	/* Special cases: catch alias instructions first */
	if (strcmp(cursor->s_val, "nop") == 0) {
		/* `nop` => `add $0,$0,$0` */
		kerchunk();
		if (parse_eol())
			return 1;
		return parse_r_type(OPER_ADD, REG_0, REG_0, REG_0);
	} else if (strcmp(cursor->s_val, "not") == 0) {
		/* `not $1` => `xor $1, $1, $H` */
		kerchunk();
		if (parse_reg(&reg) || parse_eol())
			return 1;
		return parse_r_type(OPER_XOR, reg, reg, REG_H);
	} else if (strcmp(cursor->s_val, "neg") == 0) {
		/* `neg $1` => `sub $1, $0, $1` */
		kerchunk();
		if (parse_reg(&reg) || parse_eol())
			return 1;
		return parse_r_type(OPER_SUB, reg, REG_0, reg);
	} else if (strcmp(cursor->s_val, "mv") == 0) {
		/* `mv $1,$2` => `add $1,$2,$0` */
		kerchunk();
		if (parse_reg(&reg_left) || parse_comma() || parse_reg(&reg_right) || parse_eol())
			return 1;
		return parse_r_type(OPER_ADD, reg_left, reg_right, REG_0);
	} else if (strcmp(cursor->s_val, "ldi") == 0) {
		/* `ldi $1,1234` => `addi $1,$0,1234` */
		kerchunk();
		if (parse_reg(&reg) || parse_comma())
			return 1;

		switch (cursor->type) {
			case TOKEN_NUMERIC:
				if (parse_imm(&imm) || parse_eol())
					return 1;
				return parse_i_type(OPER_ADD, reg, REG_0, imm);
			case TOKEN_IDENT:
				if (parse_ident(&ident) || parse_eol())
					return 1;
				return parse_i_ident_type(OPER_ADD, reg, REG_0, ident);
			default:
				emit("Error: Expected numeric literal or identifier, got %s\n",
					get_token_description(cursor->type));
				return 1;
		}
	}

	/* fallthrough: cursor is *not* pointing at an alias instruction, we can
	 * parse it like normal */

	enum OPER op;
	if (get_oper_from_asm(cursor->s_val, &op) == 0) {
		kerchunk();
		if (   parse_reg(&reg) || parse_comma()
			|| parse_reg(&reg_left) || parse_comma()
			|| parse_reg(&reg_right)
			|| parse_eol())
			return 1;
		return parse_r_type(op, reg, reg_left, reg_right);
	}
	if (cursor->s_val[strlen(cursor->s_val) - 1] == 'i') {
		/* temporarily remove 'i' from end */
		cursor->s_val[strlen(cursor->s_val) - 1] = '\0';

		if ((get_oper_from_asm(cursor->s_val, &op)) == 0) {
			kerchunk();
			if (   parse_reg(&reg) || parse_comma()
				|| parse_reg(&reg_left) || parse_comma())
				return 1;

			switch (cursor->type) {
				case TOKEN_NUMERIC:
					if (parse_imm(&imm) || parse_eol())
						return 1;
					return parse_i_type(op, reg, reg_left, imm);
				case TOKEN_IDENT:
					if (parse_ident(&ident) || parse_eol())
						return 1;
					return parse_i_ident_type(op, reg, reg_left, ident);
				default:
					emit("Error: Expected numeric literal or identifier, got %s\n",
						get_token_description(cursor->type));
					return 1;
			}
		}
		/* fallthrough: pop it back on, we might need it */
		cursor->s_val[strlen(cursor->s_val)] = 'i';
	}

	enum JCOND cond;
	if (get_j_from_asm(cursor->s_val, &cond) == 0) {
		kerchunk();
		switch (cursor->type) {
			case TOKEN_REGISTER:
				if (parse_reg(&reg) || parse_eol())
					return 1;
				return parse_j_reg_type(cond, reg);
			case TOKEN_NUMERIC:
				if (parse_imm(&imm) || parse_eol())
					return 1;
				return parse_j_imm_type(cond, imm);
			case TOKEN_IDENT:
				if (parse_ident(&ident) || parse_eol())
					return 1;
				return parse_j_ident_type(cond, ident);
			default:
				emit("Error: Expected register, numeric literal, or identifier, got %s\n",
					get_token_description(cursor->type));
				return 1;
		}
	}

	if (get_b_from_asm(cursor->s_val, &cond) == 0) {
		kerchunk();
		switch (cursor->type) {
			case TOKEN_NUMERIC:
				if (parse_imm(&imm) || parse_eol())
					return 1;
				return parse_b_imm_type(cond, imm);
			case TOKEN_IDENT:
				if (parse_ident(&ident) || parse_eol())
					return 1;
				return parse_b_ident_type(cond, ident);
			default:
				emit("Error: Expected numeric literal, or identifier, got %s\n",
					get_token_description(cursor->type));
				return 1;
		}
	}

	emit("Unhandled instruction %s\n", cursor->s_val);
	return 1;
}

int parse(const char *filename_local, FILE* fd_local, struct label **labels_local, size_t *labels_count_local, struct token *tokens_local, size_t tokens_count_local, struct instruction **instructions, size_t *instructions_count)
{
	int ret = 0;
	size_t i = 0;
	filename = filename_local;
	fd = fd_local;
	tokens = tokens_local;
	tokens_pos = 0;
	tokens_count = tokens_count_local;
	labels_count = 0;
	insts_count = 0;
	byte_offset = 0;

	cursor = tokens;
	while (cursor) {
		switch(cursor->type) {
			case TOKEN_EOL:
				kerchunk();
				break;
			case TOKEN_DOT:
				/* parse directive */
				kerchunk();
				EXPECT_CRITICAL(TOKEN_KEYWORD);
				if (strcmp(cursor->s_val, "base") == 0) {
					kerchunk();
					EXPECT_CRITICAL(TOKEN_NUMERIC);
					emit("FIXME ignoring base address 0x%04x (%d)\n", cursor->i_val, cursor->i_val);
				}
				EXPECT_AND_DISCARD_CRITICAL(TOKEN_EOL);
				break;
			case TOKEN_LABEL:
				if (parse_label())
					return 1;
				break;
			case TOKEN_IDENT:
				if (parse_instruction())
					return 1;
				break;
			case TOKEN_KEYWORD:
				/* FIXME parse declare bytes etc */
				printf("DEBUG: found keyword `%s'\n", cursor->s_val);
				kerchunk();
				kerchunk();
				kerchunk();
				break;
			default:
				emit("Error: Unhandled %s\n", get_token_description(cursor->type));
				return 1;
		}
	}

	*instructions = insts;
	*instructions_count = insts_count;

	*labels_local = labels;
	*labels_count_local = labels_count;

	return ret;
}
