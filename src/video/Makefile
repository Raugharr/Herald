OBJFLD := ../$(OBJFLD)
#CFLAGS+=-Wall -std=c99 -pg -ggdb
DEPS = AABB.h Sprite.h Tile.h MapRenderer.h QuadTree.h Gui.h Video.h GuiLua.h Animation.h ImageWidget.h GuiAux.h VideoLua.h MapGenerator.h TextRenderer.h
OBJ = $(OBJFLD)AABB.o $(OBJFLD)Sprite.o $(OBJFLD)Tile.o $(OBJFLD)MapRenderer.o $(OBJFLD)QuadTree.o $(OBJFLD)Gui.o $(OBJFLD)Video.o $(OBJFLD)GuiLua.o $(OBJFLD)Animation.o $(OBJFLD)ImageWidget.o \
	$(OBJFLD)GuiAux.o $(OBJFLD)VideoLua.o $(OBJFLD)MapGenerator.o $(OBJFLD)TextRenderer.o
ODIR=

$(OBJFLD)%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: $(OBJ)

.PHONY: all
