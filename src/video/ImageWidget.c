/*
 * File: ImageWidget.c
 * Author: David Brotz
 */

#include "ImageWidget.h"

#include "AABB.h"
#include "Sprite.h"

#include <stdlib.h>

struct ImageWidget* CreateImageWidget() {
	return (struct ImageWidget*) malloc(sizeof(struct ImageWidget));
}

void ConstructImageWidget(struct ImageWidget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, struct Sprite* _Sprite) {
	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->Widget.OnDraw = ImageOnDraw;
	_Widget->Widget.OnFocus = ImageOnFocus;
	_Widget->Widget.OnUnfocus = ImageOnUnfocus;
	_Widget->Widget.OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyImageWidget;
	_Widget->Widget.OnDraw = ImageOnDraw;
	_Widget->Sprite = _Sprite;
}

void DestroyImageWidget(struct ImageWidget* _Widget, lua_State* _State) {
	DestroySprite(_Widget->Sprite);
	DestroyWidget((struct Widget*)_Widget, _State);
}

int ImageOnDraw(struct Widget* _Widget) {
	if(_Widget->IsVisible == 0)
		return 1;
	if(SpriteOnDraw(((struct ImageWidget*)_Widget)->Sprite) != 0)
		return 0;
	return 1;
}

struct Widget* ImageOnFocus(struct Widget* _Widget, const SDL_Point* _Point) {
	if(PointInAABB(_Point, &_Widget->Rect) == SDL_FALSE || _Widget->CanFocus == 0) {
		return NULL;
	}
	return _Widget;
}

int ImageOnUnfocus(struct Widget* _Widget) {
	return 1;
}
