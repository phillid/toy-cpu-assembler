#include <stdio.h>
#include <stdint.h>

#include "lex.h"
#include "parse.h"
#include "instruction.h"
#include "output.h"

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

	/* FIXME package these things into `tok_result`, `parse_result` etc */
	struct instruction *insts;
	size_t insts_count;
	struct label *labels;
	size_t labels_count;
	if (ret = parse(argv[1], fin, &labels, &labels_count, tokens, tok_count, &insts, &insts_count))
		return ret;

	/* FIXME insert pass for sanity checking identifiers, sizes of values */

	/* FIXME insert optional pass for optimisation */

	if (ret = output(fout, labels, labels_count, insts, insts_count))
		return ret;

	return 0;
}
