OBJFLD=../../obj/
DEPS = GoalUsurpRuler.h
OBJ = $(OBJFLD)GoalUsurpRuler.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)

.PHONY: all
