ASM_OBJECTS = lex.o parse.o output.o assembler.o util.o
DISASM_OBJECTS = util.o

all: assembler disassembler

assembler: $(ASM_OBJECTS)

disassembler: $(DISASM_OBJECTS)

lex.o: lex.h

parse.o: lex.h parse.h instruction.h util.h

output.o: parse.h

util.o: lex.h instruction.h

.PHONY: clean test
clean:
	- rm -f assembler disasm $(ASM_OBJECTS)

test:
	make -C test test
