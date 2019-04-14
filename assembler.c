#include <stdio.h>
#include <stdint.h>

#include "lex.h"
#include "parse.h"
#include "instruction.h"

#if 0
/**
 * Types for intermediate storage of instructions
 */
struct r_type {
	enum OPER operation;
	enum REG dest;
	enum REG left;
	enum REG right;
};

struct i_type { /* covers WI and NI */
	enum OPER operation;
	enum REG dest;
	enum REG left;
	int16_t immediate;
};

struct jr_type {
	enum JCOND condition;
	enum REG reg;
};

struct ji_type {
	enum JCOND condition;
	uint16_t immediate;
};

struct b_type { /* FIXME merge with ji_type? */
	enum JCOND condition;
	uint16_t immediate; /* capped to 10 bits by IS */
};

/* Union for bringing above together */
union instruction_union {
	struct r_type r;
	struct i_type i;
	struct jr_type jr;
	struct ji_type ji;
	struct b_type b;
};

struct instruction {
	enum INST_TYPE type;
	union instruction_union i;
};
/**/
#endif

int main(int argc, char **argv)
{
	int ret = 0;
	FILE *fin = NULL;
	FILE *fout = NULL;

	if (argc < 3) {
		fprintf(stderr, "Syntax: %s <in.asm> <out.bin>\n", argv[0]);
		return 1;
	}

	if ((fin = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Error opening %s: ", argv[1]);
		perror("fopen");
		return 1;
	}

	if ((fout = fopen(argv[2], "wb")) == NULL) {
		fprintf(stderr, "Error opening %s: ", argv[2]);
		perror("fopen");
		return 1;
	}
/****/
	struct token *tokens = NULL;
	size_t tok_count = 0;

	if ((tokens = lex(argv[1], fin, &tok_count)) == NULL)
		return 2;

	struct instruction *insts;
	size_t insts_count;
	struct label *labels;
	size_t labels_count;
	if (ret = parse(argv[1], fin, &labels, &labels_count, tokens, tok_count, &insts, &insts_count))
		return ret;

	if (ret = output(fout, labels, labels_count, insts, insts_count))
		return ret;

	return 0;
}
