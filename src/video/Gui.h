/*
 * File: Gui.h
 * Author: David Brotz
 */
#ifndef __GUI_H
#define __GUI_H

#include "Video.h"

#include "../sys/HashTable.h"

#include <inttypes.h>
#include <stdbool.h>

/**
 * Every window created is placed onto g_GUIStack.
 * To get a Widget's Lua data call LuaGuiGetRef and then lua_rawgeti(State, -1, Widget->LuaRef);
 * Where -1 is the index of the results from LuaGuiGetRef and Widget is the Widget you want to retrieve. 
 *
 * Currently the Lua side of the code and the C side is intended to be seperate. This is why the Widget struct only reference to Lua is it's LuaRef, which is intended to have a #define around it if you dont want to use Lua.
 * To get the menu use g_GuiZBuff. The menu will always be the last element in g_GuiZBuff.
 *
 * NOTE: g_GuiZBuff seems to replace g_GUIStack. g_GUIStack doesn't seem to have a real purpose.
 */

//Seperate the Lua elements of the GUI with GUI_LUA to allow the GUI to be used without Lua.
#define GUI_LUA

#ifdef GUI_LUA
typedef struct lua_State lua_State;
#endif
union UWidgetOnKey;
typedef struct SDL_Color SDL_Color;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Rect SDL_Rect;
typedef int (*GuiCallWidget) (struct Widget*);
typedef struct Widget* (*GuiCallPoint) (struct Widget*, const SDL_Point*);
typedef void (*GuiCallDestroy) (struct Widget*, lua_State*);
typedef void(*GuiParentFunc) (struct Container*, struct Widget*);
typedef void (*GuiOnDebug)(const struct Widget*);
typedef void (*GuiOnDestroy)(struct Widget*);
typedef void(*GuiOnResize) (struct Widget*, int, int); 
typedef void(*GuiOnKey) (struct Widget*, const union UWidgetOnKey*); 

enum {
	WEVENT_KEY,
	WEVENT_TEXT
};

union UWidgetOnKey {
	uint8_t Type;
	struct {
		uint8_t Type;
		uint16_t Key;
		uint8_t Mod;
	} Key;
	struct {
		uint8_t Type;
		const char* Text;
	} Text;
};

struct GUIFocus {
	const struct Container* Parent;
	uint32_t Index;
	uint32_t Id;
	struct GUIFocus* Prev;
};

struct Margin {
	int32_t Top;
	int32_t Left;
	int32_t Right;
	int32_t Bottom;
};

struct GuiStyle {
	uint32_t Name;
	struct Font* Font;
	SDL_Color FontFocus;
	SDL_Color FontUnfocus;
	SDL_Color Background;
	SDL_Color BorderColor;
	struct Margin Margins;
	struct Margin Padding;
	uint16_t BorderWidth;
	uint16_t MaxWidth;
	uint16_t MaxHeight;
};

struct GuiSkin {
	const char* Name;
	struct GuiStyle* Default;
	struct GuiStyle* Label;
	struct GuiStyle* Button;
	struct GuiStyle* Table;
	struct GuiStyle* Container;
};

extern uint32_t g_GUIId;
extern struct HashTable g_GuiSkins;
extern struct GuiSkin* g_GuiSkinDefault;
extern struct GuiSkin* g_GuiSkinCurr;
extern struct GUIFocus* g_Focus;
extern struct GUIEvents* g_GUIEvents;
extern struct Stack g_GUIStack;

struct Area {
	int32_t w;
	int32_t h;
};

struct Widget {
	uint32_t Id;
	struct Container* Parent;
	SDL_Rect Rect;
	int (*OnDraw)(struct Widget*);
	struct Widget* (*OnClick)(struct Widget*, const SDL_Point*);
	struct Widget* (*OnFocus)(struct Widget*, const SDL_Point*);
	void (*SetPosition)(struct Widget*, const SDL_Point*);
	int (*OnUnfocus)(struct Widget*);
	GuiOnDebug OnDebug;
	GuiOnDestroy OnDestroy;
	GuiOnResize OnResize;
	struct Widget* (*OnDrag)(struct Widget*, const SDL_Point*);
	GuiOnKey OnKey;
	uint32_t LuaRef; //Unique id for this widget that is stored in the Lua registry.
	bool Clickable;
	bool IsVisible;
	bool CanFocus;
	bool IsDraggable;
	const struct GuiStyle* Style;
};

struct Container {
	struct Widget Widget;
	void (*NewChild)(struct Container*, struct Widget*);
	void (*RemChild)(struct Container*, struct Widget*);
	struct Widget** Children;
	uint16_t ChildrenSz;
	uint16_t ChildCt;
	const struct GuiSkin* Skin;
};

struct Container* CreateContainer(void);
struct GUIEvents* CreateGUIEvents(void);
struct GUIFocus* CreateGUIFocus(void);
struct Container* GUIZTop(void);
struct Container* GUIZBot(void);
void GuiClear();
void GuiEmpty();

void GuiSetLuaState(lua_State* State);
lua_State* GuiGetLuaState();

/**
 * Constructors
 */
void ConstructWidget(struct Widget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, uint32_t _ClassType);
void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect);
struct Font* CreateFont(const char* _Name, int _Size);

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child, SDL_Point* _Pos);
struct Widget* ContainerOnDrag(struct Container* _Widget, const struct SDL_Point* _Pos);

void WidgetSetParent(struct Container* _Parent, struct Widget* _Child);
void WidgetOnKey(struct Widget* _Widget, const union UWidgetOnKey* Event);
void WidgetSetVisibility(struct Widget* _Widget, int _Visibility);
struct Widget* WidgetOnDrag(struct Widget* _Widget, const struct SDL_Point* _Pos);
/**
 * Deconstructors
 */
void DestroyWidget(struct Widget* _Widget);
void DestroyContainer(struct Widget* Widget);
void DestroyFont(struct Font* _Font);
void DestroyGUIEvents(struct GUIEvents* _Events);
void DestroyFocus(struct GUIFocus* _Focus);

int ContainerOnDraw(struct Widget* Widget);
int MenuOnDraw(struct Container* _Container);
void ContainerSetPosition(struct Widget* Widget, const struct SDL_Point* _Point);
struct Widget* ContainerOnFocus(struct Widget* Widget, const SDL_Point* _Point);
int ContainerOnUnfocus(struct Widget* Widget);
int ContainerHorzFocChange(const struct Widget* Widget);
struct Widget* ContainerOnClick(struct Widget* Widget, const SDL_Point* _Point);
struct Widget* MenuOnClick(struct Widget* Widget, const SDL_Point* _Point);
void ContainerOnDebug(const struct Widget* Widget);

void VertConNewChild(struct Container* _Parent, struct Widget* _Child);
void HorzConNewChild(struct Container* _Parent, struct Widget* _Child);
void FixedConNewChild(struct Container* _Parent, struct Widget* _Child);
void ContextItemNewChild(struct Container* _Parent, struct Widget* _Child);
void ContainerShrink(struct Container* _Container);
/**
 * Returns 1 if the container can grow else returns 0.
 * _Width and _Height are how much the container needs to grow by.
 */
int WidgetGrow(struct Widget* _Widget, int _Width, int _Height);

/*
 * Remove the child from the parent's children leaving a gap in the array.
 */
void StaticRemChild(struct Container* _Parent, struct Widget* _Child);
/*
 * Remove the child and slide all children above to fill the hole in the array.
 */
void DynamicRemChild(struct Container* _Parent, struct Widget* _Child);

int WidgetOnDraw(struct Widget* Widget);
bool WidgetSetWidth(struct Widget* _Widget, int _Width);
bool WidgetSetHeight(struct Widget* _Widget, int _Height);
void WidgetSetPosition(struct Widget* _Widget, const SDL_Point* _Pos);
struct Widget* WidgetOnClick(struct Widget* _Widget, const SDL_Point* _Point);
void WidgetOnDebug(const struct Widget* _Widget);
int WidgetCheckVisibility(const struct Widget* _Widget);

//Should only be used once before ConstructWidget is called or the widget will become in an invalid state.
static inline void WidgetSetLuaClass(struct Widget* Widget, int Class) {
	Widget->LuaRef = Class;
}

int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget);
struct Widget* WidgetOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int WidgetOnUnfocus(struct Widget* _Widget);
struct Container* WidgetTopParent(struct Widget* _Widget);
//Sets the Z position of _Container to the top most container.
void GuiZToTop(struct Container* _Container);
void GuiDraw(void);
void GuiMenuThink(lua_State* State);
void GuiDrawDebug(void);
struct Widget* GuiGetBack(void);
/**
 * Generic function used to query the Z buffer to determine if a struct Widget's query function 
 * such as OnDrag, or OnFocus returns a struct Widget* or not.
 * _FuncOffset must be the offset to a function pointer that is defined in a struct Widget that
 * takes two arguments, a struct Widget* and a SDL_Point*.
 * _FuncOffset should be obtained by using offsetof.
 * _MousePos is contains the position of the mouse position.
 */
struct Widget* GuiFind(int _FuncOffset, const SDL_Point* _MousePos);
void GuiZBuffAdd(struct Container* _Container);
void GuiZBuffRem(struct Container* _Container);
/**
 * \brief Fills _Out with the values that must be passed to blend _Src into _Dest.
 */
void GetBlendValue(const SDL_Color* _Src, const SDL_Color* _Dest, SDL_Color* _Out);
static inline void GuiLeftOf(struct Widget* Rel, const struct Widget* Base) {
	SDL_Point Pos = {0, 0};

	Pos.x = Base->Rect.x - Base->Rect.w - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y - Rel->Parent->Widget.Rect.y;
	Pos.x += Rel->Style->Margins.Right;
	Pos.x += Base->Style->Margins.Left;
	Rel->SetPosition((struct Widget*) Rel, &Pos);
}

static inline void GuiRightOf(struct Widget* Rel, const struct Widget* Base) {
	SDL_Point Pos = {0, 0};

	Pos.x = Base->Rect.x + Base->Rect.w - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y - Rel->Parent->Widget.Rect.y;
	Pos.x += Base->Style->Margins.Right;
	Pos.x += Rel->Style->Margins.Left;
	Rel->SetPosition((struct Widget*) Rel, &Pos);
}

static inline void GuiAboveOf(struct Widget* Rel, const struct Widget* Base) {
	SDL_Point Pos = {0, 0};

	Pos.x = Base->Rect.x - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y - Rel->Rect.h - Rel->Parent->Widget.Rect.y;
	Pos.x += Rel->Style->Margins.Bottom;
	Pos.x += Base->Style->Margins.Top;
	Base->SetPosition((struct Widget*) Rel, &Pos);
}

static inline void GuiBelowOf(struct Widget* Rel, const struct Widget* Base) {
	SDL_Point Pos = {0, 0};

	Pos.x = Base->Rect.x - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y + Base->Rect.h - Rel->Parent->Widget.Rect.y;
	Pos.x += Rel->Style->Margins.Top;
	Pos.x += Base->Style->Margins.Bottom;
	Rel->SetPosition((struct Widget*)Rel, &Pos);
}

static inline void GuiAlignRight(struct Widget* Widget) {
	SDL_Point Pos = {0, 0};
	struct Container* Parent = Widget->Parent;

	if(Parent == NULL) return;
	Pos.x = Parent->Widget.Rect.w - Widget->Rect.w;
	Pos.y = Widget->Rect.y;
	Widget->SetPosition(Widget, &Pos);
}
#endif
