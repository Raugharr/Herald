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
#include "../sys/Memory.h"

#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>

struct HashTable g_GuiSkins = {0};
struct GuiSkin* g_GuiSkinDefault = NULL;
struct GuiSkin* g_GuiSkinCurr = NULL;
int g_GUIMenuChange = 0;
uint32_t g_GUIId = 0;
struct GUIFocus* g_Focus = NULL;
struct Stack g_GUIStack = {NULL, 0};
struct LinkedList g_GuiZBuff = {0, NULL, NULL};

int NextGUIId(void) {return g_GUIId++;}

struct Container* CreateContainer(void) {
	return (struct Container*) malloc(sizeof(struct Container));
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
	return (struct Container*) g_GuiZBuff.Front->Data;
}

struct Container* GUIZBot(void) {
	return (struct Container*) g_GuiZBuff.Back->Data;
}

void GuiClear(lua_State* _State) {
	struct Container* _Container = NULL;
	
	if(g_GuiZBuff.Front == NULL)
		return;
	_Container = (struct Container*) g_GuiZBuff.Front->Data;
	while(g_GuiZBuff.Front != NULL) {
		_Container = (struct Container*) g_GuiZBuff.Front->Data;
		_Container->Widget.OnDestroy((struct Widget*) _Container, _State);
		LnkLstPopFront(&g_GuiZBuff);
	}
}

void GuiEmpty() {
	LnkLstClear(&g_GuiZBuff);
}


void ConstructWidget(struct Widget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State) {
	_Widget->Id = NextGUIId();
	_Widget->IsDraggable = 0;
	_Widget->LuaRef = LuaWidgetRef(_State);
	_Widget->CanFocus = 1;
	_Widget->IsVisible = 1;
	_Widget->OnClick = WidgetOnClick;
	_Widget->SetPosition = WidgetSetPosition;
	_Widget->OnDraw = NULL;
	_Widget->OnFocus = WidgetOnFocus;
	_Widget->OnUnfocus = WidgetOnUnfocus;
	_Widget->OnKeyUp = WidgetOnKeyUp;
	_Widget->OnKey = WidgetOnKey;
	_Widget->OnDestroy = NULL;
	_Widget->OnDebug = WidgetOnDebug;
	_Widget->Rect.x = _Rect->x;
	_Widget->Rect.y = _Rect->y;
	_Widget->Rect.w = _Rect->w;
	_Widget->Rect.h = _Rect->h;
	_Widget->OnDrag = WidgetOnDrag;
	_Widget->Clickable = 1;
	_Widget->Parent = NULL;
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

void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State) {
	if(_Parent != NULL) {
		_Widget->Skin = _Parent->Skin;
		_Widget->Widget.Style = _Parent->Skin->Container;
	} else {
		_Widget->Skin = g_GuiSkinCurr;
		_Widget->Widget.Style = _Widget->Skin->Container;
	}

	ConstructWidget((struct Widget*)_Widget, _Parent, _Rect, _State);
	_Widget->Children = NULL;
	_Widget->ChildrenSz = 0;
	_Widget->ChildCt = 0;
	_Widget->Widget.OnDraw = (int(*)(struct Widget*))ContainerOnDraw;
	_Widget->Widget.SetPosition = (void(*)(struct Widget*, const SDL_Point*))ContainerSetPosition;
	_Widget->Widget.OnFocus = (struct Widget*(*)(struct Widget*, const SDL_Point*))ContainerOnFocus;
	_Widget->Widget.OnUnfocus = (int(*)(struct Widget*))ContainerOnUnfocus;
	_Widget->Widget.OnDestroy = (void(*)(struct Widget*, lua_State*))DestroyContainer;
	_Widget->Widget.OnClick = (struct Widget*(*)(struct Widget*, const SDL_Point*))ContainerOnClick;
	_Widget->NewChild = NULL;
	_Widget->RemChild = DynamicRemChild;
	//_Widget->VertFocChange = 1;
	_Widget->HorzFocChange = ContainerHorzFocChange;
	_Widget->Widget.OnDebug = (void(*)(const struct Widget*))ContainerOnDebug;
	_Widget->Widget.OnDrag = (struct Widget*(*)(struct Widget*, const SDL_Point*))ContainerOnDrag;
	//if(_Parent == NULL)
	//else
	//	_Widget->Skin = _Parent->Skin;
}

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child, SDL_Point* _Pos) {
	int32_t _X = 0;//_Parent->Widget.Style->Margins.Left;
	int32_t _Y = 0;//_Parent->Widget.Style->Margins.Top;
	struct Widget* _Widget = NULL;

	for(int i = 0; i < _Parent->ChildCt - 1 && _Parent->Children[i] != NULL; ++i) {
		_Widget = _Parent->Children[i];
		_X += _Widget->Rect.w + _Widget->Style->Margins.Left + _Widget->Style->Margins.Right;
		_Y += _Widget->Rect.h + _Widget->Style->Margins.Bottom + _Widget->Style->Margins.Top;
	}
	_Pos->x = _X;// + _Parent->Widget.Style->Margins.Left;
	_Pos->y = _Y;// + _Parent->Widget.Style->Margins.Top;
}

struct Widget* ContainerOnDrag(struct Container* _Widget, const struct SDL_Point* _Pos) {
	struct Widget* _Child = NULL;

	if(_Widget->Widget.IsDraggable == 0) {
		for(int i = 0; i < _Widget->ChildCt; ++i) {
			_Child = _Widget->Children[i];
			if(_Child->OnDrag(_Child, _Pos) != NULL)
				return _Child;
		}
		return NULL;
	}
	return (PointInAABB(_Pos, &_Widget->Widget.Rect) != 0) ? (&_Widget->Widget) : (NULL);
}

/*
 * FIXME: Check if _Child already has a parent and if it does delete it from the old parent.
 */
void WidgetSetParent(struct Container* _Parent, struct Widget* _Child) {
	if(_Parent->Widget.Id == _Child->Id)
		return;
	//Check if _Child already has a parent.
	if(_Child->Parent != NULL) {
		struct Container* _OldParent = _Child->Parent;

		_OldParent->RemChild(_OldParent, _Child);
	}
	//Check if _Child is already a child.
	for(int i = 0; i < _Parent->ChildCt; ++i) {
		if(_Parent->Children[i] == _Child)
			return;
	}
	if(_Parent->Children == NULL) {
		_Parent->Children = calloc(2, sizeof(struct Widget*));
		memset(_Parent->Children, 0, sizeof(struct Widget*) * 2);
		_Parent->ChildrenSz = 2;
	} else if(_Parent->ChildCt == _Parent->ChildrenSz) {
		_Parent->Children = Realloc(_Parent->Children, sizeof(struct Widget*) * _Parent->ChildrenSz * 2);
		for(int i = _Parent->ChildrenSz; i < _Parent->ChildrenSz * 2; ++i)
			_Parent->Children[i] = NULL;
		_Parent->ChildrenSz *= 2;
	}
	_Parent->Children[_Parent->ChildCt++] = _Child;
	_Child->Parent = _Parent;
	_Parent->NewChild(_Parent, _Child);
	if(_Child->Rect.x + _Child->Rect.w > _Parent->Widget.Rect.x + _Parent->Widget.Rect.w) {
		uint32_t _Temp = _Child->Rect.w;

		_Child->Rect.w = _Parent->Widget.Rect.w - _Child->Rect.x;
		if(_Child->Rect.w < 0)
			_Child->Rect.w = 0;
		Log(ELOG_WARNING, "Widget %i (%i, %i), cannot fit in parent, width is now %i was %i.", _Child->Id, _Child->Rect.x, _Child->Rect.y, _Child->Rect.w, _Temp);
	}
	if(_Child->Rect.y +_Child->Rect.h > _Parent->Widget.Rect.y +_Parent->Widget.Rect.h) {
		uint32_t _Temp = _Child->Rect.h;

		_Child->Rect.h = _Parent->Widget.Rect.h - _Child->Rect.y;
		if(_Child->Rect.h < 0)
			_Child->Rect.h = 0;
		Log(ELOG_WARNING, "Widget %i (%i, %i), cannot fit in parent, height is now %i was %i.", _Child->Id, _Child->Rect.x, _Child->Rect.y, _Child->Rect.h, _Temp);
	}
}

void WidgetOnKeyUp(struct Widget* _Widget, SDL_KeyboardEvent* _Event) {

}

void WidgetOnKey(struct Widget* _Widget, unsigned int _Key, unsigned int _KeyMod) {

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
	free(_Widget);
}

void DestroyContainer(struct Container* _Container, lua_State* _State) {
	for(int i = 0; i < _Container->ChildCt; ++i) {
		if(_Container->Children[i] == NULL)
			continue;
		_Container->Children[i]->OnDestroy(_Container->Children[i], _State);
	}
	DestroyWidget((struct Widget*)_Container, _State);
	GuiZBuffRem(_Container);
}

int ContainerOnDraw(struct Container* _Container) {
	struct Widget* _Widget = NULL;

	SDL_SetRenderDrawColor(g_Renderer,
		_Container->Widget.Style->Background.r, 
		_Container->Widget.Style->Background.g, 
		_Container->Widget.Style->Background.b, 
		_Container->Widget.Style->Background.a);
	SDL_RenderFillRect(g_Renderer, &_Container->Widget.Rect);
	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Widget = _Container->Children[i];
		/*if(_Widget->Rect.x >= _Container->Widget.Rect.x && _Widget->Rect.y >= _Container->Widget.Rect.y
				&& _Widget->Rect.x + _Widget->Rect.w <= _Container->Widget.Rect.x + _Container->Widget.Rect.w
				&& _Widget->Rect.y + _Widget->Rect.h <= _Container->Widget.Rect.y + _Container->Widget.Rect.h) {*/

		if(_Widget->OnDraw(_Widget) == 0)
			return 0;
		//}
	}
	SDL_SetRenderDrawColor(g_Renderer, 0x7F, 0x7F, 0x7F, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawRect(g_Renderer, &_Container->Widget.Rect);
	SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	return 1;
}

int MenuOnDraw(struct Container* _Container) {
	struct Widget* _Widget = NULL;

	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Widget = _Container->Children[i];
		if(_Widget->OnDraw(_Widget) == 0)
			return 0;
	}
	return 1;
}

void ContainerSetPosition(struct Container* _Container, const struct SDL_Point* _Point) {
	struct Widget* _Widget = NULL;
	//SDL_Point _Diff = {_Point->x - _Container->Widget.Rect.x, _Point->y - _Container->Widget.Rect.y};
	SDL_Point _WidgetPos = {0, 0};
	SDL_Point _OldPos = {_Container->Widget.Rect.x, _Container->Widget.Rect.y};
	SDL_Point _Offset = {0, 0};

	if(_Container->Widget.Parent != NULL) {
		_Offset.x = _Container->Widget.Parent->Widget.Rect.x;
		_Offset.y = _Container->Widget.Parent->Widget.Rect.y;
	}

	_Container->Widget.Rect.x = _Offset.x + _Point->x;
	_Container->Widget.Rect.y = _Offset.y + _Point->y;
	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Widget = _Container->Children[i];
		_WidgetPos.x = _Widget->Rect.x - _OldPos.x;
		_WidgetPos.y = _Widget->Rect.y - _OldPos.y;
		_Widget->SetPosition(_Widget, &_WidgetPos);
	}
}

struct Widget* ContainerOnFocus(struct Container* _Container, const SDL_Point* _Point) {
	struct Widget* _Widget = NULL;

	if(PointInAABB(_Point, &_Container->Widget.Rect) == 0)
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

	if(PointInAABB(_Point, &_Container->Widget.Rect) == 0 || _Container->Widget.Clickable == 0)
		return 0;
	for(int i = 0; i < _Container->ChildCt; ++i)
		if((_Widget = _Container->Children[i]->OnClick(_Container->Children[i], _Point)) != NULL)
			return _Widget;
	return (struct Widget*) _Container;
}

struct Widget* MenuOnClick(struct Container* _Container, const SDL_Point* _Point) {
	struct Widget* _Widget = ContainerOnClick(_Container, _Point);

	return (_Widget != ((struct Widget*)_Container)) ? (_Widget) : (NULL);
}

void ContainerOnDebug(const struct Container* _Container) {
	WidgetOnDebug((struct Widget*)_Container);
	for(int i = 0; i < _Container->ChildCt; ++i)
		_Container->Children[i]->OnDebug(_Container->Children[i]);
}

void VertConNewChild(struct Container* _Parent, struct Widget* _Child) {
	SDL_Point _Pos;

	ContainerPosChild(_Parent, _Child, &_Pos);
	_Pos.x = 0;
	_Child->SetPosition(_Child, &_Pos);
}

void FixedConNewChild(struct Container* _Parent, struct Widget* _Child) {
//	_Child->Rect.x += _Parent->Widget.Rect.x;
//	_Child->Rect.y += _Parent->Widget.Rect.y;
}

void HorzConNewChild(struct Container* _Parent, struct Widget* _Child) {
	SDL_Point _Pos;

	ContainerPosChild(_Parent, _Child, &_Pos);
	_Pos.y = 0;
	_Child->SetPosition(_Child, &_Pos);
}

void ContextItemNewChild(struct Container* _Parent, struct Widget* _Child) {
	SDL_Point _Pos;

	if(_Parent->Children[0] == _Child) {
		_Child->Rect.x = _Parent->Widget.Rect.x;
		_Child->Rect.y = _Parent->Widget.Rect.y;
		return;
	}
	ContainerPosChild(_Parent, _Child, &_Pos);
	_Child->SetPosition(_Child, &_Pos);
	//_Parent->VertFocChange = _Parent->ChildCt;
}

void ContainerShrink(struct Container* _Container) {
	int _NewW = 0;
	int _NewH = 0;
	const struct Widget* _Child = NULL;
	const struct Margin* _Margins = NULL;

	for(int i = 0; i < _Container->ChildCt; ++i) {
		_Child = _Container->Children[i];
		_Margins = &_Child->Style->Margins;
		if((_Child->Rect.x - _Container->Widget.Rect.x) + _Child->Rect.w + _Margins->Right > _NewW)
			_NewW = (_Child->Rect.x - _Container->Widget.Rect.x) + _Child->Rect.w;
		/*if(((_Child->Rect.x - _Container->Widget.Rect.x) + _Child->Rect.w) > (_NewW + _XOff)) {
			_XOff = _Child->Rect.x - _Container->Widget.Rect.x;
			_NewW = _XOff + _Child->Rect.w;
		}*/
		if(((_Child->Rect.y - _Container->Widget.Rect.y) + _Child->Rect.h + _Margins->Bottom) > _NewH)
			_NewH = (_Child->Rect.y - _Container->Widget.Rect.y) + _Child->Rect.h;
			//_NewH += _Child->Rect.h + _Margins->Bottom + _Margins->Top;
	}
	_Container->Widget.Rect.w = _NewW;
	_Container->Widget.Rect.h = _NewH;
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

bool WidgetSetWidth(struct Widget* _Widget, int _Width) {
	const struct Container* _Parent = _Widget->Parent;
	//const struct SDL_Rect* _Rect = NULL;

	if(_Widget->Rect.x + _Width > _Parent->Widget.Rect.x + _Parent->Widget.Rect.w) {
		Log(ELOG_WARNING, "Widget %d's width cannot be set to %d cannot fit into parent.", _Widget->Id, _Widget->Rect.y);
		return false;
	}
	_Widget->Rect.w = _Width;
/*	for(int i = 0; i < _Parent->ChildCt; ++i) {
		if(_Parent->Children[i] == _Widget)
			continue;
		_Rect = &_Parent->Children[i]->Rect;
		if(_Rect->x >= _Widget->Rect.x && _Rect->x + _Rect->w <= _Widget->Rect.x + _Widget->Rect.w) {
			_Parent->Children[i]->Rect.x = _Widget->Rect.x + _Widget->Rect.w + 1;
		}
	}*/
	return true;
}

bool WidgetSetHeight(struct Widget* _Widget, int _Height) {
	const struct Container* _Parent = _Widget->Parent;
//	const struct SDL_Rect* _Rect = NULL;

	if(_Widget->Rect.y + _Height > _Parent->Widget.Rect.y + _Parent->Widget.Rect.h) {
		Log(ELOG_WARNING, "Widget %d's height cannot be set to %d cannot fit into parent.", _Widget->Id, _Widget->Rect.y);
		return false;
	}
	_Widget->Rect.h = _Height;
/*	for(int i = 0; i < _Parent->ChildCt; ++i) {
		if(_Parent->Children[i] == _Widget)
			continue;
		_Rect = &_Parent->Children[i]->Rect;
		if(_Rect->y >= _Widget->Rect.y && _Rect->y + _Rect->h <= _Widget->Rect.y + _Widget->Rect.y) {
			_Parent->Children[i]->Rect.y = _Widget->Rect.y + _Widget->Rect.h + 1;
		}
	}*/
	return true;
}

void WidgetSetPosition(struct Widget* _Widget, const SDL_Point* _Pos) {
	//struct Container* _Parent = _Widget->Parent;
	
	SDL_assert(_Pos->x >= 0 || _Pos->y >= 0);
	_Widget->Rect.x = _Pos->x + _Widget->Parent->Widget.Rect.x;
	_Widget->Rect.y = _Pos->y + _Widget->Parent->Widget.Rect.y;

	SDL_assert(_Widget->Parent != NULL);
/*	if((_Widget->Rect.x + _Widget->Rect.w) > (_Parent->Widget.Rect.x + _Parent->Widget.Rect.w))
		_Widget->Rect.x = (_Widget->Rect.x + _Widget->Rect.w) - (_Parent->Widget.Rect.x + _Parent->Widget.Rect.w);
	if((_Widget->Rect.y + _Widget->Rect.h) > (_Parent->Widget.Rect.y + _Parent->Widget.Rect.h))
		_Widget->Rect.y = (_Widget->Rect.y + _Widget->Rect.h) - (_Parent->Widget.Rect.y + _Parent->Widget.Rect.h);
		*/
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
	return (_Widget->Rect.x < (_Widget->Parent->Widget.Rect.x + _Widget->Parent->Widget.Rect.w)) 
		& (_Widget->Rect.y < (_Widget->Parent->Widget.Rect.y + _Widget->Parent->Widget.Rect.h));
}

int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget) {
	return _Parent->Widget.Rect.x + (((_Parent->Widget.Rect.w) / 2) - (_Widget->Rect.w / 2));
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
		_Parent = _Parent->Widget.Parent;
	}
	return _Parent;
}

void GuiZToTop(struct Container* _Container) {
	struct LnkLst_Node* _Itr = g_GuiZBuff.Front;

	while(_Itr != NULL) {
		if((struct Container*) _Itr->Data == _Container) {
			LnkLstRemove(&g_GuiZBuff, _Itr);
			LnkLstPushFront(&g_GuiZBuff, _Container);
			return;
		}
		_Itr = _Itr->Next;
	}
	//assert(0);
}

void GuiDraw(void) {
	struct LnkLst_Node* _Itr = g_GuiZBuff.Back;
	struct Container* _Container = NULL;

	while(_Itr != NULL) {
		_Container = (struct Container*) _Itr->Data;
		_Container->Widget.OnDraw((struct Widget*) _Container);
		_Itr = _Itr->Prev;
	}
}

void GuiDrawDebug(void) {
	struct LnkLst_Node* _Itr = g_GuiZBuff.Back;
	struct Container* _Container = NULL;

	while(_Itr != NULL) {
		_Container = (struct Container*) _Itr->Data;
		_Container->Widget.OnDebug((struct Widget*) _Container);
		_Itr = _Itr->Prev;
	}
}

struct Widget* GuiFind(int _FuncOffset, const SDL_Point* _MousePos) {
	typedef struct Widget*(*GuiFindFunc)(struct Widget*, const SDL_Point*);

	struct LnkLst_Node* _Itr = g_GuiZBuff.Front;
	struct Container* _Container = NULL;
	struct Widget* _FoundWidget = NULL;
	GuiFindFunc* _Func = NULL;

	while(_Itr != NULL) {
		_Container = (struct Container*) _Itr->Data;
		_Func = (GuiFindFunc*) (((uint8_t*)_Container) + _FuncOffset);
		if((_FoundWidget = (*_Func)((struct Widget*) _Container, _MousePos)) != NULL) {
			return _FoundWidget;
		}
		_Itr = _Itr->Next;
	}
	return NULL;
}

void GuiZBuffAdd(struct Container* _Container) {
	LnkLstPushFront(&g_GuiZBuff, _Container);
}

void GuiZBuffRem(struct Container* _Container) {
	struct LnkLst_Node* _Itr = g_GuiZBuff.Front;
	
	while(_Itr != NULL) {
		if(_Itr->Data == _Container) {
			LnkLstRemove(& g_GuiZBuff, _Itr);
			return;
		}
		_Itr = _Itr->Next;	
	}
}

void GetBlendValue(const SDL_Color* _Src, const SDL_Color* _Dest, SDL_Color* _Out) {
	(*_Out).r = (((double)_Dest->r) / ((double)_Src->r)) * 255;
	(*_Out).g = (((double)_Dest->g) / ((double)_Src->g)) * 255;
	(*_Out).b = (((double)_Dest->b) / ((double)_Src->b)) * 255;
	(*_Out).a = (((double)_Dest->a) / ((double)_Src->a)) * 255;
}
