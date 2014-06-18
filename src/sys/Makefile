CC=gcc
CFLAGS=-Wall
OBJFLD=../obj/
DEPS = Random.h MemoryPool.h HashTable.h LinkedList.h Queue.h Array.h LuaHelper.h RBTree.h Constraint.h Stack.h
OBJ = $(OBJFLD)Random.o $(OBJFLD)MemoryPool.o $(OBJFLD)HashTable.o $(OBJFLD)LinkedList.o $(OBJFLD)Queue.o \
$(OBJFLD)Array.o $(OBJFLD)LuaHelper.o $(OBJFLD)RBTree.o $(OBJFLD)Constraint.o $(OBJFLD)Stack.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: $(OBJ)

.PHONY: all
