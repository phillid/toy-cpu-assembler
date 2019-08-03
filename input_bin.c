#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "parse.h"

static void disasm_rtype(uint16_t i, uint16_t unused, struct instruction *inst)
{
	(void)unused;
	inst->type = INST_TYPE_R;
	inst->inst.r.oper = GET_OPER(i);
	inst->inst.r.dest = GET_REG_DEST(i);
	inst->inst.r.left = GET_REG_LEFT(i);
	inst->inst.r.right = GET_REG_RIGHT(i);
}

static void disasm_nitype(uint16_t i, uint16_t unused, struct instruction *inst)
{
	(void)unused;
	inst->type = INST_TYPE_NI;
	inst->inst.i.oper = GET_OPER(i);
	inst->inst.i.dest = GET_REG_DEST(i);
	inst->inst.i.left = GET_REG_LEFT(i);
	inst->inst.i.imm.value = GET_NI_IMM(i);
	inst->inst.i.imm_is_ident = 0;
}

static void disasm_witype(uint16_t i, uint16_t imm, struct instruction *inst)
{
	inst->type = INST_TYPE_WI;
	inst->inst.i.oper = GET_OPER(i);
	inst->inst.i.dest = GET_REG_DEST(i);
	inst->inst.i.left = GET_REG_LEFT(i);
	inst->inst.i.imm.value = imm;
	inst->inst.i.imm_is_ident = 0;
}

static void disasm_jreg(uint16_t i, uint16_t unused, struct instruction *inst)
{
	(void)unused;
	inst->type = INST_TYPE_JR;
	inst->inst.jr.cond = GET_JB_COND(i);
	inst->inst.jr.reg = GET_JUMP_REG(i);
}

static void disasm_jimm(uint16_t i, uint16_t imm, struct instruction *inst)
{
	inst->type = INST_TYPE_JI;
	inst->inst.ji.cond= GET_JB_COND(i);
	inst->inst.ji.imm.value = imm;
	inst->inst.ji.imm_is_ident = 0;
}

static void disasm_bimm(uint16_t i, uint16_t pc, struct instruction *inst)
{
	struct { signed int s:10; } sign;
	int offset = sign.s = GET_B_OFFSET(i);
	inst->type = INST_TYPE_B;
	inst->inst.b.cond = GET_JB_COND(i);
	inst->inst.b.imm.value = pc + 2 * offset;
	inst->inst.b.imm_is_ident = 0;
}


/**
 * FIXME move and factor out with parse.c */
static struct instruction *insts = NULL;
static size_t insts_count = 0;
static int add_instruction(struct instruction inst)
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



/* FIXME needs whatsit arguments. f, tok, tok length */
static int disasm_file(FILE *f)
{
	int ret = 0;
	size_t offs = 0;
	uint16_t inst = 0;
	uint8_t c[2] = { 0 };
	size_t extra_read = 0;
	uint16_t extra_arg = 0;
	struct instruction i = { 0 };
	void (*disasm_inst)(uint16_t, uint16_t, struct instruction*);

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
				extra_arg = c[0] << 16 | c[1];
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
					extra_arg = offs;
				} else {
					if (inst & MASK_JR) {
						disasm_inst = disasm_jreg;
					} else {
						if (fread(c, sizeof(c), 1, f) != 1) {
							ret = -errno;
							break;
						}
						extra_arg = c[0] << 16 | c[1];
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
		disasm_inst(inst, extra_arg, &i);
		if (ret < 0) {
			fprintf(stderr, "Error handling instruction at byte %zd\n", offs);
			break;
		}
		if (add_instruction(i))
			return 1;
		offs += sizeof(c) + extra_read;
	}
	if (!feof(f)) {
		perror("fread");
		ret = -errno;
	}
	return ret;
}

int disasm(FILE *f, struct instruction **i, size_t *i_count)
{
	int ret = 0;

	ret = disasm_file(f);
	*i = insts;
	*i_count = insts_count;

	return ret;
}
