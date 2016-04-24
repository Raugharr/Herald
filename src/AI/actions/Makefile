#CC=gcc
#CFLAGS+=-Wall -std=c99 -pg -ggdb
OBJFLD=../../obj/
DEPS = ActionImproveRelations.h ActionRaid.h ActionDuel.h ActionMurder.h
OBJ = $(OBJFLD)ActionImproveRelations.o $(OBJFLD)ActionRaid.o $(OBJFLD)ActionDuel.o $(OBJFLD)ActionMurder.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)

.PHONY: all
