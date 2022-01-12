.PHONY: clean

DEFS   = -DDEBUG
CDEBUG = -g
CFLAGS = -std=c11 -I. -Wall -Wextra -Wpedantic $(CDEBUG) $(DEFS)
LDLIBS = -lreadline
OBJS   = shell.o lexer.o debug.o parser.o eval.o job.o

all: $(OBJS)
	$(CC) -o shell243 $(OBJS) $(LDLIBS) $(CFLAGS)

shell.o: parser.h eval.h debug.h
lexer.o: lexer.h
debug.o: debug.h lexer.h parser.h
parser.o: parser.h lexer.h debug.h
eval.o: eval.h parser.h job.h debug.h
job.o: job.h

clean:
	rm shell243 *.o
