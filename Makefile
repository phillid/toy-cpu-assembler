EXECUTABLES = assembler disassembler emulator asmcat bincat

ASM_OBJECTS = assembler.o lex.o parse.o output/output_bin.o util.o
DISASM_OBJECTS = disassembler.o input/input_bin.o output/output_asm.o util.o
EMUL_OBJECTS = input/input_bin.o util.o
ASMCAT_OBJECTS = asmcat.o lex.o parse.o output/output_asm.o util.o
BINCAT_OBJECTS = bincat.o input/input_bin.o output/output_bin.o util.o

INCLUDE += -I.

CFLAGS += $(INCLUDE) -Wall -Wextra -Wpedantic

all: $(EXECUTABLES)

# Main executables
assembler: $(ASM_OBJECTS)

disassembler: $(DISASM_OBJECTS)

emulator: $(EMUL_OBJECTS)

asmcat: $(ASMCAT_OBJECTS)

bincat: $(BINCAT_OBJECTS)

# Utils: FIXME lex and parse should be input?
lex.o: lex.h

parse.o: lex.h parse.h instruction.h util.h

util.o: lex.h instruction.h

# Output modules
output/output_bin.o: output/output_bin.h parse.h

output/output_asm.o: output/output_asm.h parse.h util.h

# Intput modules
input/input_bin.o: input/input_bin.h parse.h

.PHONY: clean test
clean:
	- rm -f $(EXECUTABLES) $(ASM_OBJECTS) $(DISASM_OBJECTS) $(ASMCAT_OBJECTS) $(BINCAT_OBJECTS)

test: all
	make -C test test
