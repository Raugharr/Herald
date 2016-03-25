/*
 * File: Gui.c
 * Author: David Brotz
 */

#include "Gui.h"

#include "Video.h"
#include "AABB.h"
#include "GuiLua.h"

#include "../sys/Stack.h"
#include "../sys/Event.h"

#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>

struct GuiStack {
	struct Container* Top;
	struct Container* Bot;
};

struct GUIDef g_GUIDefs = {NULL, {255, 255, 255, 255}, {128, 128, 128, 255}};
int g_GUIMenuChange = 0;
int g_GUIId = 0;
struct GUIFocus* g_Focus = NULL;
struct GUIEvents* g_GUIEvents = NULL;
struct Font* g_GUIFonts = NULL;
struct Stack g_GUIStack = {NULL, 0};
struct GuiStack g_GuiZBuff = {NULL, NULL};

int NextGUIId(void) {return g_GUIId++;}

struct Container* CreateContainer(void) {
	return (struct Container*) malloc(sizeof(struct Container));
}

struct GUIEvents* CreateGUIEvents(void) {
	struct GUIEvents* _New = (struct GUIEvents*) malloc(sizeof(struct GUIEvents));
	_New->TblSz = 16;
	_New->Size = 0;
	_New->Events = calloc(_New->TblSz, sizeof(struct WEvent));
	return _New;
}

struct GUIFocus* CreateGUIFocus(void) {
	struct GUIFocus* _New = (struct GUIFocus*) malloc(sizeof(struct GUIFocus));
	_New->Id = 0;
	_New->Index = 0;
	_New->Parent = NULL;
	_New->Prev = NULL;
	return _New;
}

struct Container* GUIZTop(void) {
	return g_GuiZBuff.Top;
}

struct Container* GUIZBot(void) {
	return g_GuiZBuff.Bot;
}

void GuiClear(lua_State* _State) {
	struct Container* _Container = g_GuiZBuff.Top;
	const char* _String = NULL;

	while(g_GuiZBuff.Top != NULL) {
		ILL_DESTROY(g_GuiZBuff.Top, _Container);
		_Container->OnDestroy((struct Widget*) _Container, _State);
		_Container = g_GuiZBuff.Top;	
	}
	/*free(StackPop(&g_GUIStack));
	if((_String = (char*)StackTop(&g_GUIStack)) == NULL) {
		g_VideoOk = 0;
		return;
	}
	//GuiSetMenu(_String, _State);
	*/
	g_GuiZBuff.Bot = NULL;
}

void GuiEmpty() {
	g_GuiZBuff.Top = NULL;
	g_GuiZBuff.Bot = NULL;
}

void ConstructWidget(struct Widget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State) {
	_Widget->Id = NextGUIId();
	_Widget->IsDraggable = 0;
	_Widget->LuaRef = LuaWidgetRef(_State);
	_Widget->LuaOnClickFunc = -2;
	_Widget->CanFocus = 1;
	_Widget->IsVisible = 1;
	_Widget->OnClick = WidgetOnClick;
	_Widget->SetPosition = WidgetSetPosition;
	_Widget->OnDraw = NULL;
	_Widget->OnFocus = WidgetOnFocus;
	_Widget->OnUnfocus = WidgetOnUnfocus;
	_Widget->OnKeyUp = WidgetOnKeyUp;
	_Widget->OnDestroy = NULL;
	_Widget->OnDebug = WidgetOnDebug;
	_Widget->Rect.x = _Rect->x;
	_Widget->Rect.y = _Rect->y;
	_Widget->Rect.w = _Rect->w;
	_Widget->Rect.h = _Rect->h;
	_Widget->OnDrag = WidgetOnDrag;
	if(_Parent != NULL) {
		WidgetSetParent(_Parent, _Widget);
		if(WidgetCheckVisibility(_Widget) == 0) {
			WidgetGrow((struct Widget*)_Widget->Parent, _Widget->Rect.w, _Widget->Rect.h);
			_Widget->IsVisible = WidgetCheckVisibility(_Widget);
		}
	} else if(GuiGetParentHook() != NULL) {
		WidgetSetParent(GuiGetParentHook(), _Widget);
	} else {
		_Widget->Parent = NULL;
	}
}

void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State, int _Spacing, const struct Margin* _Margin) {
	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	ILL_CREATE(g_GuiZBuff.Top, _Widget);
	if(g_GuiZBuff.Bot == NULL)
		g_GuiZBuff.Bot = _Widget;
	_Widget->Children = NULL;
	_Widget->ChildrenSz = 0;
	_Widget->ChildCt = 0;
	_Widget->OnDraw = (int(*)(struct Widget*))ContainerOnDraw;
	_Widget->SetPosition = (void(*)(struct Widget*, const SDL_Point*))ContainerSetPosition;
	_Widget->OnFocus = (struct Widget*(*)(struct Widget*, const SDL_Point*))ContainerOnFocus;
	_Widget->OnUnfocus = (int(*)(struct Widget*))ContainerOnUnfocus;
	_Widget->OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyContainer;
	_Widget->OnDraw = (int(*)(struct Widget*))ContainerOnDraw;
	_Widget->OnClick = (struct Widget*(*)(struct Widget*, const SDL_Point*))ContainerOnClick;
	_Widget->NewChild = NULL;
	_Widget->RemChild = DynamicRemChild;
	_Widget->Spacing = _Spacing;
	_Widget->Margins.Top = _Margin->Top;
	_Widget->Margins.Left = _Margin->Left;
	_Widget->Margins.Right = _Margin->Right;
	_Widget->Margins.Bottom = _Margin->Bottom;
	_Widget->VertFocChange = 1;
	_Widget->HorzFocChange = ContainerHorzFocChange;
	_Widget->OnDebug = (void(*)(const struct Widget*))ContainerOnDebug;
	_Widget->OnDrag = (struct Widget*(*)(struct Widget*, const SDL_Point*))ContainerOnDrag;
	_Widget->Background.r = 0;
	_Widget->Background.g = 0;
	_Widget->Background.b = 0;
	_Widget->Background.a = 0xFF;
}

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child, SDL_Point* _Pos) {
	int _X = _Parent->Margins.Left + _Parent->Rect.x;
	int _Y = _Parent->Margins.Top + _Parent->Rect.y;

	for(int i = 0; i < _Parent->ChildCt - 1 && _Parent->Children[i] != NULL; ++i) {
		_X += _Parent->Spacing + _Parent->Children[i]->Rect.w + _Parent->Spacing;
		_Y += _Parent->Spacing + _Parent->Children[i]->Rect.h + _Parent->Spacing;
	}
	_Pos->x = _X;
	_Pos->y = _Y;
}

struct Widget* ContainerOnDrag(struct Container* _Widget, const struct SDL_Point* _Pos) {
	struct Widget* _Child = NULL;

	if(_Widget->IsDraggable == 0) {
		for(int i = 0; i < _Widget->ChildCt; ++i) {
			_Child = _Widget->Children[i];
			if(_Child->OnDrag(_Child, _Pos) != NULL)
				return _Child;
		}
		return NULL;
	}
	return (PointInAABB(_Pos, &_Widget->Rect) != 0) ? ((struct Widget*)_Widget) : (NULL);
}

/*
 * FIXME: Check if _Child already has a parent and if it does delete it from the old parent.
 */
void WidgetSetParent(struct Container* _Parent, struct Widget* _Child) {
	int i = 0;

	if(_Parent->Children == NULL) {
		_Parent->Children = calloc(2, sizeof(struct Widget*));
		memset(_Parent->Children, 0, sizeof(struct Widget*) * 2);
		_Parent->ChildrenSz = 2;
	} else if(_Parent->ChildCt == _Parent->ChildrenSz) {
		_Parent->Children = realloc(_Parent->Children, sizeof(struct Widget*) * _Parent->ChildrenSz * 2);
		for(i = _Parent->ChildrenSz; i < _Parent->ChildrenSz * 2; ++i)
			_Parent->Children[i] = NULL;
		_Parent->ChildrenSz *= 2;
	}
	_Parent->Children[_Parent->ChildCt++] = _Child;
	_Child->Parent = _Parent;
	_Parent->NewChild(_Parent, _Child);
	if(_Child->Rect.x + _Child->Rect.w > _Parent->Rect.x + _Parent->Rect.w) {
		_Child->Rect.w = _Parent->Rect.w - _Child->Rect.x;
		if(_Child->Rect.w < 0)
			_Child->Rect.w = 0;
	}
	if(_Child->Rect.y +_Child->Rect.h > _Parent->Rect.y +_Parent->Rect.h) {
		_Child->Rect.h = _Parent->Rect.h - _Child->Rect.y;
		if(_Child->Rect.h < 0)
			_Child->Rect.h = 0;
	}
}

void WidgetOnKeyUp(struct Widget* _Widget, SDL_KeyboardEvent* _Event) {

}

void WidgetSetVisibility(struct Widget* _Widget, int _Visibility) {
	_Widget->IsVisible = (WidgetCheckVisibility(_Widget) && (_Visibility != 0));
}

struct Widget* WidgetOnDrag(struct Widget* _Widget, const struct SDL_Point* _Pos) {
	if(PointInAABB(_Pos, &_Widget->Rect) != 0)
		return NULL;
	return (_Widget->IsDraggable == 0) ? (NULL) : (_Widget);
}

void DestroyWidget(struct Widget* _Widget, lua_State* _State) {
	struct Container* _Parent = _Widget->Parent;

	if(_Parent != NULL)
		_Parent->RemChild(_Parent, _Widget);
	if(GetFocusableWidget() == _Widget)
		FocusableWidgetNull();
	LuaWidgetUnref(_State, _Widget);
	LuaWidgetOnKeyUnref(_State, _Widget);
	free(_Widget);
}

void DestroyContainer(struct Container* _Container, lua_State* _State) {
	for(int i = 0; i < _Container->ChildCt; ++i) {
		if(_Container->Children[i] == NULL)
			continue;
		_Container->Children[i]->OnDestroy(_Container->Children[i], _State);
	}
	ILL_DESTROY(g_GuiZBuff.Top, _Container);
	if(g_GuiZBuff.Bot == _Container)
		g_GuiZBuff.Bot = NULL;
	DestroyWidget((struct Widget*)_Container, _State);
}

int ContainerOnDraw(struct Container* _Container) {
	struct Widget* _Widget = NULL;

	SDL_SetRenderDrawColor(g_Renderer, _Container->Background.r, _Container->Background.g, _Container->Background.b, _Container->Background.a);
	SDL_RenderFillRect(g_Renderer, &_Container->Rect);
	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Widget = _Container->Children[i];
		/*if(_Widget->Rect.x >= _Container->Rect.x && _Widget->Rect.y >= _Container->Rect.y
				&& _Widget->Rect.x + _Widget->Rect.w <= _Container->Rect.x + _Container->Rect.w
				&& _Widget->Rect.y + _Widget->Rect.h <= _Container->Rect.y + _Container->Rect.h) {*/

		if(_Widget->OnDraw(_Widget) == 0)
			return 0;
		//}
	}
	return 1;
}

int MenuOnDraw(struct Container* _Container) {
	struct Widget* _Widget = NULL;

	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Widget = _Container->Children[i];
		if(_Widget->OnDraw(_Widget) == 0)
			return 0;
	}
}

void ContainerSetPosition(struct Container* _Container, const struct SDL_Point* _Point) {
	struct Widget* _Widget = NULL;
	SDL_Point _Diff = {_Point->x - _Container->Rect.x, _Point->y - _Container->Rect.y};
	SDL_Point _WidgetPos;

	_Container->Rect.x = _Point->x;
	_Container->Rect.y = _Point->y;
	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Widget = _Container->Children[i];
		_WidgetPos.x = _Widget->Rect.x + _Diff.x;
		_WidgetPos.y = _Widget->Rect.y +_Diff.y;
		_Widget->SetPosition(_Widget, &_WidgetPos);
	}
}

struct Widget* ContainerOnFocus(struct Container* _Container, const SDL_Point* _Point) {
	struct Widget* _Widget = NULL;

	if(PointInAABB(_Point, &_Container->Rect) == 0)
			return 0;
	for(int i = 0; i < _Container->ChildCt; ++i) {
		if((_Widget = _Container->Children[i]->OnFocus(_Container->Children[i], _Point)) != NULL)
			return _Widget;
	}
	return NULL;
}

int ContainerOnUnfocus(struct Container* _Container) {
	return 1;
}

int ContainerHorzFocChange(const struct Container* _Container) {
	return 0;
}

struct Widget* ContainerOnClick(struct Container* _Container, const SDL_Point* _Point) {
	struct Widget* _Widget = NULL;

	if(PointInAABB(_Point, &_Container->Rect) == 0)
		return 0;
	for(int i = 0; i < _Container->ChildCt; ++i)
		if((_Widget = _Container->Children[i]->OnClick(_Container->Children[i], _Point)) != NULL)
			return _Widget;
	return (struct Widget*) _Container;
}

struct Widget* MenuOnClick(struct Container* _Container, const SDL_Point* _Point) {
	struct Widget* _Widget = ContainerOnClick(_Container, _Point);

	return (_Widget != ((struct Container*)_Container)) ? (_Widget) : (NULL);
}

void ContainerOnDebug(const struct Container* _Container) {
	WidgetOnDebug((struct Widget*)_Container);
	for(int i = 0; i < _Container->ChildCt; ++i)
		_Container->Children[i]->OnDebug(_Container->Children[i]);
}

void VertConNewChild(struct Container* _Parent, struct Widget* _Child) {
	SDL_Point _Pos;

	ContainerPosChild(_Parent, _Child, &_Pos);
	_Pos.x = _Parent->Rect.x;
	_Child->SetPosition(_Child, &_Pos);
}

void FixedConNewChild(struct Container* _Parent, struct Widget* _Child) {
	_Child->Rect.x += _Parent->Rect.x;
	_Child->Rect.y += _Parent->Rect.y;
}

void HorzConNewChild(struct Container* _Parent, struct Widget* _Child) {
	SDL_Point _Pos;

	ContainerPosChild(_Parent, _Child, &_Pos);
	_Pos.y = _Parent->Rect.y;
	_Child->SetPosition(_Child, &_Pos);
}

void ContextItemNewChild(struct Container* _Parent, struct Widget* _Child) {
	SDL_Point _Pos;

	if(_Parent->Children[0] == _Child) {
		_Child->Rect.x = _Parent->Rect.x;
		_Child->Rect.y = _Parent->Rect.y;
		return;
	}
	ContainerPosChild(_Parent, _Child, &_Pos);
	_Child->SetPosition(_Child, &_Pos);
	//_Child->Rect.x = _Parent->Children[0]->Rect.w + _Parent->Spacing;
	//_Child->Rect.y = _Child->Rect.y - _Parent->Children[0]->Rect.h;
	_Parent->VertFocChange = _Parent->ChildCt;
}

void ContainerShrink(struct Container* _Container) {
	int _XOff = 0;
	int _NewW = 0;
	int _NewH = 0;
	const struct Widget* _Child = NULL;

	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Child = _Container->Children[i];
		if(((_Child->Rect.x - _Container->Rect.x) + _Child->Rect.w) > (_NewW + _XOff)) {
			_XOff = _Child->Rect.x - _Container->Rect.x;
			_NewW = _XOff + _Child->Rect.w;
		}
		if((_Child->Rect.y + _Child->Rect.h) > _NewH)
			_NewH += _Child->Rect.h;
	}
	_Container->Rect.w = _NewW;
	_Container->Rect.h = _NewH;
}

int WidgetGrow(struct Widget* _Widget, int _Width, int _Height) {
	_Widget->Rect.w += _Width;
	_Widget->Rect.h += _Height;
	//_Widget->IsVisible = WidgetCheckVisibility(_Widget);
	return 1;
}

void StaticRemChild(struct Container* _Parent, struct Widget* _Child) {
	int i;

	for(i = 0; i < _Parent->ChildrenSz; ++i) {
		if(_Parent->Children[i] == _Child) {
			_Parent->Children[i] = NULL;
			--_Parent->ChildCt;
			break;
		}
	}
}

void DynamicRemChild(struct Container* _Parent, struct Widget* _Child) {
	int i;

	for(i = 0; i < _Parent->ChildrenSz; ++i) {
		if(_Parent->Children[i] == _Child) {
			_Parent->Children[i] = NULL;
			--_Parent->ChildCt;
		for(i = i + 1; i < _Parent->ChildrenSz; ++i) {
			_Parent->Children[i - 1] = _Parent->Children[i];
		}
		if(i < _Parent->ChildrenSz)
			_Parent->Children[i] = NULL;
		break;
		}
	}
}

void WidgetOnEvent(struct Widget* _Widget, int _RefId, int _Key, int _KeyState, int _KeyMod) {
	struct KeyMouseState _State;
	struct WEvent _WEvent;

	KeyMouseStateClear(&_State);
	_State.KeyboardState = _KeyState;
	_State.KeyboardButton = _Key;
	_State.KeyboardMod = _KeyMod;

	if(g_GUIEvents->Size == g_GUIEvents->TblSz) {
		g_GUIEvents->Events = realloc(g_GUIEvents->Events, sizeof(struct WEvent) * g_GUIEvents->TblSz * 2);
		g_GUIEvents->TblSz *= 2;
	}
	_WEvent.Event = _State;
	_WEvent.WidgetId = _Widget->Id;
	_WEvent.RefId = _RefId;
	g_GUIEvents->Events[g_GUIEvents->Size++] = _WEvent;
}

void WidgetSetPosition(struct Widget* _Widget, const SDL_Point* _Pos) {
	struct Container* _Parent = _Widget->Parent;
	
	SDL_assert(_Pos->x >= 0 || _Pos->y >= 0);
	_Widget->Rect.x = _Pos->x;
	_Widget->Rect.y = _Pos->y;

	SDL_assert(_Widget->Parent != NULL);
	if((_Widget->Rect.x + _Widget->Rect.w) > (_Parent->Rect.x + _Parent->Rect.w))
		_Widget->Rect.x = (_Widget->Rect.x + _Widget->Rect.w) - (_Parent->Rect.x + _Parent->Rect.w);
	if((_Widget->Rect.y + _Widget->Rect.h) > (_Parent->Rect.y + _Parent->Rect.h))
		_Widget->Rect.y = (_Widget->Rect.y + _Widget->Rect.h) - (_Parent->Rect.y + _Parent->Rect.h);
}

struct Widget* WidgetOnClick(struct Widget* _Widget, const SDL_Point* _Point) {
	if(PointInAABB(_Point, &_Widget->Rect) == SDL_TRUE) {
		return _Widget;
	}
	return NULL;
}

void WidgetOnDebug(const struct Widget* _Widget) {
	SDL_RenderDrawRect(g_Renderer, &_Widget->Rect);
}

int WidgetCheckVisibility(const struct Widget* _Widget) {
	return (_Widget->Rect.x < (_Widget->Parent->Rect.x + _Widget->Parent->Rect.w)) & (_Widget->Rect.y < (_Widget->Parent->Rect.y + _Widget->Parent->Rect.h));
}

int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget) {
	return _Parent->Rect.x + (((_Parent ->Rect.w) / 2) - (_Widget ->Rect.w / 2));
}

struct Widget* WidgetOnFocus(struct Widget* _Widget, const SDL_Point* _Point) {
	if(PointInAABB(_Point, &_Widget->Rect) == 0)
		return NULL;
	return _Widget;
}

int WidgetOnUnfocus(struct Widget* _Widget) {
	return 1;
}

struct Container* WidgetTopParent(struct Widget* _Widget) {
	struct Container* _Parent = _Widget->Parent;

	while(_Parent != NULL) {
		_Parent = _Parent->Parent;
	}
	return _Parent;
}

void GuiZToTop(struct Container* _Container) {
	ILL_DESTROY(g_GuiZBuff.Top, _Container);
	if(g_GuiZBuff.Bot == _Container)
		g_GuiZBuff.Bot = NULL;
	ILL_CREATE(g_GuiZBuff.Top, _Container);

}
