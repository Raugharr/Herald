CC=gcc
CFLAGS+= -Wall
OBJFLD=obj/
DEPS=Building.h Crop.h Family.h Good.h Herald.h Person.h Population.h Occupation.h World.h
OBJ=$(OBJFLD)Building.o $(OBJFLD)Crop.o $(OBJFLD)Family.o \
$(OBJFLD)Good.o $(OBJFLD)Herald.o $(OBJFLD)Person.o $(OBJFLD)Population.o $(OBJFLD)Occupation.o $(OBJFLD)World.o
ODIR=.
ifdef Windows
	LIB=-lmingw32 -llua5.2 -lSDL2main -lSDL2 -lSDL2_ttf
else
	LIB=-llua5.2 -lSDL2main -lSDL2 -lSDL2_ttf
endif
SYS=sys
AI=AI
NAME=herald

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
herald: sys ai video main.c $(OBJ)
	$(CC) -o ../$(NAME) main.c $(CFLAGS) $(OBJFLD)* $(LIB)
	
all: herald

sys:
	$(MAKE) -C $(SYS)

ai:
	$(MAKE) -C $(AI)
	
.PHONY: clean all sys ai video

clean:
	rm $(OBJFLD)*.o
