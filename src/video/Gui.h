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

typedef struct SDL_Color SDL_Color;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Rect SDL_Rect;
typedef struct lua_State lua_State;
typedef int (*GuiCallWidget) (struct Widget*);
typedef struct Widget* (*GuiCallPoint) (struct Widget*, const SDL_Point*);
typedef void (*GuiCallDestroy) (struct Widget*, lua_State*);
typedef void(*GuiParentFunc) (struct Container*, struct Widget*);
typedef void(*GuiOnKey) (struct Widget*, unsigned int, unsigned int); 

/*
 * TODO: Remove LuaOnClickFunc and and put function into the Lua table and use WEvents to handle click events.
 */
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
	struct Margin Margins;
};

struct GuiSkin {
	const char* Name;
	struct GuiStyle* Label;
	struct GuiStyle* Button;
	struct GuiStyle* Table;
	struct GuiStyle* Container;
};

extern uint32_t g_GUIId;
extern int g_GUIMenuChange;
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
	void (*OnKeyUp)(struct Widget*, SDL_KeyboardEvent*);
	void (*SetPosition)(struct Widget*, const SDL_Point*);
	int (*OnUnfocus)(struct Widget*);
	void (*OnDebug)(const struct Widget*);
	void (*OnDestroy)(struct Widget*, lua_State*);
	struct Widget* (*OnDrag)(struct Widget*, const struct SDL_Point*);
	GuiOnKey OnKey;
	uint32_t LuaRef;
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
	int(*HorzFocChange)(const struct Container*);
	int VertFocChange;
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
void GuiClear(lua_State* _State);
void GuiEmpty();

/**
 * Constructors
 */
void ConstructWidget(struct Widget* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State);
void ConstructContainer(struct Container* _Widget, struct Container* _Parent, SDL_Rect* _Rect, lua_State* _State);
struct Font* CreateFont(const char* _Name, int _Size);

void ContainerPosChild(struct Container* _Parent, struct Widget* _Child, SDL_Point* _Pos);
struct Widget* ContainerOnDrag(struct Container* _Widget, const struct SDL_Point* _Pos);

void WidgetSetParent(struct Container* _Parent, struct Widget* _Child);
void WidgetOnKeyUp(struct Widget* _Widget, SDL_KeyboardEvent* _Event);
void WidgetOnKey(struct Widget* _Widget, unsigned int _Key, unsigned int _KeyMod);
void WidgetSetVisibility(struct Widget* _Widget, int _Visibility);
struct Widget* WidgetOnDrag(struct Widget* _Widget, const struct SDL_Point* _Pos);
/**
 * Deconstructors
 */
void DestroyWidget(struct Widget* _Widget, lua_State* _State);
void DestroyContainer(struct Container* _Container, lua_State* _State);
void DestroyFont(struct Font* _Font);
void DestroyGUIEvents(struct GUIEvents* _Events);
void DestroyFocus(struct GUIFocus* _Focus);

int ContainerOnDraw(struct Container* _Container);
int MenuOnDraw(struct Container* _Container);
void ContainerSetPosition(struct Container* _Container, const struct SDL_Point* _Point);
struct Widget* ContainerOnFocus(struct Container* _Container, const SDL_Point* _Point);
int ContainerOnUnfocus(struct Container* _Container);
int ContainerHorzFocChange(const struct Container* _Container);
struct Widget* ContainerOnClick(struct Container* _Container, const SDL_Point* _Point);
struct Widget* MenuOnClick(struct Container* _Container, const SDL_Point* _Point);
void ContainerOnDebug(const struct Container* _Container);

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

bool WidgetSetWidth(struct Widget* _Widget, int _Width);
bool WidgetSetHeight(struct Widget* _Widget, int _Height);
void WidgetSetPosition(struct Widget* _Widget, const SDL_Point* _Pos);
struct Widget* WidgetOnClick(struct Widget* _Widget, const SDL_Point* _Point);
void WidgetOnDebug(const struct Widget* _Widget);
int WidgetCheckVisibility(const struct Widget* _Widget);

int GetHorizontalCenter(const struct Container* _Parent, const struct Widget* _Widget);
struct Widget* WidgetOnFocus(struct Widget* _Widget, const SDL_Point* _Point);
int WidgetOnUnfocus(struct Widget* _Widget);
struct Container* WidgetTopParent(struct Widget* _Widget);
//Sets the Z position of _Container to the top most container.
void GuiZToTop(struct Container* _Container);
void GuiDraw(void);
void GuiDrawDebug(void);
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
#endif
