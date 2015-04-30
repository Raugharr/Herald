CC=gcc
CFLAGS+= -Wall
OBJFLD=obj/
DEPS=Building.h Crop.h Family.h Good.h Herald.h Person.h Population.h Occupation.h World.h Actor.h Zone.h Location.h Mission.h BigGuy.h Warband.h Government.h
OBJ=$(OBJFLD)Building.o $(OBJFLD)Crop.o $(OBJFLD)Family.o \
$(OBJFLD)Good.o $(OBJFLD)Herald.o $(OBJFLD)Person.o $(OBJFLD)Population.o $(OBJFLD)Occupation.o $(OBJFLD)World.o \
$(OBJFLD)Actor.o $(OBJFLD)Zone.o $(OBJFLD)Location.o $(OBJFLD)Mission.o $(OBJFLD)BigGuy.o $(OBJFLD)Warband.o $(OBJFLD)Government.o
ODIR=.
ifdef windows
	LIB=-lmingw32 -llua -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
else
	LIB=-llua5.2 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
endif
VIDEO=video
SYS=sys
AI=AI
NAME=herald

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
herald: sys ai video main.c $(OBJ)
	$(CC) -LC:\MinGW\lib -o ../$(NAME) main.c $(CFLAGS) $(OBJFLD)* $(LIB)
	
all: herald

sys:
	$(MAKE) -C $(SYS)
	
video:
	$(MAKE) -C $(VIDEO)

ai:
	$(MAKE) -C $(AI)
	
.PHONY: clean all sys ai video

clean:
	rm $(OBJFLD)*.o
