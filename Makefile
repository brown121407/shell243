.PHONY: clean

DEFS   = -DDEBUG
CDEBUG = -g
CFLAGS = -std=c11 -I. -Wall -Wextra -Wpedantic $(CDEBUG) $(DEFS)
LDLIBS = -lreadline
OBJS   = shell.o lexer.o debug.o parser.o eval.o

all: $(OBJS)
	$(CC) -o shell243 $(OBJS) $(LDLIBS) $(CFLAGS)

shell.o: lexer.h debug.h
lexer.o: lexer.h
debug.o: debug.h lexer.h parser.h
parser.o: debug.h parser.h lexer.h
eval.o: eval.h parser.h

clean:
	rm shell243 *.o
