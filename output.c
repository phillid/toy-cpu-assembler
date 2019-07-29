#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "parse.h"

static size_t cur_byte;

int generate_single_r_type(uint32_t *dest, struct r_type inst)
{
	uint32_t i = 0;

	i |= MASK_INST_RTYPE;
	i |= MASK_OPER(inst.oper);
	i |= MASK_REG_DEST(inst.dest);
	i |= MASK_REG_LEFT(inst.left);
	i |= MASK_REG_RIGHT(inst.right);

	*dest = i;
	return 1;
}
int generate_single_ni_type(uint32_t *dest, struct i_type inst)
{
	uint32_t i = 0;

	i |= MASK_INST_NITYPE;
	i |= MASK_OPER(inst.oper);
	i |= MASK_REG_DEST(inst.dest);
	i |= MASK_REG_LEFT(inst.left);
	i |= MASK_NI_IMM(inst.imm.value);

	*dest = i;
	return 1;
}

int generate_single_wi_type(uint32_t *dest, struct i_type inst)
{
	uint32_t i = 0;

	i |= MASK_INST_WITYPE;
	i |= MASK_OPER(inst.oper);
	i |= MASK_REG_DEST(inst.dest);
	i |= MASK_REG_LEFT(inst.left);

	/* two-word instruction - make room for the immediate */
	i <<= 16;

	i |= inst.imm.value;

	*dest = i;
	return 2;
}

int generate_single_ji_type(uint32_t *dest, struct ji_type inst)
{
	uint32_t i = 0;

	i |= MASK_INST_JTYPE;
	i |= MASK_IS_JUMP;
	i |= MASK_JB_COND(inst.cond);
	i |= MASK_JI;

	/* two-word instruction - make room for the immediate */
	i <<= 16;

	i |= inst.imm.value;

	*dest = i;
	return 2;
}

int generate_single_jr_type(uint32_t *dest, struct jr_type inst)
{
	uint32_t i = 0;

	i |= MASK_INST_JTYPE;
	i |= MASK_IS_JUMP;
	i |= MASK_JB_COND(inst.cond);
	i |= MASK_JR;
	i |= MASK_JUMP_REGISTER(inst.reg);

	*dest = i;
	return 1;
}

int generate_single_b_type(uint32_t *dest, struct b_type inst)
{
	uint32_t i = 0;

	i |= MASK_INST_JTYPE;
	i |= MASK_IS_BRANCH;
	i |= MASK_JB_COND(inst.cond);
	i |= MASK_B_OFFSET(inst.imm.value);

	*dest = i;
	return 1;
}


int look_up_label(struct label *labels, size_t labels_count, uint16_t *val, const char *label)
{
	size_t i = 0;

	for (i = 0; i < labels_count; i++) {
		if (strcmp(labels[i].name, label) == 0) {
			*val = labels[i].byte_offset;
			return 0;
		}
	}

	/* FIXME emit */
	fprintf(stderr, "Reference to undefined label `%s'\n", label);
	return 1;
}

int output_single(FILE *f, struct label *labels, size_t labels_count, struct instruction inst)
{
	int len = 0;
	uint32_t i = 0;
	uint16_t imm = 0;

	switch (inst.type) {
		case INST_TYPE_R:
			len = generate_single_r_type(&i, inst.inst.r);
			break;
		case INST_TYPE_NI:
			if (   inst.inst.i.imm_is_ident
			    && look_up_label(labels, labels_count, &inst.inst.i.imm.value, inst.inst.i.imm.label))
				return 1;

			len = generate_single_ni_type(&i, inst.inst.i);
			break;
		case INST_TYPE_WI:
			if (   inst.inst.i.imm_is_ident
			    && look_up_label(labels, labels_count, &inst.inst.i.imm.value, inst.inst.i.imm.label))
				return 1;

			len = generate_single_wi_type(&i, inst.inst.i);
			break;
		case INST_TYPE_JR:
			len = generate_single_jr_type(&i, inst.inst.jr);
			break;
		case INST_TYPE_JI:
			if (   inst.inst.ji.imm_is_ident
			    && look_up_label(labels, labels_count, &inst.inst.ji.imm.value, inst.inst.ji.imm.label))
				return 1;

			len = generate_single_ji_type(&i, inst.inst.ji);
			break;
		case INST_TYPE_B:
			if (   inst.inst.b.imm_is_ident
			    && look_up_label(labels, labels_count, &inst.inst.b.imm.value, inst.inst.b.imm.label))
				return 1;
			inst.inst.b.imm.value -= cur_byte;
			if (inst.inst.b.imm.value % 2 != 0) {
				fprintf(stderr, "Internal error: branch offset %d not a multiple of 2\n", inst.inst.b.imm.value);
			}
			inst.inst.b.imm.value /= 2;

			len = generate_single_b_type(&i, inst.inst.b);
			break;
		default:
			fprintf(stderr, "Internal error: unhandled instruction type\n");
			break;
	}

	if (len == 2) {
#define RAW
#ifdef RAW
		fputc(0xFF & (i >> 24), f);
		fputc(0xFF & (i >> 16), f);
#else
		fprintf(f, "%04x ", i >> 16);
#endif
	}
#ifdef RAW
	fputc(0xFF & (i >> 8), f);
	fputc(0xFF & (i >> 0), f);
#else
	fprintf(f, "%04x ", 0xFFFF & i);
#endif

	cur_byte += 2 * len;
	return 0;
}

int output(FILE *fout, struct label *labels, size_t label_count, struct instruction *insts, size_t insts_count)
{
	size_t i = 0;
	cur_byte = 0;

#ifndef RAW
	fprintf(fout, "v2.0 raw\n");
#endif

	for (i = 0; i < insts_count; i++)
		if (output_single(fout, labels, label_count, insts[i]))
			return 1;

	return 0;
}
