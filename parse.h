#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>
#include <stdbool.h>

#include "lex.h"
#include "instruction.h"

struct label {
	char *name;
	size_t byte_offset;
};

union immediate {
	const char *label;
	uint16_t value;
};

struct r_type {
	enum OPER oper;
	enum REG dest;
	enum REG left;
	enum REG right;
};

struct i_type {
	enum OPER oper;
	enum REG dest;
	enum REG left;
	bool imm_is_ident;
	union immediate imm;
};

struct jr_type {
	enum JCOND cond;
	enum REG reg;
};

struct ji_type {
	enum JCOND cond;
	bool imm_is_ident;
	union immediate imm;
};

struct b_type {
	enum JCOND cond;
	bool imm_is_ident;
	union immediate imm;
};

struct instruction {
	enum INST_TYPE type;
	union instruction_u {
		struct r_type r;   /* catch-all R-Type */
		struct i_type i;   /* I-type on immediate literal */
		struct jr_type jr; /* jump to register */
		struct ji_type ji; /* jump to immediate */
		struct b_type b;   /* branch to immediate literal */
	} inst;
};

int parse(const char *filename_local, FILE *fd, struct label **labels_local, size_t *labels_count_local, struct token *tokens, size_t tokens_count, struct instruction **instructions, size_t *instructions_count);
void parse_free(struct instruction *is, size_t i_count, struct label *ls, size_t l_count);

#endif /* PARSE_H */
