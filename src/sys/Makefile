OBJFLD := ../$(OBJFLD)
DEPS = Math.h MemoryPool.h HashTable.h LinkedList.h Queue.h Array.h RBTree.h Constraint.h Stack.h \
Log.h Event.h BinaryHeap.h TaskPool.h Trie.h Rule.h LuaCore.h StackAllocator.h ResourceManager.h  \
GenIterator.h Coroutine.h FrameAllocator.h Memory.h IntervalTree.h ITree.h
OBJ = $(OBJFLD)Math.o $(OBJFLD)MemoryPool.o $(OBJFLD)HashTable.o $(OBJFLD)LinkedList.o $(OBJFLD)Queue.o \
$(OBJFLD)Array.o $(OBJFLD)RBTree.o $(OBJFLD)Constraint.o $(OBJFLD)Stack.o  $(OBJFLD)GenIterator.o \
$(OBJFLD)Log.o $(OBJFLD)Event.o $(OBJFLD)BinaryHeap.o $(OBJFLD)TaskPool.o \
$(OBJFLD)Trie.o $(OBJFLD)Rule.o $(OBJFLD)LuaCore.o $(OBJFLD)StackAllocator.o $(OBJFLD)ResourceManager.o \
$(OBJFLD)Coroutine.o $(OBJFLD)FrameAllocator.o $(OBJFLD)Memory.o $(OBJFLD)IntervalTree.o $(OBJFLD)ITree.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFS)
all: $(OBJ)

.PHONY: all
