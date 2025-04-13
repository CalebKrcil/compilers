CC = gcc
LEX = flex
YACC = bison
CFLAGS = -Wall

TARGET = k0
LEX_SRC = k0lex.l
YACC_SRC = k0gram.y
TREE_SRC = tree.c
MAIN_SRC = main.c
SYMTAB_SRC = symtab.c
TYPE_SRC = type.c
SEMANTICS_SRC = semantics.c

LEX_OUT = k0lex.c
YACC_OUT = k0gram.tab.c
YACC_HEADER = k0gram.tab.h

OBJS = k0gram.tab.o k0lex.o tree.o main.o symtab.o type.o semantics.o

#--- New definitions for Lab 9 ---
LAB9_TARGET = lab9
LAB9_SRC = lab9.c tac.c
LAB9_OBJS = lab9.o tac.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lfl -lm

# Build k0 lex and yacc files.
$(LEX_OUT): $(LEX_SRC) $(YACC_HEADER)
	$(LEX) -o $(LEX_OUT) $(LEX_SRC)

$(YACC_OUT) $(YACC_HEADER): $(YACC_SRC)
	$(YACC) -d $(YACC_SRC)

k0gram.tab.o: $(YACC_OUT)
	$(CC) $(CFLAGS) -c $(YACC_OUT)

k0lex.o: $(LEX_OUT)
	$(CC) $(CFLAGS) -c $(LEX_OUT)

tree.o: $(TREE_SRC)
	$(CC) $(CFLAGS) -c $(TREE_SRC)

main.o: $(MAIN_SRC)
	$(CC) $(CFLAGS) -c $(MAIN_SRC)

symtab.o: $(SYMTAB_SRC)
	$(CC) $(CFLAGS) -c $(SYMTAB_SRC)

type.o: $(TYPE_SRC)
	$(CC) $(CFLAGS) -c $(TYPE_SRC)

semantics.o: $(SEMANTICS_SRC)
	$(CC) $(CFLAGS) -c $(SEMANTICS_SRC)

#--- New target for Lab 9 ---
$(LAB9_TARGET): $(LAB9_OBJS)
	$(CC) $(CFLAGS) -o $(LAB9_TARGET) $(LAB9_OBJS)

lab9.o: lab9.c tac.h
	$(CC) $(CFLAGS) -c lab9.c

tac.o: tac.c tac.h
	$(CC) $(CFLAGS) -c tac.c

clean:
	rm -f $(OBJS) $(LEX_OUT) $(YACC_OUT) $(YACC_HEADER) $(TARGET) $(LAB9_OBJS) $(LAB9_TARGET)
