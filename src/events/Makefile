CC=gcc
CFLAGS=-I
OBJFLD=../obj/
DEPS = Event.h EventPerson.h
OBJ = $(OBJFLD)Event.o	$(OBJFLD)EventPerson.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 
all: $(OBJ)

.PHONY: all
