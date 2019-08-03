#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "parse.h"
#include "util.h"

static size_t inst_sizes[] = {
	[INST_TYPE_R] = 2,
	[INST_TYPE_NI] = 2,
	[INST_TYPE_WI] = 4,
	[INST_TYPE_JR] = 2,
	[INST_TYPE_JI] = 4,
	[INST_TYPE_B] = 2,
};

void emit_single_r_type(FILE *f, struct r_type inst)
{
	const char *oper = get_asm_from_oper(inst.oper);
	const char *dest = get_asm_from_reg(inst.dest);
	const char *left = get_asm_from_reg(inst.left);
	const char *right = get_asm_from_reg(inst.right);

	fprintf(f, "%s  %s, %s, %s\n", oper, dest, left, right);
}

void emit_single_i_type(FILE *f, struct i_type inst)
{
	const char *oper = get_asm_from_oper(inst.oper);
	const char *dest = get_asm_from_reg(inst.dest);
	const char *left = get_asm_from_reg(inst.left);

	fprintf(f, "%si %s, %s, 0x%x\n", oper, dest, left, inst.imm.value);
}

/*void emit_single_wi_type(FILE *f, struct i_type inst)
{
	const char *oper = get_asm_from_oper(inst.oper);
	const char *dest = get_asm_from_oper(inst.dest);
	const char *left = get_asm_from_oper(inst.left);

	fprintf(f, "%si %s, %s, 0x%s\n", oper, dest, left, inst.imm.value);
}*/

void emit_single_ji_type(FILE *f, struct ji_type inst)
{
	const char *cond = get_asm_from_j(inst.cond);

	fprintf(f, "%s  0x%x\n", cond, inst.imm.value);
}

void emit_single_jr_type(FILE *f, struct jr_type inst)
{
	const char *cond = get_asm_from_j(inst.cond);
	const char *reg = get_asm_from_reg(inst.reg);

	fprintf(f, "%s  %s\n", cond, reg);
}

void emit_single_b_type(FILE *f, struct b_type inst)
{
	const char *cond = get_asm_from_b(inst.cond);

	fprintf(f, "%s  0x%x\n", cond, inst.imm.value);
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

int output_single(FILE *f, size_t *cur_byte, struct label *labels, size_t labels_count, struct instruction inst)
{
	switch (inst.type) {
		case INST_TYPE_R:
			emit_single_r_type(f, inst.inst.r);
			break;
		case INST_TYPE_NI:
		case INST_TYPE_WI:
			if (   inst.inst.i.imm_is_ident
			    && look_up_label(labels, labels_count, &inst.inst.i.imm.value, inst.inst.i.imm.label))
				return 1;

			emit_single_i_type(f, inst.inst.i);
			break;
		case INST_TYPE_JR:
			emit_single_jr_type(f, inst.inst.jr);
			break;
		case INST_TYPE_JI:
			if (   inst.inst.ji.imm_is_ident
			    && look_up_label(labels, labels_count, &inst.inst.ji.imm.value, inst.inst.ji.imm.label))
				return 1;

			emit_single_ji_type(f, inst.inst.ji);
			break;
		case INST_TYPE_B:
			if (   inst.inst.b.imm_is_ident
			    && look_up_label(labels, labels_count, &inst.inst.b.imm.value, inst.inst.b.imm.label))
				return 1;
			emit_single_b_type(f, inst.inst.b);
			break;
		default:
			fprintf(stderr, "Internal error: unhandled instruction type\n");
			break;
	}

	*cur_byte += inst_sizes[inst.type];

	return 0;
}

int output_asm(FILE *fout, struct label *labels, size_t label_count, struct instruction *insts, size_t insts_count)
{
	size_t i = 0;
	size_t cur_byte = 0;

	for (i = 0; i < insts_count; i++)
		if (output_single(fout, &cur_byte, labels, label_count, insts[i]))
			return 1;

	return 0;
}
