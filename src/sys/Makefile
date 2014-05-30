CC=gcc
CFLAGS=-I
OBJFLD=../obj/
DEPS = Random.h MemoryPool.h HashTable.h LinkedList.h Queue.h
OBJ = $(OBJFLD)Random.o $(OBJFLD)MemoryPool.o $(OBJFLD)HashTable.o $(OBJFLD)LinkedList.o $(OBJFLD)Queue.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 
all: $(OBJ)

.PHONY: all
