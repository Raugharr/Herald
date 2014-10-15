#ifndef __GUILUA_H
#define __GUILUA_H

typedef struct lua_State lua_State;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;

int LuaRegisterWidget(lua_State* _State);
int LuaRegisterContainer(lua_State* _State);
int LuaRegisterTextBox(lua_State* _State);
int LuaRegisterSurface(lua_State* _State);
int LuaRegisterFont(lua_State* _State);

int LuaCreateTextBox(lua_State* _State);
struct Container* LuaContainer(lua_State* _State);
int LuaHorizontalContainer(lua_State* _State);
int LuaVerticalContainer(lua_State* _State);


int LuaBackgroundColor(lua_State* _State);
int LuaGetFont(lua_State* _State);
int LuaDefaultFont(lua_State* _State);
int LuaSetMenu(lua_State* _State);
void LuaSetColor(lua_State* _State, unsigned char* _RedPtr, unsigned char* _GreenPtr, unsigned char* _BluePtr);
int LuaSetFocusColor(lua_State* _State);
int LuaSetUnfocusColor(lua_State* _State);
int LuaOnKey(lua_State* _State);
int LuaCloseMenu(lua_State* _State);

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
int LuaWidgetGetChildren(lua_State* _State);

/**
 * Container functions
 */

int LuaContainerGetSpacing(lua_State* _State);
int LuaContainerGetMargins(lua_State* _State);

/**
 * TextBox functions
 */

int LuaTextBoxSetText(lua_State* _State);

int InitGUILua(lua_State* _State);
int QuitGUILua(lua_State* _State);
struct Container* GetScreen(lua_State* _State);
int LuaKeyState(lua_State* _State, int _Index);
void LuaCallEvent(lua_State* _State, int _EvntIndx);
int LuaWidgetRef(lua_State* _State);
void LuaWidgetUnref(lua_State* _State, int _Ref);

#endif
