#ifndef __GUILUA_H
#define __GUILUA_H

typedef struct lua_State lua_State;
typedef struct _TTF_Font TTF_Font;
typedef struct SDL_Surface SDL_Surface;

#include "../sys/LinkedList.h"
#include "../sys/Rule.h"
#include "../sys/LuaCore.h"

#include <inttypes.h>
#include <stdbool.h>

#define GuiLoadMenu(State, File) ((bool)(LuaLoadFile((State), (File), "Menu") == LUA_OK))
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
	GUIL_ONHOVER = LUA_OSIZE,
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
void LuaGuiCallFunc(lua_State* State, struct Widget* Widget, uint32_t Func, uint32_t Args);

int LuaCreateLabel(lua_State* State);
int LuaCreateButton(lua_State* State);
int LuaCreateTable(lua_State* State);
int LuaCreateTextBox(lua_State* State);
struct Container* LuaContainer(lua_State* State);
int LuaHorizontalContainer(lua_State* State);
int LuaVerticalContainer(lua_State* State);
int LuaFixedContainer(lua_State* State);
int LuaMenuAsContainer(lua_State* State);
int LuaContextItem(lua_State* State);
//int LuaCreateWorldRender(lua_State* State);

int LuaCreateWindow(lua_State* State);

int LuaCreateImage(lua_State* State);
int LuaCreateStack(lua_State* State);
int LuaGuiStackAddTab(lua_State* State);

void GuiSetMenu(lua_State* State, const char* Menu);
/**
 * Lua function that takes one argument of type string.
 * Calls LuaSetMenu_Aux, then pushes the string onto g_GUIStack.
 * Closes the current menu and opens a new one.
 */
int LuaSetMenu(lua_State* State);
/**
 * Sets the initial state of a Menu, if the menu is not already loaded calls its Init function.
 * The value at position 1 on the Lua stack must be a string is the name of the menu to load.
 * If the Lua stack is greater than 1 the second value on the stack must be a table that will be passed
 * as the third argument to the menu's Init function.
 */
int LuaSetMenu_Aux(lua_State* State);
int LuaCloseMenu(lua_State* State);
void LuaSetColor(lua_State* State, unsigned char* RedPtr, unsigned char* GreenPtr, unsigned char* BluePtr);
int LuaGetFont(lua_State* State);
/*
 * Returns the reference id needed for WidgetOnEvent.
 * Only adds the callback to the list of callbacks, does not add it
 * to the list of events iterated over when a widget event occurs.
 */
int LuaWidgetOnEvent(lua_State* State, void(*Callback)(struct Widget*));
int LuaOnKey(lua_State* State);
int LuaWidgetOnClick(lua_State* State);
//Please Comment.
int LuaPopMenu(lua_State* State);
void LuaMenuThink(lua_State* State);
void GuiMenuOnThink(lua_State* State, struct Container* Container);
int LuaScreenWidth(lua_State* State);
int LuaScreenHeight(lua_State* State);

/**
 * Widget functions
 */

int LuaWidgetId(lua_State* State);
int LuaWidgetSetPos(lua_State* State);
int LuaWidgetSetX(lua_State* State);
int LuaWidgetGetX(lua_State* State);
int LuaWidgetSetY(lua_State* State);
int LuaWidgetGetY(lua_State* State);
int LuaWidgetGetWidth(lua_State* State);
int LuaWidgetSetWidth(lua_State* State);
int LuaWidgetGetHeight(lua_State* State);
int LuaWidgetSetHeight(lua_State* State);
int LuaWidgetGetParent(lua_State* State);
int LuaWidgetGetFocus(lua_State* State);
int LuaWidgetSetFocus(lua_State* State);
int LuaWidgetDestroy(lua_State* State);
int LuaWidgetOnHover(lua_State* State);
int LuaWidgetOnHoverLoss(lua_State* State);
int LuaWidgetIsVisible(lua_State* State);
int LuaWidgetHide(lua_State* State);
int LuaWidgetAlignRight(lua_State* State);
//int LuaWidgetSetStyle(lua_State* State);

int LuaContainerLeftOf(lua_State* State);
int LuaContainerRightOf(lua_State* State);
int LuaContainerAbove(lua_State* State);
int LuaContainerBelow(lua_State* State);
int LuaContainergetSkin(lua_State* State);


/**
 * Container functions
 */

int LuaContainerGetChild(lua_State* State);
int LuaContainerSetChild(lua_State* State);
int LuaContainerGetChildCt(lua_State* State);
int LuaContainerGetChildren(lua_State* State);
int LuaContainerDestroyChildren(lua_State* State);
int LuaContainerParagraph(lua_State* State);
int LuaContainerHorizontalCenter(lua_State* State);
int LuaContainerClear(lua_State* State);
int LuaContainerClose(lua_State* State);
int LuaContainerShrink(lua_State* State);
int LuaContainerAddChild(lua_State* State);
int LuaContainerSetSkin(lua_State* State);
int LuaContainerGetSkin(lua_State* State);
int LuaContainerOnNewChild(lua_State* State);
int LuaContainerCreateContainer(lua_State* State);

/**
 * Label functions
 */

int LuaLabelSetText(lua_State* State);

int LuaButtonClickable(lua_State* State);
int LuaButtonUnclickable(lua_State* State);

/**
 * Table functions
 */

int LuaTableGetCellIndex(lua_State* State);
int LuaTableGetRows(lua_State* State);
int LuaTableGetColumns(lua_State* State);
int LuaTableSetCellWidth(lua_State* State);
int LuaTableSetCellHeight(lua_State* State);
int LuaTableGetCellWidth(lua_State* State);
int LuaTableGetCellHeight(lua_State* State);

/**
 * Font functions
 */

int LuaFontWidth(lua_State* State);
int LuaFontHeight(lua_State* State);

int LuaTextBoxGetText(lua_State* State);

int InitGUILua(lua_State* State);
int QuitGUILua(lua_State* State);
int LuaKeyState(lua_State* State, int Index);
/**
 * Returns the Lua table that contains the references to all Gui elements.
 */
void LuaGuiGetRef(lua_State* State);
/**
 * The table that contains the Widget must be on top of the stack.
 */
int LuaWidgetRef(lua_State* State, struct Widget* Widget, uint32_t ClassType);
void LuaWidgetUnref(lua_State* State, struct Widget* Widget);
/**
 * Retrieves the Menu global from the environment Menu and sets it as a global in the main environment.
 */
void LuaAddMenu(lua_State* State, const char* Name);

void CreateMenu(const char* Menu);
void MessageBox(const char* Text);
void GuiSetParentHook(struct Container* Container);
struct Container* GuiGetParentHook(void);
int LuaGuiClose(lua_State* State);
int LuaGuiSkin(lua_State* State);
void LuaColorToSDL(lua_State* State, int Index, SDL_Color* Color);
struct GuiStyle* LuaGuiStyle(lua_State* State, int Index);
int LuaColor(lua_State* State);

int LuaGuiSetSkin(lua_State* State);
int LuaGuiGetSkin(lua_State* State);
int	LuaGuiStyleGetFont(lua_State* State);
int	LuaGuiStyleFontFocus(lua_State* State);
int	LuaGuiStyleFontUnfocus(lua_State* State);
int	LuaGuiStyleBackgroundColor(lua_State* State);
int	LuaGuiStyleMargins(lua_State* State);
int	LuaGuiSkinGetName(lua_State* State);
int	LuaGuiSkinLabel(lua_State* State);
int	LuaGuiSkinButton(lua_State* State);
int	LuaGuiSkinTable(lua_State* State);
int	LuaGuiSkinContainer(lua_State* State);

/**
 * GuiLua uses a table on the global space called GUI.
 * Table has variable named Menu that contains the currently displaying menu.
 */

bool RestoreScreen(lua_State* State);
/**
 * Sets the Lua reference 
 */
void LuaSetMenuRef(lua_State* State, int Index);
void LuaGetMenuRef(lua_State* State);
#endif
