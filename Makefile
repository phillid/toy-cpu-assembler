CFLAGS = -Wall -Wextra -Wpedantic

EXECUTABLES = assembler disassembler asmcat bincat

ASM_OBJECTS = assembler.o lex.o parse.o output.o util.o
DISASM_OBJECTS = disassembler.o input_bin.o output_asm.o util.o
ASMCAT_OBJECTS = asmcat.o lex.o parse.o output_asm.o util.o
BINCAT_OBJECTS = bincat.o input_bin.o output.o util.o

all: $(EXECUTABLES)

assembler: $(ASM_OBJECTS)

disassembler: $(DISASM_OBJECTS)

asmcat: $(ASMCAT_OBJECTS)

bincat: $(BINCAT_OBJECTS)

lex.o: lex.h

parse.o: lex.h parse.h instruction.h util.h

output.o: parse.h

util.o: lex.h instruction.h

.PHONY: clean test
clean:
	- rm -f $(EXECUTABLES) $(ASM_OBJECTS) $(DISASM_OBJECTS) $(ASMCAT_OBJECTS) $(BINCAT_OBJECTS)

test: all
	make -C test test
