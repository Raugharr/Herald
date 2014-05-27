CC=gcc
CFLAGS=-I
DEPS = Building.h Constraint.h Crop.h Family.h Good.h HashTable.h Herald.h LinkedList.h Manor.h Person.h Population.h Random.h
OBJ = Building.o Constraint.o Crop.o Family.o Good.o HashTable.o Herald.o LinkedList.o Manor.o Person.o Population.o Random.o	
ODIR=.
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 
herald: main.c $(OBJ)
	$(CC) -o $@ $^ 
.PHONY: clean

clean:
	rm ./*.o
	rm herald
