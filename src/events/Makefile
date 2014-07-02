CC=gcc
CFLAGS+=-Wall
OBJFLD=../obj/
DEPS = Event.h EventPerson.h
OBJ = $(OBJFLD)Event.o	$(OBJFLD)EventPerson.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: $(OBJ)

.PHONY: all
