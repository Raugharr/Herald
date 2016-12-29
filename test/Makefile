CC=gcc
CFLAGS+=-Wall -std=c99 -pg -ggdb -fprofile-arcs -ftest-coverage
OBJFLD=obj/
SRCOBJFLD=../src/obj/
DEPS=GoapTest.h DataStruct.h RedBlackTest.h IntervalTreeTest.h MathTest.h CoroTest.h GameHarness.h
OBJ=$(SRCOBJFLD)* $(OBJFLD)GoapTest.o $(OBJFLD)DataStruct.o $(OBJFLD)RedBlackTest.o $(OBJFLD)IntervalTreeTest.o $(OBJFLD)MathTest.o $(OBJFLD)CoroTest.o\
$(OBJFLD)GameHarness.o
LIB=-lm -lcheck -lpthread -lrt -llua5.2 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
NAME=herald-test

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

herald-test: main.c $(OBJ)
	$(CC) -o ../$(NAME) main.c $(CFLAGS) $(OBJ) $(LIB)
all: herald-test

.PHONY: all clean
clean: 
	rm ./$(OBJFLD)*.o


