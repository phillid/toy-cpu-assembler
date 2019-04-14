OBJECTS = lex.o parse.o output.o assembler.o util.o

all: assembler

assembler: $(OBJECTS)

lex.o: lex.h

parse.o: lex.h parse.h instruction.h util.h

output.o: parse.h

util.o: lex.h instruction.h


.PHONY: clean
clean:
	- rm -f assembler $(OBJECTS)
