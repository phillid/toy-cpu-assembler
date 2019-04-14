#ifndef INSTRUCTION_H
#define INSTRUCTION_H

/**
 * Values used for software-only identification instruction types. Values not
 * tied to machine language. Guaranteed unique.
 */
enum INST_TYPE {
	INST_TYPE_R,
	INST_TYPE_NI,
	INST_TYPE_WI,
	INST_TYPE_JR,
	INST_TYPE_JI,
	INST_TYPE_B
};

/**
 * Masks for all four instruction types. Not guaranteed unique
 */
#define MASK_INST_RTYPE  (0x0000)
#define MASK_INST_NITYPE (0x4000)
#define MASK_INST_WITYPE (0x8000)
#define MASK_INST_JTYPE  (0xC000)

/**
 * ALU operation types
 * R-type and I-type take 3-bit ALU oper as bits:
 * xx___xxx xxxxxxxx
 */
enum OPER {
	OPER_ADD = 0,
	OPER_SUB = 1,
	OPER_SHL = 2,
	OPER_SHR = 3,
	OPER_AND = 4,
	OPER_OR  = 5,
	OPER_XOR = 6,
	OPER_MUL = 7,
};
#define OPER_SHAMT (11)
#define MASK_OPER(x) ((x) << OPER_SHAMT)

static const char *oper_to_human[] = {
	[OPER_ADD] = "add",
	[OPER_SUB] = "sub",
	[OPER_SHL] = "shl",
	[OPER_SHR] = "shr",
	[OPER_AND] = "and",
	[OPER_OR ] = "or",
	[OPER_XOR] = "xor",
	[OPER_MUL] = "mul"
};

/**
 * Masks for jump and branch conditions
 * J-type instructions (jump, branch) take these as follows:
 * xxx___xx xxxxxxxx
 */
enum JCOND {
	JB_UNCOND  = 0x0,
	JB_NEVER   = 0x1,
	JB_ZERO    = 0x2,
	JB_NZERO   = 0x3,
	JB_CARRY   = 0x4,
	JB_NCARRY  = 0x5,
	JB_CARRYZ  = 0x6,
	JB_NCARRYZ = 0x7
};
#define JB_SHAMT   (10)
#define MASK_JB_COND(x) ((x) << JB_SHAMT)
#define MASK_IS_JUMP   (0 << 13)
#define MASK_IS_BRANCH (1 << 13)
#define MASK_JI (0x0 << 8)
#define MASK_JR (0x1 << 8)
#define MASK_JUMP_REGISTER(x) ((x) << 5)

static const char *j_to_human[] = {
	[JB_UNCOND]  = "jmp",
	[JB_NEVER]   = "jn",
	[JB_ZERO]    = "jz",
	[JB_NZERO]   = "jnz",
	[JB_CARRY]   = "jc",
	[JB_NCARRY]  = "jnc",
	[JB_CARRYZ]  = "jcz",
	[JB_NCARRYZ] = "jncz"
};
static const char *b_to_human[] = {
	[JB_UNCOND]  = "bra",
	[JB_NEVER]   = "bn",
	[JB_ZERO]    = "bz",
	[JB_NZERO]   = "bnz",
	[JB_CARRY]   = "bc",
	[JB_NCARRY]  = "bnc",
	[JB_CARRYZ]  = "bcz",
	[JB_NCARRYZ] = "bncz"
};

/**
 * Register numbers used in all manner of instructions in varying positions
 */
enum REG {
	REG_0 = 0,
	REG_1 = 1,
	REG_2 = 2,
	REG_3 = 3,
	REG_4 = 4,
	REG_5 = 5,
	REG_6 = 6,
	REG_H = 7
};

static const char *reg_to_human[] = {
	[REG_0] = "$0",
	[REG_1] = "$1",
	[REG_2] = "$2",
	[REG_3] = "$3",
	[REG_4] = "$4",
	[REG_5] = "$5",
	[REG_6] = "$6",
	[REG_H] = "$H",
};

/**
 * Offset macro to turn REG_* into mask for register operands of R-type and
 * I-type instructions
 */
/* destination reg: xxxxx___ xxxxxxxx */
#define REG_DEST_OFFSET (8)
#define MASK_REG_DEST(x) ((x) << REG_DEST_OFFSET)

/* left reg: xxxxxxxx ___xxxxx */
#define REG_LEFT_OFFSET (5)
#define MASK_REG_LEFT(x) ((x) << REG_LEFT_OFFSET)

/* right reg (R-type only): xxxxxxxx xxx___xx */
#define REG_RIGHT_OFFSET (2)
#define MASK_REG_RIGHT(x) ((x) << REG_RIGHT_OFFSET)

/* five LSb are narrow immediate value */
#define MASK_NI_IMM(x) ((x) & 0x1F)

/* 10 LSb is branch offset */
#define MASK_B_OFFSET(x) ((x) & 0x3FF)

#endif /* INSTRUCTION_H */
