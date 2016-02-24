CC=gcc
CFLAGS+=-Wall -std=c99 -pg -ggdb
OBJFLD=../obj/
DEPS = AIHelper.h BehaviorTree.h Setup.h LuaLib.h Pathfind.h goap.h Utility.h Agent.h LuaAI.h
OBJ = $(OBJFLD)AIHelper.o $(OBJFLD)BehaviorTree.o $(OBJFLD)Setup.o $(OBJFLD)LuaLib.o $(OBJFLD)Pathfind.o $(OBJFLD)goap.o $(OBJFLD)Utility.o $(OBJFLD)Agent.o $(OBJFLD)LuaAI.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
all: $(OBJ)

.PHONY: all
