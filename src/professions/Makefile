#CC=gcc
#CFLAGS+=-Wall -std=c99 -pg -ggdb
OBJFLD=../obj/
DEPS = Profession.h
OBJ = $(OBJFLD)Profession.o
ODIR =

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: $(OBJ)

.PHONY: all
