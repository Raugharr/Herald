CC=gcc
CFLAGS+= -Wall
OBJFLD=obj/
DEPS=Building.h Crop.h Family.h Good.h Herald.h Manor.h Person.h Population.h Occupation.h World.h Log.h
OBJ=$(OBJFLD)Building.o $(OBJFLD)Crop.o $(OBJFLD)Family.o $(OBJFLD)Log.o \
$(OBJFLD)Good.o $(OBJFLD)Herald.o $(OBJFLD)Manor.o $(OBJFLD)Person.o $(OBJFLD)Population.o $(OBJFLD)Occupation.o $(OBJFLD)World.o
ODIR=.
LIB=-llua -lsdl2main -lsdl2
SYS=sys
EVENTS=events
AI=ai
NAME=herald

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
herald: sys events ai main.c $(OBJ)
	$(CC) -o ../$(NAME) main.c $(CFLAGS) $(OBJFLD)* $(LIB)
	
all: herald

sys:
	$(MAKE) -C $(SYS)

events:
	$(MAKE) -C $(EVENTS)

ai:
	$(MAKE) -C $(AI)
.PHONY: clean all sys events ai

clean:
	rm $(OBJFLD)*.o
