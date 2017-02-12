/*
 * File: GuiLua.c
 * Author: David Brotz
 */

#include "GuiLua.h"

#include "Gui.h"
#include "GuiAux.h"
#include "Video.h"
#include "ImageWidget.h"
#include "Sprite.h"
#include "../sys/LuaCore.h"
#include "../sys/Array.h"
#include "../sys/Log.h"
#include "../sys/LinkedList.h"
#include "../sys/TaskPool.h"
#include "../sys/Event.h"
#include "../Herald.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_ttf.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <sys/io.h>
	#include <unistd.h>
#endif

#define LUA_EVENTIDSIZE (16)

struct Container* g_GuiParentHook = NULL;

static const luaL_Reg g_LuaFuncsGUI[] = {
		{"HorizontalContainer", LuaHorizontalContainer},
		{"VerticalContainer", LuaVerticalContainer},
		{"FixedContainer", LuaFixedContainer},
		{"CreateTable", LuaCreateTable},
		{"MenuAsContainer", LuaMenuAsContainer},
		{"CreateContextItem", LuaContextItem},
		{"SetMenu", LuaSetMenu},
		//{"CloseMenu", LuaCloseMenu},
		{"PopMenu", LuaPopMenu},
		{"ScreenWidth", LuaScreenWidth},
		{"ScreenHeight", LuaScreenHeight},
		{"CreateWindow", LuaCreateWindow},
		{"Close", LuaGuiClose},
		{"LoadSkin", LuaGuiSkin},
		{"Font", LuaGetFont},
		{"Color", LuaColor},
		{"SetSkin", LuaGuiSetSkin},
		{"GetSkin", LuaGuiGetSkin},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsWidget[] = {
		{"Id", LuaWidgetId},
		{"SetPos", LuaWidgetSetPos},
		{"SetX", LuaWidgetSetX},
		{"GetX", LuaWidgetGetX},
		{"SetY", LuaWidgetSetY},
		{"GetY", LuaWidgetGetY},
		{"GetWidth", LuaWidgetGetWidth},
		{"SetWidth", LuaWidgetSetWidth},
		{"GetHeight", LuaWidgetGetHeight},
		{"SetHeight", LuaWidgetSetHeight},
		{"GetParent", LuaWidgetGetParent},
		{"GetFocus", LuaWidgetGetFocus},
		{"SetFocus", LuaWidgetSetFocus},
		{"OnKey", LuaOnKey},
		{"OnClick", LuaWidgetOnClick},
		{"Above", LuaContainerAbove},
		{"LeftOf", LuaContainerLeftOf},
		{"RightOf", LuaContainerRightOf},
		{"Below", LuaContainerBelow},
		{"Destroy", LuaWidgetDestroy},
		{"OnHover", LuaWidgetOnHover},
		{"OnHoverLoss", LuaWidgetOnHoverLoss},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsContainer[] = {
		{"SetChild", LuaContainerSetChild},
		{"GetChildCt", LuaContainerGetChildCt},
		{"CreateLabel", LuaCreateLabel},
		{"CreateTable", LuaCreateTable},
		{"CreateTextBox", LuaCreateTextBox},
		{"CreateButton", LuaCreateButton},
		{"CreateImage", LuaCreateImage},
		{"Children", LuaContainerGetChildren},
		{"DestroyChildren", LuaContainerDestroyChildren},
		{"Paragraph", LuaContainerParagraph},
		{"GetHorizontalCenter", LuaContainerHorizontalCenter},
		{"Close", LuaContainerClose},
		{"Clear", LuaContainerClear},
		{"Shrink", LuaContainerShrink},
		{"AddChild", LuaContainerAddChild},
		{"SetSkin", LuaContainerSetSkin},
		{"GetSkin", LuaContainerGetSkin},
		{"OnNewChild", LuaContainerOnNewChild},
		{"CreateContainer", LuaContainerCreateContainer},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsLabel[] = {
		{"SetText", LuaLabelSetText},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsButton[] = {
		{"Clickable", LuaButtonClickable},
		{"Unclickable", LuaButtonUnclickable},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsTable[] = {
		{"GetCellIndex", LuaTableGetCellIndex},
		{"GetRows", LuaTableGetRows},
		{"GetColumns", LuaTableGetColumns},
		{"SetCellWidth", LuaTableSetCellWidth},
		{"SetCellHeight", LuaTableSetCellHeight},
		{"GetCellWidth", LuaTableGetCellWidth},
		{"GetCellHeight", LuaTableGetCellHeight},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsFont[] = {
		{"Width", LuaFontWidth},
		{"Height", LuaFontHeight},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsImageWidget[] = {
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsTextBox[] = {
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsGuiStyle[] = {
	{"GetFont", LuaGuiStyleGetFont},
	{"FontFocus", LuaGuiStyleFontFocus},
	{"FontUnfocus", LuaGuiStyleFontUnfocus},
	{"BackgroundColor", LuaGuiStyleBackgroundColor},
	{"Margins", LuaGuiStyleMargins},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsGuiSkin[] = {
	{"GetName", LuaGuiSkinGetName},
	{"Label", LuaGuiSkinLabel},
	{"Button", LuaGuiSkinButton},
	{"Table", LuaGuiSkinTable},
	{"Container", LuaGuiSkinContainer},
	{NULL, NULL}
};

static const struct LuaObjectReg g_GuiLuaObjects[] = {
	{LOBJ_WIDGET, "Widget", LUA_REFNIL, g_LuaFuncsWidget},
	{LOBJ_CONTAINER, "Container", LOBJ_WIDGET, g_LuaFuncsContainer},
	{LOBJ_LABEL, "Label", LOBJ_WIDGET, g_LuaFuncsLabel},
	{LOBJ_TABLE, "Table", LOBJ_CONTAINER, g_LuaFuncsTable},
	{LOBJ_SURFACE, "Surface", LOBJ_WIDGET, NULL},
	{LOBJ_FONT, "Font", LUA_REFNIL, g_LuaFuncsFont},
	{LOBJ_BUTTON, "Button", LOBJ_LABEL, g_LuaFuncsButton},
	{LOBJ_IMAGEWIDGET, "ImageWidget", LOBJ_WIDGET, g_LuaFuncsImageWidget},
	{LOBJ_TEXTBOX, "TextBox", LOBJ_WIDGET, g_LuaFuncsTextBox},
	{LOBJ_GSTYLE, "GStyle", LUA_REFNIL, g_LuaFuncsGuiStyle},
	{LOBJ_GSKIN, "GSkin", LUA_REFNIL, g_LuaFuncsGuiSkin},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

const struct LuaEnum g_LuaContainerEnum[] = {
	{"Vertical", GUIL_CNVERT},
	{"Horizontal", GUIL_CNHORZ},
	{"Fixed", GUIL_CNFIXED},
	{NULL, 0}
};

const struct LuaEnumReg g_LuaGuiEnums[] = {
	{"Container", NULL,  g_LuaContainerEnum},
	{NULL, NULL}
};

//FIXME: Font should become a Lua class.
TTF_Font* LuaCreateFont(lua_State* State, const char* Name, int Size) {
	TTF_Font* Font = NULL;

	chdir("Fonts");
	Font = lua_newuserdata(State, sizeof(TTF_Font*));
	Font = TTF_OpenFont(Name, Size);
	chdir("..");
	return Font;
	/*luaL_getmetatable(State, LOBJ_FONT);
	lua_setmetatable(State, -2);

	lua_pushstring(State, "Name");
	lua_pushstring(State, Name);
	lua_rawset(State, -3);
	lua_pushstring(State, "Size");
	lua_pushinteger(State, Size);
	lua_rawset(State, -3);
	chdir("..");
	return Font;*/
}

void LuaGuiCallFunc(lua_State* State, struct Widget* Widget, uint32_t Func) {
	LuaGuiGetRef(State);
	lua_rawgeti(State, -1, Widget->LuaRef);
	lua_rawgeti(State, -1, Func);
	if(lua_type(State, -1) == LUA_TFUNCTION) {
		lua_pushvalue(State, -2);
		LuaCallFunc(State, 1, 0, 0);
	} else {
		lua_pop(State, 1);
	}
	lua_pop(State, 1);
}

int LuaCreateLabel(lua_State* State) {
	struct Label* Label = NULL;
	struct Container* Parent = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	const char* Text = luaL_checkstring(State, 2);
	SDL_Rect Rect;
	SDL_Surface* Surface = NULL;
	struct Font* Font = Parent->Skin->Label->Font;
	SDL_Color White = {255, 255, 255};

	if((Surface = ConvertSurface(TTF_RenderText_Solid(Font->Font, Text, White))) == NULL) {
		Rect.w = 32;
		Rect.h = 32;
	} else {
		Rect.w = Surface->w;
		Rect.h = Surface->h;
	}
	Rect.x = Parent->Widget.Rect.x;
	Rect.y = Parent->Widget.Rect.y;
	lua_newtable(State);
	Label = CreateLabel();
	ConstructLabel(Label, Parent, &Rect, State, SurfaceToTexture(Surface), Parent->Skin->Label);
	LuaInitClass(State, Label, LOBJ_LABEL);
	return 1;
}

int LuaCreateButton(lua_State* State) {
	struct Container* Parent = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	const char* Text = luaL_checkstring(State, 2);
	SDL_Color Color = {255, 255, 255, 255};
	luaL_argcheck(State, 3, lua_isfunction(State, 3), "Third argument for CreateButton is not a functon");

	struct Font* Font = Parent->Skin->Button->Font;
	SDL_Surface* Surface = ConvertSurface(TTF_RenderText_Solid(Font->Font, Text, Color));
	SDL_Rect Rect = {Parent->Widget.Rect.x, Parent->Widget.Rect.y, Surface->w * 1.25f, Surface->h * 1.25f};
	struct Button* Button = NULL;

	lua_createtable(State, 2, 2);
	Button = ConstructButton(CreateButton(), Parent, &Rect, State, SurfaceToTexture(Surface), Font);
	LuaInitClass(State, Button, LOBJ_BUTTON);
	lua_pushvalue(State, 3);
	lua_rawseti(State, -2, GUIL_ONCLICK);
	return 1;
}

int LuaCreateTable(lua_State* State) {
	//int i = 0;
	struct Container* Parent = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	int Rows = luaL_checkinteger(State, 2);
	int Columns = luaL_checkinteger(State, 3);
	struct Table* Table = NULL;
	struct Font* Font = Parent->Skin->Table->Font;
	SDL_Rect Rect = {0, 0, 0, 0};

	if(lua_gettop(State) == 4)
		Font = LuaCheckClass(State, 1, LOBJ_FONT);
	Table = CreateTable();
	lua_newtable(State);
	if(Font == NULL)
		luaL_error(State, "No default font or font passed as argument.");
	lua_createtable(State, 2, 2);
	ConstructTable(Table, Parent, &Rect, State, Columns, Rows, Font);
	LuaInitClass(State, Table, LOBJ_TABLE);
	return 1;
}

int LuaCreateTextBox(lua_State* State) {
	struct Container* Parent = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	struct TextBox* TextBox = NULL;
	struct Font* Font = Parent->Skin->Label->Font;

	if(Parent == NULL)
		LuaClassError(State, 1, LOBJ_CONTAINER);
	TextBox = CreateTextBox();
	lua_createtable(State, 2, 2);
	ConstructTextBox(TextBox, Parent, 1, 16, State, Font, Parent->Skin->Label);
	LuaInitClass(State, TextBox, LOBJ_TEXTBOX);
	return 1;
}

struct Container* LuaContainer(lua_State* State) {
	enum {
		LARG_PARENT = 1,
		LARG_WIDTH,
		LARG_HEIGHT,
	};
	struct Container* Container = NULL;
	struct Container* Parent = NULL;
	SDL_Rect Rect = {0, 0, 0, 0};

	Rect.x = 0;
	Rect.y = 0;
	Rect.w = luaL_checkinteger(State, LARG_WIDTH);
	Rect.h = luaL_checkinteger(State, LARG_HEIGHT);
	if(lua_type(State, LARG_PARENT) == LUA_TTABLE)
		Parent = LuaCheckClass(State, LARG_PARENT, LOBJ_CONTAINER);
	Container = CreateContainer();
	lua_createtable(State, 2, 2);
	ConstructContainer(Container, Parent, &Rect, State);
	LuaInitClass(State, Container, LOBJ_CONTAINER);
	return Container;
}

int LuaHorizontalContainer(lua_State* State) {
	struct Container* Container = LuaContainer(State);

	Container->NewChild = HorzConNewChild;
	return 1;
}

int LuaVerticalContainer(lua_State* State) {
	struct Container* Container = LuaContainer(State);

	Container->NewChild = VertConNewChild;
	return 1;
}

int LuaFixedContainer(lua_State* State) {
	struct Container* Container = LuaContainer(State);

	Container->NewChild = FixedConNewChild;
	return 1;
}

int LuaContextItem(lua_State* State) {
	struct ContextItem* Container = NULL;
	SDL_Rect Rect = {luaL_checkint(State, 1), luaL_checkint(State, 2), luaL_checkint(State, 3), luaL_checkint(State, 4)};
	struct Container* Parent = LuaCheckClass(State, 7, LOBJ_CONTAINER);
	int i = 0;

	luaL_checktype(State, 6, LUA_TTABLE);
	lua_pushnil(State);
	while(lua_next(State, 6) != 0 && i <= 4) {
		lua_pop(State, 1);
		++i;
	}
	lua_newtable(State);
	Container = CreateContextItem();
	ConstructContextItem(Container, Parent, &Rect, State);
	LuaInitClass(State, Container, LOBJ_CONTAINER);
	return 1;
}

/*
int LuaCreateWorldRender(lua_State* State) {
	struct GameWorldWidget* Widget = NULL;
	struct Container* Parent = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	 
	lua_newtable(State);
	Widget = CreateGWWidget();
	ConstructGWWidget(Widget, Parent, &Parent->Widget.Rect, State, &g_GameWorld);
	LuaCtor(State, Widget, LOBJ_WIDGET);
	return 1;	
}
*/

int LuaGetFont(lua_State* State) {
	const char* Name = luaL_checkstring(State, 1);
	int Size = luaL_checkint(State, 2);
	struct Font* Font = NULL;

	chdir("fonts");
	Font = CreateFont(Name, Size);
	LuaCtor(State, Font, LOBJ_FONT);
	chdir("..");
	return 1;
}

int LuaCreateWindow(lua_State* State) {
	enum {
		LARG_NAME = 1,
		LARG_DATA,
		LARG_TABLE,
	};
	const char* Name = luaL_checkstring(State, LARG_NAME);
	struct Container* Container = NULL;
	SDL_Rect Rect = {0};

	if(lua_gettop(State) == 1)	
		lua_pushnil(State);	
	else						
		luaL_checktype(State, LARG_DATA, LUA_TTABLE);
	lua_getglobal(State, Name);
	if(lua_type(State, -1) == LUA_TNIL)
		luaL_error(State, "Menu %s not not exist", Name);

	lua_pushstring(State, "Width");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TNUMBER)
		return luaL_error(State, "%s's Width parameter is not a number.", Name);
	Rect.w = lua_tointeger(State, -1);
	lua_pop(State, 1);

	lua_pushstring(State, "Height");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TNUMBER)
		return luaL_error(State, "%s's Height parameter is not a number.", Name);
	Rect.h = lua_tointeger(State, -1);
	lua_pop(State, 1);

	lua_newtable(State);
	LuaCopyTable(State, -2);
	Container = CreateContainer();
	ConstructContainer(Container, NULL, &Rect, State);
	Container->NewChild = FixedConNewChild;
	Container->Widget.Parent = NULL;

	LuaInitClass(State, Container, LOBJ_CONTAINER);
	
	lua_pushstring(State, "Init");
	lua_rawget(State, LARG_TABLE);

	lua_pushvalue(State, LARG_TABLE);
	lua_pushvalue(State, LARG_DATA);
	GuiSetParentHook(Container);
	if(LuaCallFunc(State, 2, 0, 0) == 0)
		return luaL_error(State, "%s.Init function call failed", Name);
	GuiSetParentHook(NULL);
	//WidgetSetParent(NULL, (struct Widget*) Container);
	Container->Widget.IsDraggable = 1;
	GuiZBuffAdd(Container);
	return 1;
}

int LuaCreateImage(lua_State* State) {
	struct Container* Parent = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	struct Sprite* Sprite = LuaCheckClass(State, 2, LOBJ_SPRITE);
	SDL_Rect Rect = {Parent->Widget.Rect.x, Parent->Widget.Rect.y, 0, 0};
	struct ImageWidget* Widget = CreateImageWidget();

	if(Sprite == NULL) {
		lua_pushnil(State);
		return 1;
	}
	Rect.w = Sprite->Rect.w;
	Rect.h = Sprite->Rect.h;
	ConstructImageWidget(Widget, Parent, &Rect, State, Sprite, Parent->Skin->Label);
	Sprite->SpritePos.x = Widget->Widget.Rect.x;
	Sprite->SpritePos.y = Widget->Widget.Rect.y;
	Sprite->SpritePos.w = Widget->Widget.Rect.w;
	Sprite->SpritePos.h = Widget->Widget.Rect.h;
	LuaCtor(State, Widget, LOBJ_IMAGEWIDGET);
	return 1;
}

int LuaMenuAsContainer(lua_State* State) {
	struct Container* Container = CreateContainer();
	struct SDL_Rect Rect = {0, 0, 0, 0};

	lua_newtable(State);
	ConstructContainer(Container, NULL, &Rect, State);
	GuiSetParentHook(Container);
	LuaCreateWindow(State);
	//LuaSetMenu_Aux(State);
	GuiSetParentHook(NULL);
	return 1;
}

void GuiSetMenu(lua_State* State, const char* Menu) {
	char* NameCopy = NULL;

	lua_settop(State, 0);
	lua_pushstring(State, Menu);
	LuaSetMenu_Aux(State);
	NameCopy = calloc(strlen(Menu) + 1, sizeof(char));
	strcpy(NameCopy, Menu);
	StackPush(&g_GUIStack, NameCopy);
}

int LuaSetMenu(lua_State* State) {
	const char* Name = luaL_checkstring(State, 1);
	char* NameCopy = NULL;
	int Ret = 0;

	Ret = LuaSetMenu_Aux(State);
	NameCopy = calloc(strlen(Name) + 1, sizeof(char));
	strcpy(NameCopy, Name);
	StackPush(&g_GUIStack, NameCopy);
	return Ret;
}

int LuaSetMenu_Aux(lua_State* State) {
	enum SETMENU_LUAARGS {
		SETMENU_NAME = 1,
		SETMENU_DATA,
		SETMENU_WIDTH,
		SETMENU_HEIGHT,
		SETMENU_TABLE
	};
	const char* Name = luaL_checkstring(State, 1);
	SDL_Rect MenuRect = {0};
	struct Container* MenuContainer = NULL;
	int Width = SDL_WIDTH;
	int Height = SDL_HEIGHT;
	//struct Font* Font = NULL;
	//struct Font* Prev = NULL;

	switch(lua_gettop(State)) {
		case 4:
			Height = luaL_checkinteger(State, SETMENU_HEIGHT);
			Width = luaL_checkinteger(State, SETMENU_WIDTH);
			break;
		case 3:
			Width = luaL_checkinteger(State, SETMENU_WIDTH);
			lua_pushnil(State);
			break;
		case 1: 
			lua_pushnil(State);
		case 2:
			lua_pushnil(State);
			lua_pushnil(State);		
			break;
		default:
			SDL_assert(0 == 1 && "Incorrect amount of args.");
	}
	if(g_GUIStack.Size > 0) {
		GuiEmpty();
		LuaCloseMenu(State);
	}
	//Check if the global Name exists if it doesn't close the menu.
	lua_getglobal(State, Name);
	if(lua_type(State, -1) == LUA_TNIL)
		luaL_error(State, "Menu %s not not exist", Name);

	lua_newtable(State);
	LuaCopyTable(State, -2); //Copy Menu prototype into a new table.
	//This should be in its own function that also StackPush to g_GUIStack.
	lua_getglobal(State, "Gui");
	lua_pushstring(State, "Menu");
	lua_pushvalue(State, SETMENU_TABLE); //Get the new table.
	MenuRect.w = Width;
	MenuRect.h = Height;
	lua_newtable(State);
	MenuContainer = CreateContainer();
	ConstructContainer(MenuContainer, NULL, &MenuRect, State);
	MenuContainer->Widget.OnDraw = (int(*)(struct Widget*))MenuOnDraw;
	MenuContainer->Widget.OnClick = (struct Widget*(*)(struct Widget*, const SDL_Point*))MenuOnClick;
	MenuContainer->NewChild = FixedConNewChild;
	lua_pushvalue(State, SETMENU_TABLE);
	LuaInitClass(State, MenuContainer, LOBJ_CONTAINER);
	lua_pop(State, 2);

	lua_rawset(State, -3);//Set GUI.Menu equal to the global Name.
	lua_pop(State, 1);// Pop global GUI.

	lua_pushstring(State, "__name");
	lua_pushstring(State, Name);
	lua_rawset(State, SETMENU_TABLE);

	g_Focus = CreateGUIFocus();
	if(lua_type(State, -1) != LUA_TTABLE) {
		RestoreScreen(State);
		return luaL_error(State, "%s is not a table.", Name);
	}
	lua_pushstring(State, "Init");
	lua_rawget(State, SETMENU_TABLE);
	if(lua_type(State, -1) != LUA_TFUNCTION || lua_iscfunction(State, -1) != 0) {
		if(g_GUIStack.Top != NULL) {
			RestoreScreen(State);
		}
		return luaL_error(State, "Init is not a function in menu %s.", Name);
	}
	lua_pushvalue(State, SETMENU_TABLE);
	lua_pushvalue(State, SETMENU_DATA);
	if(LuaCallFunc(State, 2, 0, 0) == 0) {
		if(g_GUIStack.Size > 0) {
			RestoreScreen(State);
		}
		return luaL_error(State, "%s.Init function call failed", Name);
	}
	lua_getglobal(State, Name);
	lua_pushstring(State, "__input");
	lua_pushvalue(State, SETMENU_DATA);
	lua_rawset(State, -3);
	lua_pop(State, 3);
	g_VideoOk = 1;
	GuiZBuffAdd(MenuContainer);
	return 0;
}

int LuaCloseMenu(lua_State* State) {
	int Len = 0;

	g_GUIMenuChange = 1;
	//Get the current menu.
	lua_getglobal(State, "Gui");
	lua_pushstring(State, "Menu");
	lua_rawget(State, -2);

	//Call its Quit function.
	lua_pushstring(State, "Quit");
	lua_rawget(State, -2);
	lua_pushvalue(State, -2);
	LuaCallFunc(State, 1, 0, 0);

	lua_pushstring(State, "__name");
	lua_rawget(State, -2);
	lua_getglobal(State, lua_tostring(State, -1));
	lua_pop(State, 3);

	lua_pushstring(State, "ScreenStack");
	lua_rawget(State, -2);
	Len = lua_rawlen(State, -1);
	if(Len > 0) {
		lua_rawgeti(State, -1, Len);
		lua_pushstring(State, "__ehooks");
		lua_newtable(State);

		lua_pushstring(State, "__focus");
		lua_pushlightuserdata(State, g_Focus);
		lua_rawset(State, -3);
		lua_pop(State, 3);
		if(Len == 0)
			g_VideoOk = 0;
		goto no_destroy;
	} else {
		g_VideoOk = 0;
		lua_pop(State, 1);
	}
	//destroy:
	/*lua_pushstring(State, "EventIds");
	lua_rawget(State, -2);
	for(i = 0; i < g_GUIEvents->Size; ++i) {
		luaL_unref(State, -1, g_GUIEvents->Events[i].RefId);
	}
	lua_pop(State, 1);
	GuiClear(State);
	DestroyFocus(g_Focus);
	DestroyGUIEvents(g_GUIEvents);
	free(StackPop(&g_GUIStack));*/
	no_destroy:
	g_Focus = NULL;
	lua_pop(State, 1);
	return 0;
}

void LuaSetColor(lua_State* State, unsigned char* RedPtr, unsigned char* GreenPtr, unsigned char* BluePtr) {
	int Red = luaL_checkint(State, 1);
	int Green = luaL_checkint(State, 2);
	int Blue = luaL_checkint(State, 3);

	if(Red < 0 || Red > 255)
		luaL_error(State, "Red is not between 0 and 255");
	if(Green < 0 || Green > 255)
		luaL_error(State, "Green is not between 0 and 255");
	if(Blue < 0 || Red > 255)
		luaL_error(State, "Blue is not between 0 and 255");
	*RedPtr = Red;
	*GreenPtr = Green;
	*BluePtr = Blue;
}
int LuaWidgetOnEvent(lua_State* State, void(*Callback)(struct Widget*)) {
	int RefId = 0;

	lua_getglobal(State, "Gui");
	lua_pushstring(State, "EventIds");
	lua_rawget(State, -2);
	lua_pushlightuserdata(State, Callback);
	RefId = luaL_ref(State, -2);
	lua_pop(State, 2);
	return RefId;
}

int LuaOnKey(lua_State* State) {
	LuaCheckClass(State, 1, LOBJ_WIDGET);
	luaL_argcheck(State, (lua_isfunction(State, 2) == 1 || lua_iscfunction(State, 2) == 1), 2, "Is not a function");
	lua_pushvalue(State, 2);
	lua_rawseti(State, 1, GUIL_ONKEY);
	return 0;
}

int LuaWidgetOnClick(lua_State* State) {
	LuaCheckClass(State, 1, LOBJ_WIDGET);
	luaL_argcheck(State, lua_isfunction(State, 2), 2, "Is not a function.");
	lua_rawseti(State, 1, GUIL_ONCLICK);
	return 0;
}

int LuaPopMenu(lua_State* State) {
	char* String = NULL;

	free(StackPop(&g_GUIStack));
	if((String = (char*)StackGet(&g_GUIStack, -1)) == NULL) {
		g_VideoOk = 0;
		return 0;
	}
	lua_pushstring(State, String);
	LuaSetMenu_Aux(State);
	return 0;
}

void LuaMenuThink(lua_State* State) {
	lua_getglobal(State, "Gui");
	lua_pushstring(State, "Menu");
	lua_rawget(State, -2);

	lua_pushstring(State, "Think");
	lua_rawget(State, -2);
	lua_pushvalue(State, -2);
	LuaCallFunc(State, 1, 0, 0);
	lua_pop(State, 2);
}

int LuaScreenWidth(lua_State* State) {
	lua_pushinteger(State, SDL_WIDTH);
	return 1;
}

int LuaScreenHeight(lua_State* State) {
	lua_pushinteger(State, SDL_HEIGHT);
	return 1;
}

/*
 * NOTE: Do we need the GUI message code still?
 */
int LuaSendMessage(lua_State* State) {
	luaL_checkstring(State, 1);
	if(lua_gettop(State) != 2) {
		luaL_error(State, "SendMessage does not have any data.");
		return 0;
	}
	lua_getglobal(State, "Gui");
	lua_pushstring(State, "Messages");
	lua_rawget(State, -2);
	lua_pushvalue(State, 1);
	lua_pushvalue(State, 2);
	lua_rawset(State, -3);
	return 0;
}

int LuaWidgetId(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	lua_pushinteger(State, Widget->Id);
	return 1;
}

int LuaWidgetSetPos(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);
	SDL_Point Pos;

	Pos.x = luaL_checkinteger(State, 2);
	Pos.y = luaL_checkinteger(State, 3);
	Widget->SetPosition(Widget, &Pos);
	return 0;
}

int LuaWidgetSetX(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);
	SDL_Point Pos = {0, Widget->Rect.y};

	Pos.x = luaL_checkinteger(State, 2);
	Widget->SetPosition(Widget, &Pos);
	//_Widget->Rect.x = luaL_checkinteger(State, 2);
	return 0;
}

int LuaWidgetGetX(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);
	SDL_Point Pos = {Widget->Rect.x, 0};

	Pos.y = luaL_checkinteger(State, 2);
	Widget->SetPosition(Widget, &Pos);
	//lua_pushinteger(State, Widget->Rect.y);
	return 1;
}

int LuaWidgetSetY(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	Widget->Rect.y = luaL_checkinteger(State, 2);
	return 0;
}

int LuaWidgetGetY(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	lua_pushinteger(State, Widget->Rect.y);
	return 1;
}

int LuaWidgetGetWidth(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	lua_pushinteger(State, Widget->Rect.w);
	return 1;
}

int LuaWidgetSetWidth(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);
	int Width = luaL_checkinteger(State, 2);

	//_Widget->Rect.w = Width;
	WidgetSetWidth(Widget, Width);
	lua_pushvalue(State, 1);
	return 1;
}

int LuaWidgetGetHeight(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	lua_pushinteger(State, Widget->Rect.h);
	return 1;
}

int LuaWidgetSetHeight(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);
	int Height = luaL_checkinteger(State, 2);

	WidgetSetHeight(Widget, Height);
	lua_pushvalue(State, 1);
	return 1;
}

int LuaWidgetGetParent(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	LuaGuiGetRef(State);
	lua_rawgeti(State, -1, Widget->Parent->Widget.LuaRef);
	return 1;
}

int LuaWidgetGetFocus(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	lua_pushboolean(State, Widget->CanFocus);
	return 1;
}

int LuaWidgetSetFocus(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	luaL_checktype(State, 2, LUA_TBOOLEAN);
	Widget->CanFocus = lua_toboolean(State, 2);
	lua_pushvalue(State, 1);
	return 1;
}

int LuaWidgetDestroy(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);

	if(Widget == NULL)
		return 0;
	lua_pushstring(State, "__self");
	lua_pushnil(State);
	lua_rawset(State, -3);
	Widget->OnDestroy(Widget, State);
	return 0;
}

int LuaWidgetOnHover(lua_State* State) {
	LuaCheckClass(State, 1, LOBJ_WIDGET);
	luaL_checktype(State, 2, LUA_TFUNCTION);
	lua_rawseti(State, 1, GUIL_ONHOVER);
	return 0;
}

int LuaWidgetOnHoverLoss(lua_State* State) {
	LuaCheckClass(State, 1, LOBJ_WIDGET);
	luaL_checktype(State, 2, LUA_TFUNCTION);
	lua_rawseti(State, 1, GUIL_ONHOVERLOSS);
	return 0;
}

int LuaContainerGetChild(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	int Index = luaL_checkinteger(State, 2);

	LuaGuiGetRef(State);
	lua_rawgeti(State, -1, Container->Children[Index]->LuaRef);
	lua_remove(State, -2);
	return 1;
}

int LuaContainerSetChild(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	int Index = luaL_checkinteger(State, 2);
	struct Widget* Child = LuaCheckClass(State, 1, LOBJ_WIDGET);

	luaL_argcheck(State, (Index >= 0 && Index < Container->ChildrenSz), 2, "Index is out of bounds.");
	if(Container->Children[Index] != NULL) {
		Container->Children[Index]->OnDestroy(Container->Children[Index], State);
		Container->Children[Index] = Child;
	} else {
		Container->Children[Index] = Child;
		++Container->ChildCt;
	}
	return 0;
}

int LuaContainerGetChildCt(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);

	lua_pushinteger(State, Container->ChildCt);
	return 1;
}

int LuaContainerGetChildren(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);

	LuaGuiGetRef(State);
	lua_newtable(State);
	for(int i = 0; i < Container->ChildCt; ++i) {
		lua_rawgeti(State, -2, Container->Children[i]->LuaRef);
		lua_rawseti(State, -2, i + 1);
	}
	lua_insert(State, -2);
	lua_pop(State, 1);
	return 1;
}

int LuaContainerDestroyChildren(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	
	for(int i = 0; i < Container->ChildCt; ++i) {
		if(Container->Children[i] == NULL)
			continue;
		Container->Children[i]->OnDestroy(Container->Children[i], State);	
	}
	Container->ChildCt = 0;
	return 0;
}

int LuaContainerHorizontalCenter(lua_State* State) {
	lua_pushinteger(State, GetHorizontalCenter(LuaCheckClass(State, 1, LOBJ_CONTAINER), LuaCheckClass(State, 2, LOBJ_WIDGET)));
	return 1;
}

int LuaContainerClear(lua_State* State) {
	struct Container* Container = LuaToObject(State, 1, LOBJ_CONTAINER);
	GuiParentFunc RemChild = Container->RemChild;


	Container->RemChild = StaticRemChild;
	for(int i = 0; i < Container->ChildCt; ++i) {
		if(Container->Children[i] != NULL) {
			Container->Children[i]->OnDestroy(Container->Children[i], State);
		}
	}
	Container->ChildCt = 0;
	Container->RemChild = RemChild;
	return 0;
}

int LuaContainerClose(lua_State* State) {
	struct Container* Container = LuaToObject(State, 1, LOBJ_CONTAINER);

	Container->Widget.OnDestroy((struct Widget*)Container, State);
	return 0;
}

int LuaContainerShrink(lua_State* State) {
	struct Container* Container = LuaToObject(State, 1, LOBJ_CONTAINER);

	ContainerShrink(Container);
	return 0;
}

int LuaContainerAddChild(lua_State* State) {
	struct Container* Container = LuaToObject(State, 1, LOBJ_CONTAINER);
	struct Widget* Widget = LuaToObject(State, 2, LOBJ_WIDGET);

	WidgetSetParent(Container, Widget);
	return 0;
}

int LuaContainerSetSkin(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	struct GuiSkin* Skin = LuaCheckClass(State, 2, LOBJ_GSKIN);

	if(Skin == NULL) return luaL_error(State, "Skin is NULL.");
	Container->Skin = Skin;
	return 0;
}

int LuaContainerGetSkin(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);

	LuaConstCtor(State, Container->Skin, LOBJ_GSKIN);
	return 1;
}

int LuaContainerOnNewChild(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	int OnChild = luaL_checkinteger(State, 2);
	
	switch(OnChild) {
		case GUIL_CNVERT:
			Container->NewChild = VertConNewChild;
			break;
		case GUIL_CNHORZ:
			Container->NewChild = HorzConNewChild;
			break;
		case GUIL_CNFIXED:
			Container->NewChild = FixedConNewChild;
			break;
	}
	return 0;
}


int LuaContainerCreateContainer(lua_State* State) {
	//struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);

	LuaContainer(State);
	lua_insert(State, 1);
	lua_insert(State, 2);
	lua_settop(State, 2);
	LuaContainerOnNewChild(State);
	lua_pop(State, 1);
	return 1;
}

int LuaContainerOnHoverLoss(lua_State* State) {
	LuaCheckClass(State, 1, LOBJ_CONTAINER);
	luaL_checktype(State, 2, LUA_TFUNCTION);
	lua_rawseti(State, 1, GUIL_ONHOVERLOSS);
	return 0;
}

int LuaContainerParagraph(lua_State* State) {
	struct Container* Parent = LuaCheckClass(State, 1, LOBJ_CONTAINER);
	struct Container* NewContainer = NULL;
	struct Font* Font = NULL;
	const char* String = luaL_checkstring(State, 2);
	const char* Temp = String;
	int CharWidth = 0;
	int WordSz = 0;
	int WordWidth = 0;
	struct Label* Label = NULL;
	SDL_Rect Rect = {0, 0, 0, 0};
	SDL_Rect PRect = {0, 0, 0, 0};
	SDL_Surface* Surface = NULL;
	SDL_Color White = {255, 255, 255};
	int Ct = 0;

	if(lua_gettop(State) < 3)
		Font = Parent->Skin->Label->Font;
	else
		Font = LuaCheckClass(State, 3, LOBJ_FONT);
	Rect.w = TTF_FontFaceIsFixedWidth(Font->Font);
	Rect.x = 0;
	Rect.y = 0;
	PRect.w = Parent->Widget.Rect.w;
	PRect.h = Parent->Widget.Rect.h;
	NewContainer = CreateContainer();
	lua_createtable(State, 2, 2);
	ConstructContainer(NewContainer, Parent, &PRect, State);
	NewContainer->NewChild = VertConNewChild;
	NewContainer->Widget.CanFocus = 0;
	while(*Temp != '\0') {
		do {
			if((*Temp) == ' ') {
				Ct += WordSz;
				Rect.w += WordWidth;
				WordSz = 0;
				WordWidth = 0;
			}
			TTF_GlyphMetrics(Font->Font, *Temp, NULL, NULL, NULL, NULL, &CharWidth);
			WordWidth += CharWidth;
			if(Rect.w + WordWidth + CharWidth > Parent->Widget.Rect.w) {
				Temp -= WordSz;
				//NOTE: Used to stop infinite loop when the parent is to small to hold a single letter.
				if(WordSz == 0 && Temp == String)
					goto func_end;
				WordSz = 0;
				WordWidth = 0;
				goto create_buffer;
				break;
			}
			++WordSz;
			++Temp;
		} while((*Temp) != '\0');
		create_buffer:
		Ct += WordSz;
		Rect.w += WordWidth;
		WordSz = 0;
		WordWidth = 0;
		if(Ct > 0) {
			char Buffer[Ct + 1];
			strncpy(Buffer, String, Ct);
			Buffer[Ct] = '\0';
			Label = CreateLabel();
			Surface = TTF_RenderText_Solid(Font->Font, Buffer, White);
			Rect.w = Surface->w;
			Rect.h = Surface->h;
			PRect.h += Rect.h;
			ConstructLabel(Label, NewContainer, &Rect, State, SurfaceToTexture(ConvertSurface(Surface)), Parent->Skin->Label);
		String = Temp + 1;
		Label->Widget.CanFocus = 0;
		}
		Rect.w = 0;
		Ct = 0;
	}
	func_end:
	ContainerShrink(NewContainer);
	LuaCtor(State, NewContainer, LOBJ_CONTAINER);
	return 1;
}

int LuaLabelSetText(lua_State* State) {
	struct Label* Label = LuaCheckClass(State, 1, LOBJ_LABEL);
	const char* Text = luaL_checkstring(State, 2);

	SDL_Surface* Surface = ConvertSurface(TTF_RenderText_Solid(Label->Widget.Style->Font->Font, Text, Label->Widget.Style->FontUnfocus));
	Label->SetText((struct Widget*)Label, SurfaceToTexture(Surface));
	return 0;
}

int LuaButtonClickable(lua_State* State) {
	struct Button* Button = LuaCheckClass(State, 1, LOBJ_BUTTON);

	ButtonSetClickable(Button, 1);
	return 0;
}

int LuaButtonUnclickable(lua_State* State) {
	struct Button* Button = LuaCheckClass(State, 1, LOBJ_BUTTON);

	ButtonSetClickable(Button, 0);
	return 0;
}

int LuaTableGetCellIndex(lua_State* State) {
	struct Table* Table = LuaCheckClass(State, 1, LOBJ_TABLE);
	int Row = luaL_checkinteger(State, 2);
	int Col = luaL_checkinteger(State, 3);

	lua_pushinteger(State, Row + Col * Table->Rows);
	return 1;
}

int LuaTableGetRows(lua_State* State) {
	struct Table* Table = LuaCheckClass(State, 1, LOBJ_TABLE);

	lua_pushinteger(State, Table->Rows);
	return 1;
}

int LuaTableGetColumns(lua_State* State) {
	struct Table* Table = LuaCheckClass(State, 1, LOBJ_TABLE);

	lua_pushinteger(State, Table->Columns);
	return 1;
}

int LuaTableSetCellWidth(lua_State* State) {
	struct Table* Table = LuaCheckClass(State, 1, LOBJ_TABLE);
	int CellMax = luaL_checkinteger(State, 2);

	if(WidgetSetWidth(&Table->Container.Widget, CellMax * Table->Rows) == true)
		Table->CellMax.w = CellMax;
	lua_pushvalue(State, 1);
	return 1;
}

int LuaTableSetCellHeight(lua_State* State) {
	struct Table* Table = LuaCheckClass(State, 1, LOBJ_TABLE);
	int CellMax = luaL_checkinteger(State, 2);

	if(WidgetSetHeight(&Table->Container.Widget, CellMax * Table->Columns) == true)
		Table->CellMax.h = CellMax;
	lua_pushvalue(State, 1);
	return 1;
}

int LuaTableGetCellWidth(lua_State* State) {
	struct Table* Table = LuaCheckClass(State, 1, LOBJ_TABLE);

	lua_pushinteger(State, Table->CellMax.w);
	return 1;
}

int LuaTableGetCellHeight(lua_State* State) {
	struct Table* Table = LuaCheckClass(State, 1, LOBJ_TABLE);

	lua_pushinteger(State, Table->CellMax.h);
	return 1;
}

int LuaFontWidth(lua_State* State) {
	struct Font* Font = LuaCheckClass(State, 1, LOBJ_FONT);
	int Size = TTF_FontFaceIsFixedWidth(Font->Font);

	if(Size == 0)
		TTF_SizeText(Font->Font, "w", &Size, NULL);
	lua_pushinteger(State, Size);
	return 1;
}

int LuaFontHeight(lua_State* State) {
	struct Font* Font = LuaCheckClass(State, 1, LOBJ_FONT);

	lua_pushinteger(State, TTF_FontHeight(Font->Font));
	return 1;
}

int InitGUILua(lua_State* State) {
	DIR* Dir = NULL;
	struct dirent* Dirent = NULL;
	char* Temp = NULL;
	const char* Ext = NULL;

	RegisterLuaObjects(State, g_GuiLuaObjects);
	luaL_newlib(State, g_LuaFuncsGUI);
	lua_setglobal(State, "Gui");

	lua_newtable(State);
	lua_pushstring(State, "Menu");
	lua_newtable(State);
	lua_rawset(State, -3);

	lua_newtable(State);
	lua_pushstring(State, "__index");
	lua_pushglobaltable(State);
	lua_rawset(State, -3);
	lua_setmetatable(State, -2);
	LuaSetEnv(State, "Menu");
	lua_pop(State, 1);
	if(LuaLoadFile(State, "data/video.lua", NULL) != LUA_OK)
		return 0;
	chdir("data/gui");
	Dir = opendir("./");
	while((Dirent = readdir(Dir)) != NULL) {
		if(((!strcmp(Dirent->d_name, ".") || !strcmp(Dirent->d_name, ".."))
			|| ((Ext = strrchr(Dirent->d_name, '.')) == NULL) || strncmp(Ext, ".lua", 4) != 0))
			continue;
		if(LuaLoadFile(State, Dirent->d_name, "Menu") != LUA_OK)
			goto error;
		Temp = strrchr(Dirent->d_name, '.');
		int Size = Temp - Dirent->d_name;
		char MenuName[Size + 1];
		strncpy(MenuName, Dirent->d_name, Size);
		MenuName[Size] = '\0';
		LuaAddMenu(State, MenuName);
	}
	chdir("../..");
	lua_getglobal(State, "Gui");

	lua_pushstring(State, "Widgets");
	lua_newtable(State);
	lua_rawset(State, -3);

	lua_pushstring(State, "ScreenStack");
	lua_newtable(State);
	lua_rawset(State, -3);

	lua_pushstring(State, "Messages");
	lua_newtable(State);
	lua_rawset(State, -3);

	lua_pushstring(State, "EventIds");
	lua_createtable(State, LUA_EVENTIDSIZE, 0);
	lua_rawset(State, -3);

	lua_pushstring(State, "Init");
	lua_rawget(State, -2);
	if(LuaCallFunc(State, 0, 0, 0) == 0)
		goto error;
	closedir(Dir);
	lua_pop(State, 1);
	return 1;
	error:
	chdir("../..");
	lua_pop(State, 1);
	return 0;
}

int QuitGUILua(lua_State* State) {
	struct Container* Container = NULL;

	/* Free all containers on the ScreenStack. */
	lua_getglobal(State, "Gui");
	lua_pushstring(State, "ScreenStack");
	lua_rawget(State, -2);
	if(lua_type(State, -1) == LUA_TNIL) {
		luaL_error(State, "QuitGUILua has no ScreenStack.");
		return 0;
	}
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		lua_pushstring(State, "__screen");
		lua_rawget(State, -2);
		Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);
		Container->Widget.OnDestroy((struct Widget*)Container, State);
		lua_pop(State, 2);
	}
	lua_pop(State, 2);
	GuiClear(State);
	return 1;
}

int LuaKeyState(lua_State* State, int Index) {
	const char* Type = NULL;

	if(lua_type(State, Index) != LUA_TSTRING)
		return -1;
	Type = lua_tostring(State, Index);
	if(strcmp(Type, "Released") == 0)
		return SDL_RELEASED;
	else if(strcmp(Type, "Pressed") == 0)
		return SDL_PRESSED;
	return -1;
}

void LuaGuiGetRef(lua_State* State) {
	lua_pushvalue(State, LUA_REGISTRYINDEX);
}

int LuaWidgetRef(lua_State* State) {
	int Ref = 0;

	LuaGuiGetRef(State);
	lua_pushvalue(State, -2);
	Ref = luaL_ref(State, -2);
	lua_pop(State, 1);
	return Ref;
}

void LuaWidgetUnref(lua_State* State, struct Widget* Widget) {
	LuaGuiGetRef(State);
	luaL_unref(State, -1, Widget->LuaRef);
	lua_pop(State, 1);
}

void LuaAddMenu(lua_State* State, const char* Name) {
	LuaGetEnv(State, "Menu");
	lua_pushstring(State, "Menu");
	lua_rawget(State, -2);

	//NOTE: Should this be placed in the registry instead to avoid GC cleanup?
	lua_setglobal(State, Name);
	lua_pushstring(State, "Menu");
	lua_newtable(State);
	lua_rawset(State, -3);

	lua_pop(State, 1);
}

void CreateMenu(const char* Menu) {
//	lua_settop(g_LuaState, 0);
	lua_pushcfunction(g_LuaState, LuaCreateWindow);
	//lua_getglobal(g_LuaState, "Gui");
	//if(lua_isnil(g_LuaState, 1))
	//	return;
	//lua_pushstring(g_LuaState, "CreateWindow");
	//lua_rawget(g_LuaState, -2);
	//lua_remove(g_LuaState, -2);

	lua_pushstring(g_LuaState, Menu);
	lua_pushvalue(g_LuaState, -3);
	//lua_createtable(g_LuaState, 0, 1);
	//lua_pushstring(g_LuaState, "Text");
	//lua_pushstring(g_LuaState, Text);
	//lua_rawset(g_LuaState, -3);
	LuaCallFunc(g_LuaState, 2, 0, 0);
}

void GuiSetParentHook(struct Container* Container) {
	g_GuiParentHook = Container;
}

struct Container* GuiGetParentHook(void) {
	return g_GuiParentHook;
}

int LuaGuiClose(lua_State* State) {
	GuiClear(State);
	while(g_GUIStack.Size > 0)
		free(StackPop(&g_GUIStack));
	g_VideoOk = 0;
	return 0;
}

int LuaGuiMenuHorCenter(lua_State* State) {
	struct Widget* Widget = LuaCheckClass(State, 1, LOBJ_WIDGET);
	int Width = 0;

	if(lua_type(State, 1) != LUA_TTABLE)
		return luaL_error(State, "Arg #1 is not a table.");
	lua_getfield(State, 1, "Width");	
	return ((Width / 2) - (Widget->Rect.w / 2));
}

int LuaContainerLeftOf(lua_State* State) {
	struct Widget* Rel = LuaCheckClass(State, 1, LOBJ_WIDGET);
	struct Widget* Base = LuaCheckClass(State, 2, LOBJ_WIDGET);
	SDL_Point Pos = {0, 0};
	
	if(Rel->Parent != Base->Parent)
		return luaL_error(State, "Arg #1 and Arg #2 do not have the same parent.");
	Pos.x = Base->Rect.x - Base->Rect.w - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y - Rel->Parent->Widget.Rect.y;
	Rel->SetPosition((struct Widget*) Rel, &Pos);
	lua_pushvalue(State, 1);
	return 1;
}

int LuaContainerRightOf(lua_State* State) {
	struct Widget* Rel = LuaCheckClass(State, 1, LOBJ_WIDGET);
	struct Widget* Base = LuaCheckClass(State, 2, LOBJ_WIDGET);
	SDL_Point Pos = {0, 0};
	
	if(Rel->Parent != Base->Parent)
		return luaL_error(State, "Arg #1 and Arg #2 do not have the same parent.");
	Pos.x = Base->Rect.x + Base->Rect.w - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y - Rel->Parent->Widget.Rect.y;
	Rel->SetPosition((struct Widget*) Rel, &Pos);
	lua_pushvalue(State, 1);
	return 1;
}

int LuaContainerAbove(lua_State* State) {
	struct Widget* Rel = LuaCheckClass(State, 1, LOBJ_WIDGET);
	struct Widget* Base = LuaCheckClass(State, 2, LOBJ_WIDGET);
	SDL_Point Pos = {0, 0};
	
	if(Rel->Parent != Base->Parent)
		return luaL_error(State, "Arg #1 and Arg #2 do not have the same parent.");
	Pos.x = Base->Rect.x - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y - Rel->Rect.h - Rel->Parent->Widget.Rect.y;
	Base->SetPosition((struct Widget*) Rel, &Pos);
	lua_pushvalue(State, 1);
	return 1;
}

int LuaContainerBelow(lua_State* State) {
	struct Widget* Rel = LuaCheckClass(State, 1, LOBJ_WIDGET);
	struct Widget* Base = LuaCheckClass(State, 2, LOBJ_WIDGET);
	SDL_Point Pos = {0, 0};
	
	if(Rel->Parent != Base->Parent)
		return luaL_error(State, "Arg #1 and Arg #2 do not have the same parent.");
	Pos.x = Base->Rect.x - Rel->Parent->Widget.Rect.x;
	Pos.y = Base->Rect.y + Base->Rect.h - Rel->Parent->Widget.Rect.y;
	Rel->SetPosition((struct Widget*)Rel, &Pos);
	lua_pushvalue(State, 1);
	return 1;
}

int LuaContainergetSkin(lua_State* State) {
	struct Container* Container = LuaCheckClass(State, 1, LOBJ_CONTAINER);

	LuaConstCtor(State, Container->Skin, LOBJ_GSKIN);
	return 1;
}

int LuaGuiSkin(lua_State* State) {
	const char* Name = NULL;
	struct GuiSkin* Skin = NULL;
	struct GuiStyle* Default = NULL;
	struct GuiStyle* Style = NULL;
	struct GuiStyle** StylePtr = NULL;
	static const char* StyleStr[] = {
		"Label",
		"Button",
		"Table",
		"Container",
		NULL
	};

	luaL_checktype(State, 1, LUA_TTABLE);

	lua_pushstring(State, "Name");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TSTRING) {
		return luaL_error(State, "GuiSkin does not contain a name.");
	}
	Name = lua_tostring(State, 2);
	lua_pushstring(State, "Default");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TTABLE) {
		return luaL_error(State, "GuiSkin Default must be a table.");
	}
	Default = LuaGuiStyle(State, -1);
	Skin = malloc(sizeof(struct GuiSkin));
	StylePtr = &Skin->Label;
	for(int i = 0; StyleStr[i] != NULL; ++i) {
		lua_pushstring(State, StyleStr[i]);
		lua_rawget(State, 1);
		if(lua_type(State, -1) != LUA_TTABLE) {
			Style = NULL;
		} else {
			Style = LuaGuiStyle(State, -1);
		}
		*StylePtr = (Style != NULL) ? (Style) : (Default); 
		++StylePtr;
	}

	Skin->Name = calloc(strlen(Name) + 1, sizeof(char));
	strcpy((char*)Skin->Name, Name);
	if(g_GuiSkinDefault == NULL) {
		g_GuiSkinDefault = Skin;
		g_GuiSkinCurr = g_GuiSkinDefault;
	}
	if(g_GuiSkins.TblSize <= g_GuiSkins.Size)
		HashResize(&g_GuiSkins);
	HashInsert(&g_GuiSkins, Skin->Name, Skin);
	lua_pop(State, 1);
	return 0;
}

void LuaColorToSDL(lua_State* State, int Index, SDL_Color* Color) {
	Index = lua_absindex(State, Index);
	//LuaTestClass(State, Index, LOBJ_COLOR);
	lua_rawgeti(State, Index, 1);
	Color->r = lua_tointeger(State, -1);
	lua_rawgeti(State, Index, 2);
	Color->g = lua_tointeger(State, -1);
	lua_rawgeti(State, Index, 3);
	Color->b = lua_tointeger(State, -1);
	Color->a = 0xFF;
	lua_pop(State, 3);
}

void LuaSDLToColor(lua_State* State, SDL_Color* Color) {
	lua_createtable(State, 4, 0);
	lua_pushinteger(State, Color->r);
	lua_rawseti(State, 2, 1);
	lua_pushinteger(State, Color->b);
	lua_rawseti(State, 2, 2);
	lua_pushinteger(State, Color->g);
	lua_rawseti(State, 2, 3);
	lua_pushinteger(State, Color->a);
	lua_rawseti(State, 2, 4);
}

struct GuiStyle* LuaGuiStyle(lua_State* State, int Index) {
	struct GuiStyle* Style = malloc(sizeof(struct GuiStyle));

	Index = lua_absindex(State, Index);
	lua_pushstring(State, "Font");
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TTABLE) {
		free(Style);
		return (void*)luaL_error(State, "GuiStyle Font must be a Font.");
	}
	Style->Font = LuaCheckClass(State, -1, LOBJ_FONT);

	lua_pushstring(State, "FocusColor");
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TTABLE) {
		free(Style);
		return (void*)luaL_error(State, "GuiStyle FocusColor must be a color.");
	}
	LuaColorToSDL(State, -1, &Style->FontFocus);
	lua_pop(State, 1);
	lua_pushstring(State, "UnfocusColor");
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TTABLE) {
		free(Style);
		return (void*)luaL_error(State, "GuiStyle UnfocusColor must be a color.");
	}
	LuaColorToSDL(State, -1, &Style->FontUnfocus);
	lua_pop(State, 1);
	lua_pushstring(State, "Background");
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TTABLE) {
		free(Style);
		return (void*)luaL_error(State, "GuiStyle Background must be a color.");
	}
	LuaColorToSDL(State, -1, &Style->Background);
	lua_pop(State, 1);
	
	lua_pushstring(State, "Margins");
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TTABLE) {
		if(lua_type(State, -1) == LUA_TNIL) {
			for(int i = 0; i < 4; ++i) {
				((int32_t*)&Style->Margins)[i] = 0;
			}
			goto end;
		}
		free(Style);
		return (void*)luaL_error(State, "GuiStyle Margins must be a table.");
	}
	for(int i = 1; i <= 4; ++i) {
		lua_rawgeti(State, -1, i);	
		if(lua_type(State, -1) != LUA_TNUMBER) {
			free(Style);
			return (void*)luaL_error(State, "GuiStyle Margins index #%d must be an integer.", i);
		}
		((int32_t*)&Style->Margins)[i - 1] = lua_tointeger(State, -1);
		lua_pop(State, 1);
	}
	lua_pop(State, 1);
	end:
	return Style;
}

int LuaColor(lua_State* State) {
	int Red = luaL_checkint(State, 1);
	int Green = luaL_checkint(State, 2);
	int Blue = luaL_checkint(State, 3);

	if(Red < 0 || Red > 255)
		return luaL_error(State, "Red is out of bounds.");
	if(Green < 0 || Green > 255)
		return luaL_error(State, "Green is out of bounds.");
	if(Blue < 0 || Blue > 255)
		return luaL_error(State, "Blue is out of bounds.");
	lua_createtable(State, 3, 0);
	lua_pushvalue(State, 1);
	lua_rawseti(State, -2, 1);
	lua_pushvalue(State, 2);
	lua_rawseti(State, -2, 2);
	lua_pushvalue(State, 3);
	lua_rawseti(State, -2, 3);
	return 1;
}

int LuaGuiSetSkin(lua_State* State) {
	const char* SkinStr = luaL_checkstring(State, 1);
	struct GuiSkin* Skin = HashSearch(&g_GuiSkins, SkinStr);

	if(Skin == NULL) {
		return luaL_error(State, "%s is not a valid Gui skin name.", SkinStr);
	}
	g_GuiSkinCurr = Skin;
	return 0;
}

int LuaGuiGetSkin(lua_State* State) {
	const char* SkinStr = luaL_checkstring(State, 1);
	struct GuiSkin* Skin = HashSearch(&g_GuiSkins, SkinStr);

	if(Skin == NULL) {
		return luaL_error(State, "%s is not a valid Gui skin name.", SkinStr);
	}
	LuaCtor(State, Skin, LOBJ_GSKIN);
	return 1;
}

int	LuaGuiStyleGetFont(lua_State* State) {
	struct GuiStyle* Style = LuaCheckClass(State, 1, LOBJ_GSTYLE);

	LuaCtor(State, Style->Font, LOBJ_FONT);
	return 1;
}

int	LuaGuiStyleFontFocus(lua_State* State) {
	struct GuiStyle* Style = LuaCheckClass(State, 1, LOBJ_GSTYLE);

	LuaSDLToColor(State, &Style->FontFocus);
	return 1;
}

int	LuaGuiStyleFontUnfocus(lua_State* State) {
	struct GuiStyle* Style = LuaCheckClass(State, 1, LOBJ_GSTYLE);

	LuaSDLToColor(State, &Style->FontUnfocus);
	return 1;
}

int	LuaGuiStyleBackgroundColor(lua_State* State) {
	struct GuiStyle* Style = LuaCheckClass(State, 1, LOBJ_GSTYLE);

	LuaSDLToColor(State, &Style->Background);
	return 1;
}

int	LuaGuiStyleMargins(lua_State* State) {
	struct GuiStyle* Style = LuaCheckClass(State, 1, LOBJ_GSTYLE);

	lua_createtable(State, 4, 0);
	lua_pushinteger(State, Style->Margins.Top);
	lua_rawseti(State, -2, 1);
	lua_pushinteger(State, Style->Margins.Left);
	lua_rawseti(State, -2, 1);
	lua_pushinteger(State, Style->Margins.Bottom);
	lua_rawseti(State, -2, 1);
	lua_pushinteger(State, Style->Margins.Right);
	lua_rawseti(State, -2, 1);
	return 0;
}

int	LuaGuiSkinGetName(lua_State* State) {
	struct GuiSkin* Skin = LuaCheckClass(State, 1, LOBJ_GSKIN);

	lua_pushstring(State, Skin->Name);
	return 1;
}

int	LuaGuiSkinLabel(lua_State* State) {
	struct GuiSkin* Skin = LuaCheckClass(State, 1, LOBJ_GSKIN);

	LuaCtor(State, Skin->Label, LOBJ_GSTYLE);
	return 1;
}

int	LuaGuiSkinButton(lua_State* State) {
	struct GuiSkin* Skin = LuaCheckClass(State, 1, LOBJ_GSKIN);

	LuaCtor(State, Skin->Button, LOBJ_GSTYLE);
	return 1;
}

int	LuaGuiSkinTable(lua_State* State) {
	struct GuiSkin* Skin = LuaCheckClass(State, 1, LOBJ_GSKIN);

	LuaCtor(State, Skin->Table, LOBJ_GSTYLE);
	return 1;
}

int	LuaGuiSkinContainer(lua_State* State) {
	struct GuiSkin* Skin = LuaCheckClass(State, 1, LOBJ_GSKIN);

	LuaCtor(State, Skin->Container, LOBJ_GSTYLE);
	return 1;
}
