CC=gcc
CFLAGS= -Wall
OBJFLD=obj/
DEPS=Building.h Crop.h Family.h Good.h Herald.h Manor.h Person.h Population.h Occupation.h World.h LuaWrappers.h
OBJ=$(OBJFLD)Building.o $(OBJFLD)Crop.o $(OBJFLD)Family.o \
$(OBJFLD)Good.o $(OBJFLD)Herald.o $(OBJFLD)Manor.o $(OBJFLD)Person.o $(OBJFLD)Population.o $(OBJFLD)Occupation.o $(OBJFLD)World.o $(OBJFLD)LuaWrappers.o
ODIR=.
LIB=lua
SYS=sys
EVENTS=events
NAME=herald

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
herald: sys events main.c $(OBJ)
	$(CC) -o ../$(NAME) main.c $(OBJFLD)* -l$(LIB)
	
all: herald

sys:
	$(MAKE) -C $(SYS)

events:
	$(MAKE) -C $(EVENTS)
.PHONY: clean all sys events

clean:
	rm $(OBJFLD)*.o
