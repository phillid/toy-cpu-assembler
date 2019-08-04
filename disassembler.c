#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "instruction.h"
#include "parse.h"
#include "input/input_bin.h"
#include "output/output_asm.h"

void print_help(const char *argv0)
{
	fprintf(stderr, "Syntax: %s <in.bin> <out.asm>\n", argv0);
}

int main(int argc, char **argv)
{
	int error_ret = 1;
	int ret = 0;
	const char *path_in = NULL;
	const char *path_out = NULL;
	FILE *fin = NULL;
	FILE *fout = NULL;

	if (argc < 3) {
		print_help(argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-q") == 0) {
		if (argc != 4) {
			print_help(argv[0]);
			return 0;
		}
		error_ret = 0;
		path_in = argv[2];
		path_out = argv[3];
	} else {
		path_in = argv[1];
		path_out = argv[2];
	}


	if ((fin = fopen(path_in, "r")) == NULL) {
		fprintf(stderr, "Error opening %s: ", path_in);
		perror("fopen");
		return error_ret;
	}

	if ((fout = fopen(path_out, "w")) == NULL) {
		fprintf(stderr, "Error opening %s: ", path_out);
		perror("fopen");
		return error_ret;
	}

/****/
	/* FIXME package these things into `tok_result`, parse_result` etc */
	struct instruction *insts;
	size_t insts_count;
	struct label *labels;
	size_t labels_count;
	labels = NULL;
	labels_count = 0;

	if ((ret = input_bin(fin, &insts, &insts_count)))
		return error_ret && ret;

	fclose(fin);

	if ((ret = output_asm(fout, labels, labels_count, insts, insts_count)))
		return error_ret && ret;

	parse_free(insts, insts_count, NULL, 0);
	fclose(fout);
	return 0;
}
