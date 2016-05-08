CC=gcc
#CFLAGS+= -Wall -std=c99 -pg -ggdb
OBJFLD=obj/
DEPS=Herald.h Building.h Crop.h Family.h Good.h Person.h Population.h World.h Actor.h Location.h Mission.h BigGuy.h Warband.h Government.h LuaFamily.h LuaSettlement.h LuaWorld.h ArmyGoal.h Battle.h \
WorldState.h Date.h Feud.h Profession.h Trait.h Bulitin.h BigGuyRelation.h Plot.h
OBJ=$(OBJFLD)Herald.o $(OBJFLD)Building.o $(OBJFLD)Crop.o $(OBJFLD)Family.o \
$(OBJFLD)Good.o $(OBJFLD)Person.o $(OBJFLD)Population.o $(OBJFLD)World.o \
$(OBJFLD)Actor.o $(OBJFLD)Location.o $(OBJFLD)Mission.o $(OBJFLD)BigGuy.o $(OBJFLD)Warband.o $(OBJFLD)Government.o \
$(OBJFLD)LuaFamily.o $(OBJFLD)LuaSettlement.o $(OBJFLD)LuaWorld.o $(OBJFLD)ArmyGoal.o $(OBJFLD)Battle.o $(OBJFLD)WorldState.o \
$(OBJFLD)Date.o $(OBJFLD)Feud.o $(OBJFLD)Profession.o $(OBJFLD)Trait.o $(OBJFLD)Bulitin.o $(OBJFLD)BigGuyRelation.o $(OBJFLD)Plot.o
ODIR=.
DEFS=
export DEFS
ifdef windows
	LIB=-lmingw32 -llua -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
	DEFS+=-DWINDOWS
else
	LIB=-lm -llua5.2 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image
endif
VIDEO=video
SYS=sys
AI=AI
NAME=herald

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
herald: sys ai video main.c $(OBJ)
	$(CC) -LC:\MinGW\lib -o ../$(NAME) main.c $(CFLAGS) $(OBJFLD)* $(LIB) $(DEFS)
	
all: herald

sys:
	$(MAKE) -C $(SYS)
	
video:
	$(MAKE) -C $(VIDEO)

ai:
	$(MAKE) -C $(AI)
	
.PHONY: clean all sys ai video

clean:
	rm ./$(OBJFLD)*.o
