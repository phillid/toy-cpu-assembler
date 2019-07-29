#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "util.h"

void print_help(const char *argv0)
{
	fprintf(stderr, "Syntax: %s <in.bin>\0 <out.asm>\n", argv0);
}

int disasm_rtype(uint16_t i, uint16_t unused)
{
	const char *oper = get_asm_from_oper(GET_OPER(i));
	const char *dest = get_asm_from_reg(GET_REG_DEST(i));
	const char *left = get_asm_from_reg(GET_REG_LEFT(i));
	const char *right = get_asm_from_reg(GET_REG_RIGHT(i));

	/* FIXME add special cases:
	 *  - nop
	 *  - neg
	 *  - mv
	 */
	printf("%s  %s, %s, %s\n", oper, dest, left, right);

	return 0;
}

int disasm_nitype(uint16_t i, uint16_t unused)
{
	const char *oper = get_asm_from_oper(GET_OPER(i));
	const char *dest = get_asm_from_reg(GET_REG_DEST(i));
	const char *left = get_asm_from_reg(GET_REG_LEFT(i));
	uint16_t imm = GET_NI_IMM(i);
	/** FIXME add special cases:
	 *  - ldi
	 */
	/* FIXME review sign around immediate value */
	printf("%si %s, %s, 0x%x\n", oper, dest, left, imm);
	return 0;
}

int disasm_witype(uint16_t i, uint16_t unused)
{
	const char *oper = get_asm_from_oper(GET_OPER(i));
	const char *dest = get_asm_from_reg(GET_REG_DEST(i));
	const char *left = get_asm_from_reg(GET_REG_LEFT(i));
	uint16_t imm = GET_NI_IMM(i);
	/* FIXME handle wide imm value */
	printf("%si, %s, %s, 0x%x\n", "oper", dest, left, "?");
	return 0;
}

int disasm_jreg(uint16_t i, uint16_t unused)
{
	const char *inst = get_asm_from_j(GET_JB_COND(i));
	const char *reg = get_asm_from_reg(GET_JUMP_REG(i));
	printf("%s  %s\n", inst, reg);
	return 0;
}

int disasm_bimm(uint16_t i, uint16_t unused)
{
	const char *inst = get_asm_from_b(GET_JB_COND(i));
	/* FIXME immediate value is meant to be signed */
	printf("%s  0x%x\n", inst, GET_B_OFFSET(i));
	return 0;
}

int disasm_jimm(uint16_t i, uint16_t imm)
{
	const char *inst = get_asm_from_j(GET_JB_COND(i));
	printf("%s  0x%x\n", inst, imm);
	return 0;
}

int disasm(FILE *f)
{
	int ret = 0;
	size_t offs = 0;
	size_t extra_read = 0;
	uint16_t inst = 0;
	uint8_t c[2] = { 0 };
	int (*disasm_inst)(uint16_t, uint16_t);

	while (!feof(f) && fread(c, sizeof(c), 1, f) == 1) {
		extra_read = 0;
		inst = c[0] << 8 | c[1];
		switch (GET_INST_TYPE(inst)) {
			case INST_TYPE_RTYPE:
				disasm_inst = disasm_rtype;
				break;
			case INST_TYPE_NITYPE:
				disasm_inst = disasm_nitype;
				break;
			case INST_TYPE_WITYPE:
				if (fread(c, sizeof(c), 1, f) != 1) {
					ret = -errno;
					break;
				}
				extra_read = sizeof(c);
				disasm_inst = disasm_witype;
				break;
			case INST_TYPE_JTYPE:
				/* J Type can be split into three further subtypes:
				 *  - branch (always immediate, 2 bytes)
				 *  - jump reg (2 bytes)
				 *  - jump immediate (4 bytes)
				 */
				if (inst & MASK_IS_BRANCH) {
					disasm_inst = disasm_bimm;
				} else {
					if (inst & MASK_JR) {
						disasm_inst = disasm_jreg;
					} else {
						if (fread(c, sizeof(c), 1, f) != 1) {
							ret = -errno;
							break;
						}
						extra_read = sizeof(c);
						disasm_inst = disasm_jimm;
					}
				}
				break;
			default:
				printf("Unhandled instruction at byte %zd\n", offs);
				ret = -1;
				break;
		}
		/* Fall out of loop if picking a handler errored */
		if (ret) {
			break;
		}
		printf("%04x:\t", offs);
		ret = disasm_inst(inst, c[0] << 8 | c[1]);
		if (ret < 0) {
			fprintf(stderr, "Error handling instruction at byte %zd\n", offs);
			break;
		}
		offs += sizeof(c) + extra_read;
	}

	if (!feof(f)) {
		perror("fread");
		ret = errno;
	} else {
		/* print out final, empty label */
		printf("%04x:\n", offs);
	}

	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;
	FILE *fin = NULL;

	if (argc < 2) {
		print_help(argv[0]);
		return 1;
	}

	if (!(fin = fopen(argv[1], "rb"))) {
		perror("fopen");
		return 1;
	}

	ret = disasm(fin);

	fclose(fin);
	return ret;
}
