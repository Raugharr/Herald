CC=gcc
CFLAGS=-I
OBJFLD=obj/
DEPS = Building.h Constraint.h Crop.h Family.h Good.h Herald.h Manor.h Person.h Population.h 
OBJ = $(OBJFLD)Building.o $(OBJFLD)Constraint.o $(OBJFLD)Crop.o $(OBJFLD)Family.o $(OBJFLD)Good.o $(OBJFLD)Herald.o $(OBJFLD)Manor.o $(OBJFLD)Person.o $(OBJFLD)Population.o 	
ODIR=.
SYS=sys
EVENTS=events

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 

herald: sys events main.c $(OBJ)
	$(CC) -o ../$@ main.c $(OBJFLD)* 

sys:
	$(MAKE) -C $(SYS)

events:
	$(MAKE) -C $(EVENTS)
.PHONY: clean sys events

clean:
	rm $(OBJFLD)*.o
