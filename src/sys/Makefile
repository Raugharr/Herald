CC=gcc
CFLAGS=-I
OBJFLD=../obj/
DEPS = Random.h MemoryPool.h HashTable.h LinkedList.h
OBJ = $(OBJFLD)Random.o $(OBJFLD)MemoryPool.o $(OBJFLD)HashTable.o $(OBJFLD)LinkedList.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 
all: $(OBJ)

.PHONY: all
