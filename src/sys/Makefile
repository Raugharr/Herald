CC=gcc
CFLAGS+=-Wall
OBJFLD=../obj/
DEPS = Random.h MemoryPool.h HashTable.h LinkedList.h Queue.h Array.h LuaHelper.h RBTree.h Constraint.h Stack.h \
ATimer.h Log.h KDTree.h Event.h Video.h GuiLua.h BinaryHeap.h TaskPool.h Trie.h Gui.h
OBJ = $(OBJFLD)Random.o $(OBJFLD)MemoryPool.o $(OBJFLD)HashTable.o $(OBJFLD)LinkedList.o $(OBJFLD)Queue.o \
$(OBJFLD)Array.o $(OBJFLD)LuaHelper.o $(OBJFLD)RBTree.o $(OBJFLD)Constraint.o $(OBJFLD)Stack.o $(OBJFLD)ATimer.o \
$(OBJFLD)Log.o $(OBJFLD)KDTree.o $(OBJFLD)Event.o $(OBJFLD)Video.o $(OBJFLD)GuiLua.o $(OBJFLD)BinaryHeap.o $(OBJFLD)TaskPool.o $(OBJFLD)Trie.o $(OBJFLD)Gui.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: $(OBJ)

.PHONY: all
