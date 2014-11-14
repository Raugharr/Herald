#ifndef __GUILUA_H
#define __GUILUA_H

typedef struct lua_State lua_State;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;

int LuaRegisterWidget(lua_State* _State);
int LuaRegisterContainer(lua_State* _State);
int LuaRegisterTextBox(lua_State* _State);
int LuaRegisterTable(lua_State* _State);
int LuaRegisterSurface(lua_State* _State);
int LuaRegisterFont(lua_State* _State);

int LuaCreateTextBox(lua_State* _State);
int LuaCreateTable(lua_State* _State);
struct Container* LuaContainer(lua_State* _State);
int LuaHorizontalContainer(lua_State* _State);
int LuaVerticalContainer(lua_State* _State);

int LuaBackgroundColor(lua_State* _State);
int LuaGetFont(lua_State* _State);
int LuaDefaultFont(lua_State* _State);
int LuaSetMenu(lua_State* _State);
int LuaSetMenu_Aux(lua_State* _State);
void LuaSetColor(lua_State* _State, unsigned char* _RedPtr, unsigned char* _GreenPtr, unsigned char* _BluePtr);
int LuaSetFocusColor(lua_State* _State);
int LuaSetUnfocusColor(lua_State* _State);
int LuaOnKey(lua_State* _State);
int LuaCloseMenu(lua_State* _State);
int LuaPopMenu(lua_State* _State);
int LuaScreenWidth(lua_State* _State);
int LuaScreenHeight(lua_State* _State);

/**
 * Check functions
 */

struct Widget* LuaCheckWidget(lua_State* _State, int _Index);
struct Container* LuaCheckContainer(lua_State* _State, int _Index);
struct TextBox* LuaCheckTextBox(lua_State* _State, int _Index);
SDL_Surface* LuaCheckSurface(lua_State* _State, int _Index);
struct Font* LuaCheckFont(lua_State* _State, int _Index);

/**
 * Widget functions
 */

int LuaWidgetId(lua_State* _State);
int LuaWidgetGetX(lua_State* _State);
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

/**
 * TextBox functions
 */

int LuaTextBoxSetText(lua_State* _State);

/**
 * Table functions
 */

int LuaTableGetCellIndex(lua_State* _State);
int LuaTableGetFont(lua_State* _State);
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
void LuaCallEvent(lua_State* _State, int _EvntIndx);
/**
 * The table that contains the Widget must be on top of the stack.
 */
int LuaWidgetRef(lua_State* _State);
void LuaWidgetUnref(lua_State* _State, int _Ref);

#endif
