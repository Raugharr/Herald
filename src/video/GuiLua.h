#ifndef __GUILUA_H
#define __GUILUA_H

typedef struct lua_State lua_State;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;

#include "../sys/LinkedList.h"
#include "../sys/Rule.h"
#include "../sys/LuaCore.h"

#include <inttypes.h>

/**
 * GuiLua uses a table on the global space called GUI.
 * Table has variable named Menu that contains the currently displaying menu.
 */

#define RestoreScreen(_State) lua_settop((_State), 0);								\
		if(g_GUIStack.Top != NULL) {												\
		lua_pushstring((_State), (const char*)g_GUIStack.Top[g_GUIStack.Size]);		\
		LuaSetMenu_Aux((_State));													\
		}
#define GuiLoadMenu(_State, _File) ((bool)(LuaLoadFile((_State), (_File), "Menu") == LUA_OK))

struct Widget;
typedef struct SDL_Color SDL_Color;
extern const struct LuaEnumReg g_LuaGuiEnums[];

enum {
	GUIL_CNVERT,
	GUIL_CNHORZ,
	GUIL_CNFIXED
};

//Table indexes used to store callbacks for each type of gui event.
enum {
	GUIL_ONHOVER = 1,
	GUIL_ONHOVERLOSS,
	GUIL_ONKEY,
	GUIL_ONCLICK
};

/*struct GUIMessagePacket {
	void* One;
	void* Two;
	struct Primitive RecvPrim;
	lua_State* State;
};

struct GUIMessagePair {
	GUIMessageFunc Callback;
	lua_State* State;
	const char* Key;
	void* One;
	void* Two;
};*/

//_Func should be a value from the GUIL_* enumeration.
void LuaGuiCallFunc(lua_State* _State, struct Widget* _Widget, uint32_t _Func);

int LuaCreateLabel(lua_State* _State);
int LuaCreateButton(lua_State* _State);
int LuaCreateTable(lua_State* _State);
int LuaCreateTextBox(lua_State* _State);
struct Container* LuaContainer(lua_State* _State);
int LuaHorizontalContainer(lua_State* _State);
int LuaVerticalContainer(lua_State* _State);
int LuaFixedContainer(lua_State* _State);
int LuaMenuAsContainer(lua_State* _State);
int LuaContextItem(lua_State* _State);
//int LuaCreateWorldRender(lua_State* _State);

int LuaCreateWindow(lua_State* _State);

int LuaCreateImage(lua_State* _State);

void GuiSetMenu(lua_State* _State, const char* _Menu);
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
int LuaCloseMenu(lua_State* _State);
void LuaSetColor(lua_State* _State, unsigned char* _RedPtr, unsigned char* _GreenPtr, unsigned char* _BluePtr);
int LuaGetFont(lua_State* _State);
/*
 * Returns the reference id needed for WidgetOnEvent.
 * Only adds the callback to the list of callbacks, does not add it
 * to the list of events iterated over when a widget event occurs.
 */
int LuaWidgetOnEvent(lua_State* _State, void(*_Callback)(struct Widget*));
int LuaOnKey(lua_State* _State);
int LuaWidgetOnClick(lua_State* _State);
//Please Comment.
int LuaPopMenu(lua_State* _State);
void LuaMenuThink(lua_State* _State);
int LuaScreenWidth(lua_State* _State);
int LuaScreenHeight(lua_State* _State);

/**
 * Widget functions
 */

int LuaWidgetId(lua_State* _State);
int LuaWidgetSetPos(lua_State* _State);
int LuaWidgetSetX(lua_State* _State);
int LuaWidgetGetX(lua_State* _State);
int LuaWidgetSetY(lua_State* _State);
int LuaWidgetGetY(lua_State* _State);
int LuaWidgetGetWidth(lua_State* _State);
int LuaWidgetSetWidth(lua_State* _State);
int LuaWidgetGetHeight(lua_State* _State);
int LuaWidgetSetHeight(lua_State* _State);
int LuaWidgetGetParent(lua_State* _State);
int LuaWidgetGetFocus(lua_State* _State);
int LuaWidgetSetFocus(lua_State* _State);
int LuaWidgetDestroy(lua_State* _State);
int LuaWidgetOnHover(lua_State* _State);
int LuaWidgetOnHoverLoss(lua_State* _State);
int LuaWidgetSetStyle(lua_State* _State);
int LuaContainerLeftOf(lua_State* _State);
int LuaContainerRightOf(lua_State* _State);
int LuaContainerAbove(lua_State* _State);
int LuaContainerBelow(lua_State* _State);
int LuaContainergetSkin(lua_State* _State);

/**
 * Container functions
 */

int LuaContainerGetChild(lua_State* _State);
int LuaContainerSetChild(lua_State* _State);
int LuaContainerGetChildCt(lua_State* _State);
int LuaContainerGetChildren(lua_State* _State);
int LuaContainerDestroyChildren(lua_State* State);
int LuaContainerParagraph(lua_State* _State);
int LuaContainerHorizontalCenter(lua_State* _State);
int LuaContainerClear(lua_State* _State);
int LuaContainerClose(lua_State* _State);
int LuaContainerShrink(lua_State* _State);
int LuaContainerAddChild(lua_State* _State);
int LuaContainerSetSkin(lua_State* _State);
int LuaContainerGetSkin(lua_State* _State);
int LuaContainerOnNewChild(lua_State* _State);
int LuaContainerCreateContainer(lua_State* State);

/**
 * Label functions
 */

int LuaLabelSetText(lua_State* _State);

int LuaButtonClickable(lua_State* _State);
int LuaButtonUnclickable(lua_State* _State);

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
int LuaKeyState(lua_State* _State, int _Index);
void LuaGuiGetRef(lua_State* _State);
/**
 * The table that contains the Widget must be on top of the stack.
 */
int LuaWidgetRef(lua_State* _State);
void LuaWidgetUnref(lua_State* _State, struct Widget* _Widget);
/**
 * Retrieves the Menu global from the environment Menu and sets it as a global in the main environment.
 */
void LuaAddMenu(lua_State* _State, const char* _Name);

void MessageBox(const char* _Text);
void GuiSetParentHook(struct Container* _Container);
struct Container* GuiGetParentHook(void);
int LuaGuiClose(lua_State* _State);
int LuaGuiSkin(lua_State* _State);
void LuaColorToSDL(lua_State* _State, int _Index, SDL_Color* _Color);
struct GuiStyle* LuaGuiStyle(lua_State* _State, int _Index);
int LuaColor(lua_State* _State);

int LuaGuiSetSkin(lua_State* _State);
int LuaGuiGetSkin(lua_State* _State);
int	LuaGuiStyleGetFont(lua_State* _State);
int	LuaGuiStyleFontFocus(lua_State* _State);
int	LuaGuiStyleFontUnfocus(lua_State* _State);
int	LuaGuiStyleBackgroundColor(lua_State* _State);
int	LuaGuiStyleMargins(lua_State* _State);
int	LuaGuiSkinGetName(lua_State* _State);
int	LuaGuiSkinLabel(lua_State* _State);
int	LuaGuiSkinButton(lua_State* _State);
int	LuaGuiSkinTable(lua_State* _State);
int	LuaGuiSkinContainer(lua_State* _State);
#endif
