CC=gcc
CFLAGS+=-Wall
OBJFLD=../obj/
DEPS = BehaviorTree.h Setup.h LuaLib.h
OBJ = $(OBJFLD)BehaviorTree.o $(OBJFLD)Setup.o $(OBJFLD)LuaLib.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
all: $(OBJ)

.PHONY: all
