CC=gcc
CFLAGS=-I
OBJFLD=./obj
DEPS = Building.h Constraint.h Crop.h Family.h Good.h HashTable.h Herald.h LinkedList.h Manor.h Person.h Population.h Random.h
OBJ = $(OBJFLD)/Building.o $(OBJFLD)/Constraint.o $(OBJFLD)/Crop.o $(OBJFLD)/Family.o $(OBJFLD)/Good.o $(OBJFLD)/HashTable.o $(OBJFLD)/Herald.o $(OBJFLD)/LinkedList.o $(OBJFLD)/Manor.o $(OBJFLD)/Person.o $(OBJFLD)/Population.o $(OBJFLD)/Random.o	
ODIR=.
%.o: %.c $(DEPS)
	$(CC) -c -o $(OBJFLD)/$@ $< 
herald: main.c $(OBJ)
	$(CC) -o ../$@ $^ 
.PHONY: clean

clean:
	rm ./*.o
	rm herald*
