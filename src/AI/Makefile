OBJFLD := ../$(OBJFLD)
DEPS = BehaviorTree.h Setup.h LuaLib.h Pathfind.h goap.h Agent.h LuaAI.h Behaviors.h Blackboard.h GoapGoal.h GoapAction.h
OBJ = $(OBJFLD)BehaviorTree.o $(OBJFLD)Setup.o $(OBJFLD)LuaLib.o $(OBJFLD)Pathfind.o \
	$(OBJFLD)goap.o $(OBJFLD)Agent.o $(OBJFLD)LuaAI.o $(OBJFLD)Behaviors.o $(OBJFLD)Blackboard.o $(OBJFLD)GoapGoal.o \
	$(OBJFLD)GoapAction.o
ODIR=
ACTIONS=actions
GOALS=goals

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
all: $(OBJ)
	$(MAKE) -C $(ACTIONS)
	$(MAKE) -C $(GOALS)

.PHONY: all
