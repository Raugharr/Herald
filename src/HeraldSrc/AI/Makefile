CC=gcc
CFLAGS+=-Wall
OBJFLD=../obj/
DEPS = AIHelper.h BehaviorTree.h Setup.h LuaLib.h
OBJ = $(OBJFLD)AIHelper.o $(OBJFLD)BehaviorTree.o $(OBJFLD)Setup.o $(OBJFLD)LuaLib.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
all: $(OBJ)

.PHONY: all
