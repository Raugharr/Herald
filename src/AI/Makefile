CC=gcc
CFLAGS+=-Wall -std=c99 -pg -ggdb
OBJFLD=../obj/
DEPS = BehaviorTree.h Setup.h LuaLib.h Pathfind.h goap.h Agent.h LuaAI.h Behaviors.h Blackboard.h
OBJ = $(OBJFLD)BehaviorTree.o $(OBJFLD)Setup.o $(OBJFLD)LuaLib.o $(OBJFLD)Pathfind.o \
	$(OBJFLD)goap.o $(OBJFLD)Agent.o $(OBJFLD)LuaAI.o $(OBJFLD)Behaviors.o $(OBJFLD)Blackboard.o
ODIR=
ACTIONS=actions

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
all: $(OBJ)
	$(MAKE) -C $(ACTIONS)

.PHONY: all
