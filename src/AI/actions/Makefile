#CC=gcc
#CFLAGS+=-Wall -std=c99 -pg -ggdb
OBJFLD=../../obj/
DEPS=ActionRaid.h ActionDuel.h ActionRemovePolicy.h ActionGrowInfluence.h
OBJ=$(OBJFLD)ActionRaid.o $(OBJFLD)ActionDuel.o $(OBJFLD)ActionRemovePolicy.o $(OBJFLD)ActionGrowInfluence.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)

.PHONY: all
