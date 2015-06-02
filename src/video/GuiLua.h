#ifndef __GUILUA_H
#define __GUILUA_H

typedef struct lua_State lua_State;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;

#include "../sys/LinkedList.h"

/**
 * GuiLua uses a table on the global space called GUI.
 * Table has variable named Menu that contains the currently displaying menu.
 */

#define RestoreScreen(_State) lua_settop((_State), 0);					\
		lua_pushstring((_State), (const char*)g_GUIStack.Top->Data);	\
		LuaSetMenu_Aux((_State))

struct Widget;

extern struct LinkedList g_GUIMessageList;

struct GUIMessagePair {
	int(*Callback)(void*, void*);
	lua_State* State;
	const char* Key;
	void* One;
	void* Two;
};

int LuaRegisterWidget(lua_State* _State);
int LuaRegisterContainer(lua_State* _State);
int LuaRegisterLabel(lua_State* _State);
int LuaRegisterTable(lua_State* _State);
int LuaRegisterSurface(lua_State* _State);
int LuaRegisterFont(lua_State* _State);

int LuaCreateLabel(lua_State* _State);
int LuaCreateTable(lua_State* _State);
struct Container* LuaContainer(lua_State* _State);
int LuaHorizontalContainer(lua_State* _State);
int LuaVerticalContainer(lua_State* _State);
int LuaContextItem(lua_State* _State);

int LuaBackgroundColor(lua_State* _State);
int LuaGetFont(lua_State* _State);
int LuaDefaultFont(lua_State* _State);
int LuaGetDefaultFont(lua_State* _State);
/**
 * Lua function that takes one argument of type string.
 * Calls LuaSetMenu_Aux, then pushes the string onto g_GUIStack.
 */
int LuaSetMenu(lua_State* _State);
/**
 * Sets the initial state of a Menu, if the menu is not already loaded calls its Init function.
 * The value at position 1 on the Lua stack must be a string is the name of the menu to load.
 * If the Lua stack is greater than 1 the second value on the stack must be a table that will be passed
 * as the third argument to the menu's Init function.
 */
int LuaSetMenu_Aux(lua_State* _State);
void LuaSetColor(lua_State* _State, unsigned char* _RedPtr, unsigned char* _GreenPtr, unsigned char* _BluePtr);
int LuaSetFocusColor(lua_State* _State);
int LuaSetUnfocusColor(lua_State* _State);
/*
 * Returns the reference id needed for WidgetOnEvent.
 * Only adds the callback to the list of callbacks, does not add it
 * to the list of events iterated over when a widget event occurs.
 */
int LuaWidgetOnEvent(lua_State* _State, void(*_Callback)(struct Widget*));
int LuaOnKey(lua_State* _State);
//Please Comment.
int LuaCloseMenu(lua_State* _State);
int LuaPopMenu(lua_State* _State);
void LuaMenuThink(lua_State* _State);
int LuaScreenWidth(lua_State* _State);
int LuaScreenHeight(lua_State* _State);

int LuaSendMessage(lua_State* _State);
void GUIMessageCallback(lua_State* _State, const char* _Key, int(*_Callback)(void*, void*), void* _One, void* _Two);
void GUIMessageCheck(struct LinkedList* _List);

/**
 * Check functions
 */

struct Widget* LuaCheckWidget(lua_State* _State, int _Index);
struct Container* LuaCheckContainer(lua_State* _State, int _Index);
struct Label* LuaCheckLabel(lua_State* _State, int _Index);
SDL_Surface* LuaCheckSurface(lua_State* _State, int _Index);
struct Font* LuaCheckFont(lua_State* _State, int _Index);

/**
 * Widget functions
 */

int LuaWidgetId(lua_State* _State);
int LuaWidgetSetX(lua_State* _State);
int LuaWidgetGetX(lua_State* _State);
int LuaWidgetSetY(lua_State* _State);
int LuaWidgetGetY(lua_State* _State);
int LuaWidgetGetWidth(lua_State* _State);
int LuaWidgetGetHeight(lua_State* _State);
int LuaWidgetGetParent(lua_State* _State);
int LuaWidgetGetFocus(lua_State* _State);
int LuaWidgetSetFocus(lua_State* _State);
int LuaWidgetDestroy(lua_State* _State);

/**
 * Container functions
 */

int LuaContainerGetChild(lua_State* _State);
int LuaContainerSetChild(lua_State* _State);
int LuaContainerGetChildCt(lua_State* _State);
int LuaContainerGetChildren(lua_State* _State);
int LuaContainerGetSpacing(lua_State* _State);
int LuaContainerGetMargins(lua_State* _State);
int LuaContainerParagraph(lua_State* _State);
int LuaContainerHorizontalCenter(lua_State* _State);

/**
 * Label functions
 */

int LuaLabelSetText(lua_State* _State);

/**
 * Table functions
 */

int LuaTableGetCellIndex(lua_State* _State);
int LuaTableGetRows(lua_State* _State);
int LuaTableGetColumns(lua_State* _State);
int LuaTableSetCellWidth(lua_State* _State);
int LuaTableSetCellHeight(lua_State* _State);

/**
 * Font functions
 */

int LuaFontWidth(lua_State* _State);
int LuaFontHeight(lua_State* _State);

int InitGUILua(lua_State* _State);
int QuitGUILua(lua_State* _State);
struct Container* GetScreen(lua_State* _State);
int LuaKeyState(lua_State* _State, int _Index);
void LuaCallEvent(lua_State* _State, int _EvntIndx, struct Widget* _Callback);
/**
 * The table that contains the Widget must be on top of the stack.
 */
int LuaWidgetRef(lua_State* _State);
void LuaWidgetUnref(lua_State* _State, int _Ref);

#endif
