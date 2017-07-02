/*
 * File: ImageWidget.c
 * Author: David Brotz
 */

#include "ImageWidget.h"

#include "AABB.h"
#include "Sprite.h"

#include "../sys/LuaCore.h"

#include <stdlib.h>

struct ImageWidget* CreateImageWidget() {
	struct ImageWidget* Widget = (struct ImageWidget*) malloc(sizeof(struct ImageWidget));

	memset(Widget, 0, sizeof(*Widget));
	return Widget;
}

void ConstructImageWidget(struct ImageWidget* Widget, struct Container* Parent, SDL_Rect* Rect, struct Sprite* Sprite) {

	Widget->Widget.Style = Parent->Skin->Default;
	ConstructWidget(&Widget->Widget, Parent, Rect, LOBJ_IMAGEWIDGET);
	Widget->Widget.OnDraw = ImageOnDraw;
	Widget->Widget.OnFocus = ImageOnFocus;
	Widget->Widget.OnUnfocus = ImageOnUnfocus;
	Widget->Widget.OnDestroy = DestroyImageWidget;
	Widget->Widget.OnDraw = ImageOnDraw;
	Widget->Sprite = Sprite;
}

void DestroyImageWidget(struct Widget* Widget) {
	struct ImageWidget* ImageWidget = (struct ImageWidget*)Widget;
	DestroySprite(ImageWidget->Sprite);
	Widget->OnDestroy(Widget);
}

int ImageOnDraw(struct Widget* Widget) {
	if(Widget->IsVisible == 0)
		return 1;
	SpriteOnDraw(g_Renderer, ((struct ImageWidget*)Widget)->Sprite, 0, 0);
	return 1;
}

struct Widget* ImageOnFocus(struct Widget* Widget, const SDL_Point* Point) {
	if(PointInAABB(Point, &Widget->Rect) == SDL_FALSE || Widget->CanFocus == 0) {
		return NULL;
	}
	return Widget;
}

int ImageOnUnfocus(struct Widget* Widget) {
	return 1;
}
