OBJECTS = lex.o parse.o output.o assembler.o tok_util.o

all: assembler

assembler: $(OBJECTS)

lex.o: lex.h

parse.o: lex.h parse.h instruction.h tok_util.h

output.o: parse.h

tok_util.o: lex.h


.PHONY: clean
clean:
	- rm -f assembler $(OBJECTS)
