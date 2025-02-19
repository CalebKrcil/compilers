CC = gcc
LEX = flex
YACC = bison

TARGET = k0
LEX_SRC = k0lex.l
YACC_SRC = k0gram.y
MAIN_SRC = main.c

LEX_OUT = k0lex.c
YACC_OUT = k0gram.tab.c
YACC_HEADER = k0gram.tab.h

OBJS = $(YACC_OUT:.c=.o) $(LEX_OUT:.c=.o) $(MAIN_SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -lfl -lm

$(LEX_OUT): $(LEX_SRC) $(YACC_HEADER)
	$(LEX) -o $(LEX_OUT) $(LEX_SRC)

$(YACC_OUT) $(YACC_HEADER): $(YACC_SRC)
	$(YACC) -d $(YACC_SRC)

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LEX_OUT) $(YACC_OUT) $(YACC_HEADER) $(TARGET)
