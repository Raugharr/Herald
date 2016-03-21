CC=gcc
CFLAGS+= -Wall -std=c99 -pg -ggdb -fprofile-arcs -ftest-coverage
OBJFLD=obj/
SRCOBJFLD=../src/obj/
DEPS=
OBJ=$(SRCOBJFLD)*
LIB=-lm -lcheck -lpthread -lrt -llua5.2 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
NAME=herald-test

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

herald-test: main.c $(OBJ)
	$(CC) -o ../$(NAME) main.c $(CFLAGS) $(LIB) $(OBJ)

all: herald-test

.PHONY: all clean
clean: 
	rm ./$(OBJFLD)*.o


