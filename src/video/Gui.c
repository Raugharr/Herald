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
uint32_t g_GUIId = 0;
struct GUIFocus* g_Focus = NULL;
struct Stack g_GUIStack = {NULL, 0};
struct LinkedList g_GuiZBuff = {0, NULL, NULL};
static lua_State* g_GuiLState = NULL;

uint32_t NextGUIId(void) {return g_GUIId++;}
void GuiSetLuaState(lua_State* State) {g_GuiLState = State;}
lua_State* GuiGetLuaState() {return g_GuiLState;}

struct Container* CreateContainer(void) {
	struct Container* Container = (struct Container*) malloc(sizeof(struct Container));

	memset(Container, 0, sizeof(*Container));
	return Container;
}

struct GUIFocus* CreateGUIFocus(void) {
	struct GUIFocus* New = (struct GUIFocus*) malloc(sizeof(struct GUIFocus));
	New->Id = 0;
	New->Index = 0;
	New->Parent = NULL;
	New->Prev = NULL;
	return New;
}

struct Container* GUIZTop(void) {
	return (struct Container*) g_GuiZBuff.Front->Data;
}

struct Container* GUIZBot(void) {
	return (struct Container*) g_GuiZBuff.Back->Data;
}

void GuiClear() {
	struct Container* Container = NULL;
	
	if(g_GuiZBuff.Front == NULL)
		return;
	Container = (struct Container*) g_GuiZBuff.Front->Data;
	while(g_GuiZBuff.Front != NULL) {
		Container = (struct Container*) g_GuiZBuff.Front->Data;
		Container->Widget.OnDestroy((struct Widget*) Container);
		LnkLstPopFront(&g_GuiZBuff);
	}
}

void GuiEmpty() {
	LnkLstClear(&g_GuiZBuff);
}


void ConstructWidget(struct Widget* Widget, struct Container* Parent, SDL_Rect* Rect, uint32_t ClassType) {
	Widget->Id = NextGUIId();
	Widget->IsDraggable = 0;
	/*
	 * If a class doesn't want to use ClassType as its actual ClassType it can calloc
	 * WidgetSetLuaClass to the class it wants to use before calling ConstructWidget.
	 * Here we check if someone has done that and then call LuaWidgetRef with the correct
	 * ClassType instead of the given ClassType.
	 */
	Assert(Widget->LuaRef < LOBJ_SIZE);
	if(Widget->LuaRef != 0) ClassType = Widget->LuaRef;
	Widget->LuaRef = LuaWidgetRef(g_GuiLState, Widget, ClassType);
	Widget->CanFocus = 1;
	Widget->IsVisible = 1;
	Widget->OnClick = WidgetOnClick;
	Widget->SetPosition = WidgetSetPosition;
	Widget->OnDraw = NULL;
	Widget->OnFocus = WidgetOnFocus;
	Widget->OnUnfocus = WidgetOnUnfocus;
	Widget->OnKey = WidgetOnKey;
	Widget->OnDestroy = NULL;
	Widget->OnDebug = WidgetOnDebug;
	Widget->OnDrag = WidgetOnDrag;
	Widget->Clickable = 1;
	Widget->Parent = NULL;
	Widget->Rect = *Rect;

	if(Parent != NULL) {
		const struct GuiStyle* Style = Widget->Style;

		WidgetSetParent(Parent, Widget);
	/*	if(WidgetCheckVisibility(Widget) == 0) {
			WidgetGrow((struct Widget*)Widget->Parent, Widget->Rect.w, Widget->Rect.h);
			Widget->IsVisible = WidgetCheckVisibility(Widget);
		}*/
	} else if(GuiGetParentHook() != NULL) {
		WidgetSetParent(GuiGetParentHook(), Widget);
	} else {
		Widget->Parent = NULL;
	}
}

void ConstructContainer(struct Container* Widget, struct Container* Parent, SDL_Rect* Rect) {
	if(Parent != NULL) {
		Widget->Skin = Parent->Skin;
		Widget->Widget.Style = Parent->Skin->Container;
	} else {
		Widget->Skin = g_GuiSkinCurr;
		Widget->Widget.Style = Widget->Skin->Container;
	}

	ConstructWidget((struct Widget*)Widget, Parent, Rect, LOBJ_CONTAINER);
	Widget->Children = NULL;
	Widget->ChildrenSz = 0;
	Widget->ChildCt = 0;
	Widget->Widget.OnDraw = ContainerOnDraw;
	Widget->Widget.SetPosition = ContainerSetPosition;
	Widget->Widget.OnFocus = ContainerOnFocus;
	Widget->Widget.OnUnfocus = ContainerOnUnfocus;
	Widget->Widget.OnDestroy = DestroyContainer;
	Widget->Widget.OnClick = ContainerOnClick;
	Widget->NewChild = FixedConNewChild;
	Widget->RemChild = DynamicRemChild;
	Widget->Widget.OnDebug = (void(*)(const struct Widget*))ContainerOnDebug;
	Widget->Widget.OnDrag = (struct Widget*(*)(struct Widget*, const SDL_Point*))ContainerOnDrag;
}

void ContainerPosChild(struct Container* Parent, struct Widget* Child, SDL_Point* Pos) {
	int32_t X = 0;
	int32_t Y = 0;
	struct Widget* Widget = NULL;

	for(int i = 0; i < Parent->ChildCt - 1 && Parent->Children[i] != NULL; ++i) {
		Widget = Parent->Children[i];
		X += Widget->Rect.w + Widget->Style->Margins.Left + Widget->Style->Margins.Right;
		Y += Widget->Rect.h + Widget->Style->Margins.Bottom + Widget->Style->Margins.Top;
	}
	//X -= (Child->Style->Padding.Left + Child->Style->Padding.Right) / 2;
	//Y -= (Child->Style->Padding.Bottom + Child->Style->Padding.Top) / 2;
	Pos->x = X;
	Pos->y = Y;
}

struct Widget* ContainerOnDrag(struct Container* Widget, const struct SDL_Point* Pos) {
	struct Widget* Child = NULL;

	if(Widget->Widget.IsDraggable == 0) {
		for(int i = 0; i < Widget->ChildCt; ++i) {
			Child = Widget->Children[i];
			if(Child->OnDrag(Child, Pos) != NULL)
				return Child;
		}
		return NULL;
	}
	return (PointInAABB(Pos, &Widget->Widget.Rect) != 0) ? (&Widget->Widget) : (NULL);
}

/*
 * FIXME: Check if Child already has a parent and if it does delete it from the old parent.
 */
void WidgetSetParent(struct Container* Parent, struct Widget* Child) {
	if(Parent->Widget.Id == Child->Id)
		return;
	//Check if Child already has a parent.
	if(Child->Parent != NULL) {
		struct Container* OldParent = Child->Parent;

		OldParent->RemChild(OldParent, Child);
	}
	//Check if Child is already a child.
	for(int i = 0; i < Parent->ChildCt; ++i) {
		if(Parent->Children[i] == Child)
			return;
	}
	if(Parent->Children == NULL) {
		Parent->Children = calloc(2, sizeof(struct Widget*));
		memset(Parent->Children, 0, sizeof(struct Widget*) * 2);
		Parent->ChildrenSz = 2;
	} else if(Parent->ChildCt == Parent->ChildrenSz) {
		Parent->Children = Realloc(Parent->Children, sizeof(struct Widget*) * Parent->ChildrenSz * 2);
		for(int i = Parent->ChildrenSz; i < Parent->ChildrenSz * 2; ++i)
			Parent->Children[i] = NULL;
		Parent->ChildrenSz *= 2;
	}
	Parent->Children[Parent->ChildCt++] = Child;
	Child->Parent = Parent;
	Parent->NewChild(Parent, Child);
	if(Child->Rect.x + Child->Rect.w > Parent->Widget.Rect.x + Parent->Widget.Rect.w) {
		uint32_t Temp = Child->Rect.w;

		Child->Rect.w = Parent->Widget.Rect.w - Child->Rect.x;
		if(Child->Rect.w < 0)
			Child->Rect.w = 0;
		Log(ELOG_WARNING, "Widget %i (%i, %i), cannot fit in parent, width is now %i was %i.", Child->Id, Child->Rect.x, Child->Rect.y, Child->Rect.w, Temp);
	}
	if(Child->Rect.y + Child->Rect.h > Parent->Widget.Rect.y + Parent->Widget.Rect.h) {
		uint32_t Temp = Child->Rect.h;

		Child->Rect.h = Parent->Widget.Rect.h - Child->Rect.y;
		if(Child->Rect.h < 0)
			Child->Rect.h = 0;
		Log(ELOG_WARNING, "Widget %i (%i, %i), cannot fit in parent, height is now %i was %i.", Child->Id, Child->Rect.x, Child->Rect.y, Child->Rect.h, Temp);
	}
}

void AddMargins(struct Widget* Widget, SDL_Point* Pos) {
	Pos->x += Widget->Style->Margins.Left;
	Pos->y += Widget->Style->Margins.Top;
}

void WidgetOnKey(struct Widget* Widget, const union UWidgetOnKey* Event) {

}

void WidgetSetVisibility(struct Widget* Widget, int Visibility) {
	Widget->IsVisible = (WidgetCheckVisibility(Widget) && (Visibility != 0));
}

struct Widget* WidgetOnDrag(struct Widget* Widget, const struct SDL_Point* Pos) {
	if(PointInAABB(Pos, &Widget->Rect) != 0)
		return NULL;
	return (Widget->IsDraggable == 0) ? (NULL) : (Widget);
}

void DestroyWidget(struct Widget* Widget) {
	struct Container* Parent = Widget->Parent;

	if(Parent != NULL)
		Parent->RemChild(Parent, Widget);
	if(GetFocusableWidget() == Widget)
		FocusableWidgetNull();
	LuaWidgetUnref(g_GuiLState, Widget);
	free(Widget);
}

void DestroyContainer(struct Widget* Widget) {
	struct Container* Container = (struct Container*)Widget;

	for(int i = 0; i < Container->ChildCt; ++i) {
		if(Container->Children[i] == NULL)
			continue;
		Container->Children[i]->OnDestroy(Container->Children[i]);
	}
	DestroyWidget(Widget);
	GuiZBuffRem(Container);
}

int ContainerOnDraw(struct Widget* Widget) {
	struct Widget* Child = NULL;
	struct Container* Container = (struct Container*)Widget;

	SDL_SetRenderDrawColor(g_Renderer,
		Container->Widget.Style->Background.r, 
		Container->Widget.Style->Background.g, 
		Container->Widget.Style->Background.b, 
		Container->Widget.Style->Background.a);
	SDL_RenderFillRect(g_Renderer, &Container->Widget.Rect);
	SDL_SetRenderDrawColor(g_Renderer,
		Container->Widget.Style->BorderColor.r, 
		Container->Widget.Style->BorderColor.g, 
		Container->Widget.Style->BorderColor.b, 
		Container->Widget.Style->BorderColor.a);
	SDL_RenderDrawRect(g_Renderer, &Container->Widget.Rect);
	for(int i = 0; i < Container->ChildCt; ++i) {
		Child = Container->Children[i];
		if(Child->IsVisible == false) continue;
		if(Child->OnDraw(Child) == 0) {
			Assert(true);
			Log(ELOG_ERROR, "Widget #%i failed to draw.", Child->Id);
			continue;
		}
	}
//	SDL_SetRenderDrawColor(g_Renderer, 0x7F, 0x7F, 0x7F, SDL_ALPHA_OPAQUE);
//	SDL_RenderDrawRect(g_Renderer, &Container->Widget.Rect);
	SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	return 1;
}

int MenuOnDraw(struct Container* Container) {
	struct Widget* Widget = NULL;

	for(int i = 0; i < Container->ChildCt; ++i) {
		Widget = Container->Children[i];
		if(Widget->OnDraw(Widget) == 0) {
			Log(ELOG_ERROR, "Widget #%i failed to draw.", Widget->Id);
			continue;
		}
	}
	return 1;
}

void ContainerSetPosition(struct Widget* Widget, const struct SDL_Point* Point) {
	struct Container* Container = (struct Container*)Widget;
	struct Widget* Child = NULL;
	//SDL_Point Diff = {_Point->x - Container->Widget.Rect.x, Point->y - Container->Widget.Rect.y};
	SDL_Point ChildPos = {0, 0};
	SDL_Point OldPos = {Container->Widget.Rect.x, Container->Widget.Rect.y};
	SDL_Point Offset = {0, 0};

	if(Container->Widget.Parent != NULL) {
		Offset.x = Container->Widget.Parent->Widget.Rect.x;
		Offset.y = Container->Widget.Parent->Widget.Rect.y;
	}

	Widget->Rect.x = Offset.x + Point->x;
	Widget->Rect.y = Offset.y + Point->y;
	for(int i = 0; i < Container->ChildCt; ++i) {
		Child = Container->Children[i];
		ChildPos.x = Child->Rect.x - OldPos.x;
		ChildPos.y = Child->Rect.y - OldPos.y;
		Child->SetPosition(Child, &ChildPos);
	}
}

struct Widget* ContainerOnFocus(struct Widget* Widget, const SDL_Point* Point) {
	struct Container* Container = (struct Container*)Widget;
	struct Widget* Child = NULL;

	if(PointInAABB(Point, &Container->Widget.Rect) == 0)
			return 0;
	for(int i = 0; i < Container->ChildCt; ++i) {
		if(Container->Children[i]->IsVisible == false) continue;
		if((Child = Container->Children[i]->OnFocus(Container->Children[i], Point)) != NULL)
			return Child;
	}
	return NULL;
}

int ContainerOnUnfocus(struct Widget* Widget) {
	return 1;
}

int ContainerHorzFocChange(const struct Widget* Widget) {
	return 0;
}

struct Widget* ContainerOnClick(struct Widget* Widget, const SDL_Point* Point) {
	struct Container* Container = (struct Container*)Widget;
	struct Widget* Child = NULL;

	if(PointInAABB(Point, &Container->Widget.Rect) == 0 || Container->Widget.Clickable == 0)
		return NULL;
	for(int i = 0; i < Container->ChildCt; ++i) {
		if(Container->Children[i]->IsVisible == false) continue;
		if((Child = Container->Children[i]->OnClick(Container->Children[i], Point)) != NULL) {
			return Child;
		}
	}
	return Widget;
}

struct Widget* MenuOnClick(struct Widget* Widget, const SDL_Point* Point) {
	struct Widget* Child = ContainerOnClick(Widget, Point);

	return (Child != Widget) ? (Child) : (NULL);
}

void ContainerOnDebug(const struct Widget* Widget) {
	struct Container* Container = (struct Container*) Widget;
	WidgetOnDebug(Widget);
	for(int i = 0; i < Container->ChildCt; ++i)
		Container->Children[i]->OnDebug(Container->Children[i]);
}

void VertConNewChild(struct Container* Parent, struct Widget* Child) {
	SDL_Point Pos;

	ContainerPosChild(Parent, Child, &Pos);
	Pos.x = 0;
	AddMargins(Child, &Pos);
	Child->SetPosition(Child, &Pos);
}

void FixedConNewChild(struct Container* Parent, struct Widget* Child) {
	SDL_Point Pos = {0};

	Child->SetPosition(Child, &Pos);
}

void HorzConNewChild(struct Container* Parent, struct Widget* Child) {
	SDL_Point Pos;

	ContainerPosChild(Parent, Child, &Pos);
	Pos.y = 0;
	AddMargins(Child, &Pos);
	Child->SetPosition(Child, &Pos);
}

void ContextItemNewChild(struct Container* Parent, struct Widget* Child) {
	SDL_Point Pos;

	if(Parent->Children[0] == Child) {
		Child->Rect.x = Parent->Widget.Rect.x;
		Child->Rect.y = Parent->Widget.Rect.y;
		return;
	}
	ContainerPosChild(Parent, Child, &Pos);
	Child->SetPosition(Child, &Pos);
}

void ContainerShrink(struct Container* Container) {
	int NewW = 0;
	int NewH = 0;
	const struct Widget* Child = NULL;
	const struct Margin* Margins = NULL;

	for(int i = 0; i < Container->ChildCt; ++i) {
		Child = Container->Children[i];
		Margins = &Child->Style->Margins;
		if((Child->Rect.x - Container->Widget.Rect.x) + Child->Rect.w + Margins->Right > NewW)
			NewW = (Child->Rect.x - Container->Widget.Rect.x) + Child->Rect.w;
		if(((Child->Rect.y - Container->Widget.Rect.y) + Child->Rect.h + Margins->Bottom) > NewH)
			NewH = (Child->Rect.y - Container->Widget.Rect.y) + Child->Rect.h;
	}
	Container->Widget.Rect.w = NewW;
	Container->Widget.Rect.h = NewH;
}

int WidgetGrow(struct Widget* Widget, int Width, int Height) {
	Widget->Rect.w += Width;
	Widget->Rect.h += Height;
	//_Widget->IsVisible = WidgetCheckVisibility(Widget);
	return 1;
}

void StaticRemChild(struct Container* Parent, struct Widget* Child) {
	int i;

	for(i = 0; i < Parent->ChildrenSz; ++i) {
		if(Parent->Children[i] == Child) {
			Parent->Children[i] = NULL;
			--Parent->ChildCt;
			break;
		}
	}
}

void DynamicRemChild(struct Container* Parent, struct Widget* Child) {
	int i;

	for(i = 0; i < Parent->ChildrenSz; ++i) {
		if(Parent->Children[i] == Child) {
			Parent->Children[i] = NULL;
			--Parent->ChildCt;
		for(i = i + 1; i < Parent->ChildrenSz; ++i) {
			Parent->Children[i - 1] = Parent->Children[i];
		}
		if(i < Parent->ChildrenSz)
			Parent->Children[i] = NULL;
		break;
		}
	}
}


int WidgetOnDraw(struct Widget* Widget) {
	return Widget->IsVisible;
}

bool WidgetSetWidth(struct Widget* Widget, int Width) {
	const struct Container* Parent = Widget->Parent;

	if(Widget->Rect.x + Width > Parent->Widget.Rect.x + Parent->Widget.Rect.w) {
		Log(ELOG_WARNING, "Widget %d's width cannot be set to %d cannot fit into parent.", Widget->Id, Widget->Rect.y);
		return false;
	}
	Widget->Rect.w = Width;
	return true;
}

bool WidgetSetHeight(struct Widget* Widget, int Height) {
	const struct Container* Parent = Widget->Parent;
//	const struct SDL_Rect* Rect = NULL;

	if(Widget->Rect.y + Height > Parent->Widget.Rect.y + Parent->Widget.Rect.h) {
		Log(ELOG_WARNING, "Widget %d's height cannot be set to %d cannot fit into parent.", Widget->Id, Widget->Rect.y);
		return false;
	}
	Widget->Rect.h = Height;
/*	for(int i = 0; i < Parent->ChildCt; ++i) {
		if(Parent->Children[i] == Widget)
			continue;
		Rect = &Parent->Children[i]->Rect;
		if(Rect->y >= Widget->Rect.y && Rect->y + Rect->h <= Widget->Rect.y + Widget->Rect.y) {
			Parent->Children[i]->Rect.y = Widget->Rect.y + Widget->Rect.h + 1;
		}
	}*/
	return true;
}

void WidgetSetPosition(struct Widget* Widget, const SDL_Point* Pos) {
	//struct Container* Parent = Widget->Parent;
	
	SDL_assert(Pos->x >= 0 || Pos->y >= 0);
	SDL_assert(Widget->Parent != NULL);
	Widget->Rect.x = Pos->x + Widget->Parent->Widget.Rect.x;
	Widget->Rect.y = Pos->y + Widget->Parent->Widget.Rect.y;

}

struct Widget* WidgetOnClick(struct Widget* Widget, const SDL_Point* Point) {
	if(PointInAABB(Point, &Widget->Rect) == SDL_TRUE) {
		return Widget;
	}
	return NULL;
}

void WidgetOnDebug(const struct Widget* Widget) {
	SDL_RenderDrawRect(g_Renderer, &Widget->Rect);
}

int WidgetCheckVisibility(const struct Widget* Widget) {
	return (Widget->Rect.x < (Widget->Parent->Widget.Rect.x + Widget->Parent->Widget.Rect.w)) 
		& (Widget->Rect.y < (Widget->Parent->Widget.Rect.y + Widget->Parent->Widget.Rect.h));
}

int GetHorizontalCenter(const struct Container* Parent, const struct Widget* Widget) {
	return Parent->Widget.Rect.x + (((Parent->Widget.Rect.w) / 2) - (Widget->Rect.w / 2));
}

struct Widget* WidgetOnFocus(struct Widget* Widget, const SDL_Point* Point) {
	if(PointInAABB(Point, &Widget->Rect) == 0)
		return NULL;
	return Widget;
}

int WidgetOnUnfocus(struct Widget* Widget) {
	return 1;
}

struct Container* WidgetTopParent(struct Widget* Widget) {
	struct Container* Parent = Widget->Parent;

	while(Parent != NULL) {
		Parent = Parent->Widget.Parent;
	}
	return Parent;
}

void GuiZToTop(struct Container* Container) {
	struct LnkLst_Node* Itr = g_GuiZBuff.Front;

	while(Itr != NULL) {
		if((struct Container*) Itr->Data == Container) {
			LnkLstRemove(&g_GuiZBuff, Itr);
			LnkLstPushFront(&g_GuiZBuff, Container);
			return;
		}
		Itr = Itr->Next;
	}
	//assert(0);
}

void GuiDraw(void) {
	struct LnkLst_Node* Itr = g_GuiZBuff.Back;
	struct Container* Container = NULL;

	while(Itr != NULL) {
		Container = (struct Container*) Itr->Data;
		Container->Widget.OnDraw((struct Widget*) Container);
		Itr = Itr->Prev;
	}
}

void GuiMenuThink(lua_State* State) {
	struct LnkLst_Node* Itr = g_GuiZBuff.Back;
	struct Container* Container = NULL;

	while(Itr != NULL) {
		Container = (struct Container*) Itr->Data;
		GuiMenuOnThink(State, Container);
		Itr = Itr->Prev;
	}
}

void GuiDrawDebug(void) {
	struct LnkLst_Node* Itr = g_GuiZBuff.Back;
	struct Container* Container = NULL;

	while(Itr != NULL) {
		Container = (struct Container*) Itr->Data;
		Container->Widget.OnDebug((struct Widget*) Container);
		Itr = Itr->Prev;
	}
}

struct Widget* GuiGetBack(void) {
	struct LnkLst_Node* Node = g_GuiZBuff.Back;

	return (Node == NULL) ? (NULL) : (Node->Data);
}

struct Widget* GuiFind(int FuncOffset, const SDL_Point* MousePos) {
	typedef struct Widget*(*GuiFindFunc)(struct Widget*, const SDL_Point*);

	struct LnkLst_Node* Itr = g_GuiZBuff.Front;
	struct Container* Container = NULL;
	struct Widget* FoundWidget = NULL;
	GuiFindFunc* Func = NULL;

	while(Itr != NULL) {
		Container = (struct Container*) Itr->Data;
		Func = (GuiFindFunc*) (((uint8_t*)Container) + FuncOffset);
		if((FoundWidget = (*Func)((struct Widget*) Container, MousePos)) != NULL) {
			return FoundWidget;
		}
		Itr = Itr->Next;
	}
	return NULL;
}

void GuiZBuffAdd(struct Container* Container) {
	LnkLstPushFront(&g_GuiZBuff, Container);
}

void GuiZBuffRem(struct Container* Container) {
	struct LnkLst_Node* Itr = g_GuiZBuff.Front;
	
	while(Itr != NULL) {
		if(Itr->Data == Container) {
			LnkLstRemove(& g_GuiZBuff, Itr);
			return;
		}
		Itr = Itr->Next;	
	}
}

void GetBlendValue(const SDL_Color* Src, const SDL_Color* Dest, SDL_Color* Out) {
	(*Out).r = (((double)Dest->r) / ((double)Src->r)) * 255;
	(*Out).g = (((double)Dest->g) / ((double)Src->g)) * 255;
	(*Out).b = (((double)Dest->b) / ((double)Src->b)) * 255;
	(*Out).a = (((double)Dest->a) / ((double)Src->a)) * 255;
}
