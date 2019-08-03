CFLAGS = -Wall -Wextra

EXECUTABLES = assembler disassembler

ASM_OBJECTS = lex.o parse.o output.o assembler.o util.o
DISASM_OBJECTS = disassembler.o util.o input_bin.o output_asm.o

all: $(EXECUTABLES)

assembler: $(ASM_OBJECTS)

disassembler: $(DISASM_OBJECTS)

lex.o: lex.h

parse.o: lex.h parse.h instruction.h util.h

output.o: parse.h

util.o: lex.h instruction.h

.PHONY: clean test
clean:
	- rm -f $(EXECUTABLES) $(ASM_OBJECTS) $(DISASM_OBJECTS)

test: all
	make -C test test
