OBJFLD := ../$(OBJFLD)
DEPS=ActionRaid.h ActionDuel.h ActionRemovePolicy.h ActionGrowInfluence.h
OBJ=$(OBJFLD)ActionRaid.o $(OBJFLD)ActionDuel.o $(OBJFLD)ActionRemovePolicy.o $(OBJFLD)ActionGrowInfluence.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)

.PHONY: all
