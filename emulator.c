#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "lex.h"
#include "parse.h"
#include "instruction.h"
#include "input/input_bin.h"

//#define DEBUG
#include "debug.h"

/* Macro for safe wrap-around RAM access */
#define RAM_AT(ctx, offs) (ctx->ram[offs % ctx->ram_size])


struct emul_context {
	uint8_t *ram;
	size_t ram_size;
	uint16_t pc;
	bool zf;
	bool cf;
	uint16_t registers[REG_COUNT];
};

int should_jump(struct emul_context *ctx, enum JCOND cond) {
	switch (cond) {
		case JB_UNCOND: return 1;
		case JB_NEVER:  return 0;
		case JB_ZERO:   return (ctx->zf);
		case JB_NZERO:  return !(ctx->zf);
		case JB_CARRY:  return (ctx->cf);
		case JB_NCARRY: return !(ctx->cf);
		case JB_CARRYZ: return (ctx->zf || ctx->cf);
		case JB_NCARRYZ:return (!ctx->zf && !ctx->cf);
		default:
			assert(0);
	}
}

int execute_r(struct emul_context *ctx, struct instruction i)
{
	uint16_t res = 0;
	uint16_t *d = &ctx->registers[i.inst.r.dest];
	uint16_t *l = &ctx->registers[i.inst.r.left];
	uint16_t *r = &ctx->registers[i.inst.r.right];

	switch (i.inst.r.oper) {
		case OPER_ADD:
			res = *l + *r;
			break;
		case OPER_SUB:
			res = *l - *r;
			break;
		case OPER_SHL:
			res = *l << *r;
			break;
		case OPER_SHR:
			res = *l >> *r;
			break;
		case OPER_AND:
			res = *l & *r;
			break;
		case OPER_OR:
			res = *l | *r;
			break;
		case OPER_XOR:
			res = *l ^ *r;
			break;
		case OPER_MUL:
			res = *l * *r;
			break;
		default:
			return 1;
	}

	ctx->zf = (res == 0);
	/* FIXME set cf */

	if (i.inst.r.dest != REG_0 && i.inst.r.dest != REG_H) {
		*d = res;
	}
	return 0;
}

int execute_i(struct emul_context *ctx, struct instruction i)
{
	uint16_t res = 0;
	uint16_t *d = &ctx->registers[i.inst.i.dest];
	uint16_t *l = &ctx->registers[i.inst.i.left];
	uint16_t imm = i.inst.i.imm.value;

	switch (i.inst.i.oper) {
		case OPER_ADD:
			res = *l + imm;
			break;
		case OPER_SUB:
			res = *l - imm;
			break;
		case OPER_SHL:
			res = *l << imm;
			break;
		case OPER_SHR:
			res = *l >> imm;
			break;
		case OPER_AND:
			res = *l & imm;
			break;
		case OPER_OR:
			res = *l | imm;
			break;
		case OPER_XOR:
			res = *l ^ imm;
			break;
		case OPER_MUL:
			res = *l * imm;
			break;
		default:
			return 1;
	}

	ctx->zf = (res == 0);
	/* FIXME set cf */

	if (i.inst.r.dest != REG_0 && i.inst.r.dest != REG_H) {
		*d = res;
	}
	return 0;
}

int execute_jr(struct emul_context *ctx, struct instruction i)
{
	if (should_jump(ctx, i.inst.jr.cond))
		ctx->pc = ctx->registers[i.inst.jr.reg];
	return 0;
}

int execute_ji(struct emul_context *ctx, struct instruction i)
{
	if (should_jump(ctx, i.inst.ji.cond))
		ctx->pc = i.inst.ji.imm.value;
	return 0;
}

int execute_b(struct emul_context *ctx, struct instruction i)
{
	if (should_jump(ctx, i.inst.b.cond))
		ctx->pc -= (int16_t)i.inst.b.imm.value;

	return 0;
}

int execute_single(struct emul_context *ctx)
{
	int ret = 0;
	struct instruction i = { 0 };
	int (*f)(struct emul_context*, struct instruction) = NULL;
	ret = disasm_single(&i, ctx->pc,
		RAM_AT(ctx, ctx->pc    ) << 8 | RAM_AT(ctx, ctx->pc + 1),
		RAM_AT(ctx, ctx->pc + 2) << 8 | RAM_AT(ctx, ctx->pc + 3));
	if (ret <= 0) {
		printf("disasm_single returned %d\n", ret);
		return ret ? ret : -1;
	}

	ctx->pc += ret;
	switch(i.type) {
		case INST_TYPE_R:
			f = execute_r;
			break;
		case INST_TYPE_NI:
		case INST_TYPE_WI:
			f = execute_i;
			break;
		case INST_TYPE_JR:
			f = execute_jr;
			break;
		case INST_TYPE_JI:
			f = execute_ji;
			break;
		case INST_TYPE_B:
			f = execute_b;
			break;
		default:
			fprintf(stderr, "Unhandled instruction '0x%x' at 0x%x (%d), stop.\n",
				ctx->ram[ctx->pc], ctx->pc, ctx->pc);
			return 1;
	}

	ret = f(ctx, i);

	return ret;
}

int emulator_run(uint8_t *ram, size_t ram_size, size_t bytes_used)
{
	int ret = 0;
	struct emul_context ctx = {0};
	ctx.ram = ram;
	ctx.ram_size = ram_size;
	ctx.registers[REG_H] = ~(uint16_t)0;

	for (ctx.pc = 0; ctx.pc < ctx.ram_size && ctx.pc < bytes_used;) {
		if ((ret = execute_single(&ctx))) {
			return ret;
		}
		debug("pc:%d\n", ctx.pc);
	}

	if (ctx.pc >= bytes_used) {
		debug("Fell off the bottom of the given program, stopping.\n");
	} else {
		debug("Fell off the bottom of memory, stopping.\n");
	}

	printf(
		"Registers:\n"
		"pc: 0x%x (%d)\n"
		"$0: 0x%x (%d)\n"
		"$1: 0x%x (%d)\n"
		"$2: 0x%x (%d)\n"
		"$3: 0x%x (%d)\n"
		"$4: 0x%x (%d)\n"
		"$5: 0x%x (%d)\n"
		"$6: 0x%x (%d)\n"
		"$H: 0x%x (%d)\n",
		ctx.pc, ctx.pc,
		ctx.registers[REG_0], ctx.registers[REG_0],
		ctx.registers[REG_1], ctx.registers[REG_1],
		ctx.registers[REG_2], ctx.registers[REG_2],
		ctx.registers[REG_3], ctx.registers[REG_3],
		ctx.registers[REG_4], ctx.registers[REG_4],
		ctx.registers[REG_5], ctx.registers[REG_5],
		ctx.registers[REG_6], ctx.registers[REG_6],
		ctx.registers[REG_H], ctx.registers[REG_H]
	);
	return 0;
}

void print_help(const char *argv0)
{
	fprintf(stderr, "Syntax: %s <in.bin>\n", argv0);
}

int main(int argc, char **argv)
{
	int error_ret = 1;
	int ret = 0;
	const char *path_in = NULL;
	FILE *fin = NULL;

	if (argc < 2) {
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
	} else {
		path_in = argv[1];
	}

	if ((fin = fopen(path_in, "r")) == NULL) {
		fprintf(stderr, "Error opening %s: ", path_in);
		perror("fopen");
		return error_ret;
	}

	uint8_t ram[65536] = { 0 };
	uint16_t bytes_used = 0;
	size_t nread = 0;
	while((nread = fread(ram + bytes_used, 1, 128, fin))) {
		bytes_used += nread;
	}

	if (!feof(fin)) {
		perror("fread");
		return error_ret;
	}
	fclose(fin);

	debug("Read %d bytes of program into memory\n", bytes_used);
	if ((ret = emulator_run(ram, sizeof(ram), bytes_used)))
		return error_ret && ret;

	return 0;
}
