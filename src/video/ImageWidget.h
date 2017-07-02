/*
 * File: ImageWidget.h
 * Author: David Brotz
 */
#ifndef __IMAGEWIDGET_H
#define __IMAGEWIDGET_H

#include "Gui.h"

#include <SDL2/SDL.h>

struct ImageWidget {
	struct Widget Widget;
	struct Sprite* Sprite;
};

struct ImageWidget* CreateImageWidget();
void ConstructImageWidget(struct ImageWidget* Widget, struct Container* Parent, SDL_Rect* Rect, struct Sprite* Sprite);
void DestroyImageWidget(struct Widget* Widget);
int ImageOnDraw(struct Widget* Widget);
struct Widget* ImageOnFocus(struct Widget* Widget, const SDL_Point* Point);
int ImageOnUnfocus(struct Widget* Widget);

#endif /* IMAGEWIDGET_H_ */
