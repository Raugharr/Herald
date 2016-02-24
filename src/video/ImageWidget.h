/*
 * File: ImageWidget.h
 * Author: David Brotz
 */
#ifndef __IMAGEWIDGET_H
#define __IMAGEWIDGET_H

#include "Gui.h"

#include <SDL2/SDL.h>

struct ImageWidget {
	DECLARE_WIDGET;
	struct Sprite* Sprite;
};

struct ImageWidget* CreateImageWidget();
void ConstructImageWidget(struct ImageWidget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, struct Sprite* _Sprite);
void DestroyImageWidget(struct ImageWidget* _Widget, lua_State* _State);
int ImageOnDraw(struct Widget* _Widget);
struct Widget* ImageOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int ImageOnUnfocus(struct Widget* _Widget);

#endif /* IMAGEWIDGET_H_ */
