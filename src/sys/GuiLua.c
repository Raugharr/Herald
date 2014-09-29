/*
 * File: LuaLib.c
 * Author: David Brotz
 */

#include "GuiLua.h"

#include "Video.h"
#include "../sys/LuaHelper.h"
#include "../sys/Array.h"

#include <sdl2/SDL.h>
#include <sdl2/SDL_events.h>
#include <SDL2/SDL_ttf.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <io.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

static const luaL_Reg g_LuaFuncsGUI[] = {
		{"HorizontalContainer", LuaHorizontalContainer},
		{"VerticalContainer", LuaVerticalContainer},
		{"BackgroundColor", LuaBackgroundColor},
		{"GetFont", LuaGetFont},
		{"SetFont", LuaDefaultFont},
		{"DefaultFont", LuaDefaultFont},
		{"SetMenu", LuaSetMenu},
		{"SetColor", LuaSetColor},
		{"Quit", LuaQuit},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsWidget[] = {
		{"Id", LuaWidgetId},
		{"X", LuaWidgetGetX},
		{"Y", LuaWidgetGetY},
		{"Width", LuaWidgetGetWidth},
		{"Height", LuaWidgetGetHeight},
		{"Parent", LuaWidgetGetParent},
		{"Children", LuaWidgetGetChildren},
		{"OnKey", LuaOnKey},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsContainer[] = {
		{"Spacing", LuaContainerGetSpacing},
		{"Margins", LuaContainerGetMargins},
		{"CreateTextBox", LuaCreateTextBox},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsTextBox[] = {
		{"SetText", LuaTextBoxSetText},
		{NULL, NULL}
};

int LuaRegisterWidget(lua_State* _State) {
	if(luaL_newmetatable(_State, "Widget") == 0)
		return 0;

	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	luaL_setfuncs(_State, g_LuaFuncsWidget, 0);
	lua_setglobal(_State, "Widget");
	return 1;
}

int LuaRegisterContainer(lua_State* _State) {
	lua_getglobal(_State, "Widget");
	if(luaL_newmetatable(_State, "Container") == 0) {
		lua_pop(_State, 1);
		return 0;
	}
	LuaCopyTable(_State, -2);
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__baseclass");
	lua_getglobal(_State, "Widget");
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsContainer, 0);
	lua_setglobal(_State, "Container");
	return 1;
}

int LuaRegisterTextBox(lua_State* _State) {
	lua_getglobal(_State, "Widget");
	if(luaL_newmetatable(_State, "TextBox") == 0) {
		lua_pop(_State, 1);
		return 0;
	}
	LuaCopyTable(_State, -2);

	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__baseclass");
	lua_getglobal(_State, "Widget");
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsTextBox, 0);
	lua_setglobal(_State, "TextBox");
	return 1;
}

int LuaRegisterSurface(lua_State* _State) {
	if(luaL_newmetatable(_State, "Surface") == 0)
		return 0;

	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_setglobal(_State, "Surface");
	return 1;
}

struct Widget* LuaWidgetPush(lua_State* _State) {
	struct Widget* _Widget = (struct Widget*) lua_newuserdata(_State, sizeof(struct Widget));

	luaL_getmetatable(_State, "Widget");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "Children");
	lua_newtable(_State);
	lua_rawset(_State, -3);
	return _Widget;
}

struct Container* LuaContainerPush(lua_State* _State) {
	struct Container* _Container = (struct Container*) lua_newuserdata(_State, sizeof(struct Container));

	luaL_getmetatable(_State, "Container");
	lua_pushvalue(_State, -1);
	lua_setmetatable(_State, -3);

	lua_pushstring(_State, "Children");
	lua_newtable(_State);
	lua_rawset(_State, -3);
	lua_pop(_State, 1);
	return _Container;
}

struct TextBox* LuaTextBoxPush(lua_State* _State) {
	struct TextBox* _TextBox = (struct TextBox*) lua_newuserdata(_State, sizeof(struct TextBox));

	luaL_getmetatable(_State, "TextBox");
	lua_pushvalue(_State, -1);
	lua_setmetatable(_State, -3);

	lua_pushstring(_State, "Children");
	lua_newtable(_State);
	lua_rawset(_State, -3);
	lua_pop(_State, 1);
	return _TextBox;
}

int LuaCreateTextBox(lua_State* _State) {
	struct TextBox* _TextBox = NULL;
	struct Widget* _Parent = LuaCheckWidget(_State, 1);
	const char* _Text = luaL_checkstring(_State, 2);

	if(g_GUIDefs.Font == NULL)
		return luaL_error(_State, "Default font is NULL");
	_TextBox = LuaTextBoxPush(_State);
	_TextBox->Id = NextGUIId();
	_TextBox->OnDraw = TextBoxOnDraw;
	_TextBox->OnFocus = TextBoxOnFocus;
	_TextBox->OnUnfocus = TextBoxOnUnfocus;
	_TextBox->SetText = WidgetSetText;
	_TextBox->Text = TTF_RenderText_Solid(g_GUIDefs.Font, _Text, g_GUIDefs.FontUnfocus);
	_TextBox->Children = NULL;
	_TextBox->ChildrenSz = 0;
	_TextBox->CanFocus = 1;
	_TextBox->Rect.w = _TextBox->Text->w;
	_TextBox->Rect.h = _TextBox->Text->h;
	ContainerPosChild((struct Container*)_Parent, (struct Widget*)_TextBox);
	_TextBox->Rect.x = 0;
	luaL_getmetafield(_State, 1, "Children");
	lua_pushvalue(_State, 3);

	lua_rawseti(_State, -2, lua_rawlen(_State, -2) + 1);
	lua_pop(_State, 1);
	return 1;
}

int LuaHorizontalContainer(lua_State* _State) {
	lua_newtable(_State);
	lua_getglobal(_State, "Widget");
	lua_settable(_State, -2);

	lua_pushstring(_State, "Id");
	lua_pushinteger(_State,  NextGUIId());
	lua_rawset(_State, -3);
	return 1;
}

int LuaVerticalContainer(lua_State* _State) {
	struct Container* _Container = LuaContainerPush(_State);

	_Container->Id = NextGUIId();
	_Container->Rect.x = luaL_checkint(_State, 1);
	_Container->Rect.y = luaL_checkint(_State, 2);
	_Container->Rect.w = luaL_checkint(_State, 3);
	_Container->Rect.h = luaL_checkint(_State, 4);
	if(lua_gettop(_State) > 5) {
		_Container->Parent = luaL_checkudata(_State, 5, "Widget");

		luaL_getmetafield(_State, -1, "Children");
		lua_pushvalue(_State, 6);

		lua_rawseti(_State, -3, lua_rawlen(_State, -2));
		lua_pop(_State, 2);
	} else {
		lua_getglobal(_State, "GUI");
		lua_pushstring(_State, "Screen");
		lua_pushvalue(_State, 5);
		lua_rawset(_State, -3);
		lua_pop(_State, 1);
		_Container->Parent = NULL;
	}
	_Container->Children = NULL;
	_Container->ChildrenSz = 0;
	_Container->CanFocus = 0;
	_Container->OnDraw = WidgetOnDraw;
	_Container->OnFocus = WidgetOnFocus;
	_Container->OnUnfocus = WidgetOnUnfocus;
	_Container->Spacing = 0;
	_Container->Margins.Top = 0;
	_Container->Margins.Left = 0;
	_Container->Margins.Right = 0;
	_Container->Margins.Bottom = 0;
	return 1;
}

int LuaBackgroundColor(lua_State* _State) {
	SDL_SetRenderDrawColor(g_Renderer, luaL_checkint(_State, 1), luaL_checkint(_State, 2), luaL_checkint(_State, 3), 255);
	return 0;
}

int LuaGetFont(lua_State* _State) {
	const char* _Name = luaL_checkstring(_State, 1);
	int _Size = luaL_checkint(_State, 2);
	TTF_Font* _Font = NULL;

	chdir("Fonts");
	_Font = TTF_OpenFont(_Name, _Size);
	lua_pushlightuserdata(_State, _Font);
	chdir("..");
	return 1;
}

int LuaDefaultFont(lua_State* _State) {
	luaL_checktype(_State, 1, LUA_TLIGHTUSERDATA);
	g_GUIDefs.Font = lua_touserdata(_State, 1);
	return 0;
}

int LuaSetMenu(lua_State* _State) {
	const char* _Name = luaL_checkstring(_State, 1);

	lua_getglobal(_State, _Name);
	if(lua_type(_State, -1) != LUA_TTABLE)
		return luaL_error(_State, "%s is not a table.", _Name);
	lua_pushstring(_State, "Init");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TFUNCTION || lua_iscfunction(_State, -1) != 0)
		return luaL_error(_State, "%s is not a function.", _Name);
	lua_pushinteger(_State, SDL_WIDTH);
	lua_pushinteger(_State, SDL_HEIGHT);
	if(LuaCallFunc(_State, 2, 0, 0) == 0)
		luaL_error(_State, "%s.Init function call failed", _Name);
	lua_pop(_State, 1);
	return 0;
}

int LuaSetColor(lua_State* _State) {
	int _Red = luaL_checkint(_State, 1);
	int _Blue = luaL_checkint(_State, 2);
	int _Green = luaL_checkint(_State, 3);

	if(_Red < 0 || _Red > 255)
		luaL_error(_State, "Red is not between 0 and 255");
	if(_Blue < 0 || _Blue > 255)
		luaL_error(_State, "Blue is not between 0 and 255");
	if(_Green < 0 || _Red > 255)
		luaL_error(_State, "Green is not between 0 and 255");
	g_GUIDefs.FontFocus.r = _Red;
	g_GUIDefs.FontFocus.b = _Blue;
	g_GUIDefs.FontFocus.g = _Green;
	return 0;
}

int LuaOnKey(lua_State* _State) {
	int _Key = SDLK_RETURN; //LuaStringToKey(_State, 2);
	int _KeyState = LuaKeyState(_State, 3);
	int _KeyMod = KMOD_NONE;
	SDL_Event _Event;

	luaL_argcheck(_State, (lua_isfunction(_State, 4) == 1 || lua_iscfunction(_State, 4) == 1), 4, "Is not a function");
	luaL_argcheck(_State, (_KeyState != -1), 3, "Is not a valid key state");
	_Event.type = SDL_KEYUP;
	_Event.key.state = _KeyState;
	_Event.key.keysym.sym = _Key;
	_Event.key.keysym.mod = _KeyMod;

	if(g_GUIEvents.Size == g_GUIEvents.TblSz) {
		g_GUIEvents.Events = realloc(g_GUIEvents.Events, sizeof(SDL_Event) * g_GUIEvents.TblSz * 2);
		g_GUIEvents.TblSz *= 2;
	}
	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "EventIds");
	lua_rawget(_State, -2);
	lua_pushvalue(_State, 4);
	lua_rawseti(_State, -2, luaL_ref(_State, -2));
	lua_pop(_State, 1);
	g_GUIEvents.Events[g_GUIEvents.Size++] = _Event;
	qsort(g_GUIEvents.Events, g_GUIEvents.Size, sizeof(SDL_Event), SDLEventCmp);
	return 0;
}

int LuaQuit(lua_State* _State) {
	g_GUIOk = 0;
	return 0;
}

struct Widget* LuaCheckWidget(lua_State* _State, int _Index) {
	struct Widget* _Widget = NULL;

	luaL_checktype(_State, _Index, LUA_TUSERDATA);
	if((_Widget = luaL_testudata(_State, _Index, "Widget")) == NULL)
		return (struct Widget*) LuaCheckClass(_State, _Index, "Widget");
	return _Widget;
}

struct Container* LuaCheckContainer(lua_State* _State, int _Index) {
	struct Container* _Container = NULL;

	luaL_checktype(_State, _Index, LUA_TUSERDATA);
	if((_Container = luaL_checkudata(_State, _Index, "Container")) == NULL)
		return (struct Container*) luaL_error(_State, LUA_TYPERROR(_State, 1, "Container", "CheckContainer"));
	return _Container;
}

struct TextBox* LuaCheckTextBox(lua_State* _State, int _Index) {
	struct TextBox* _TextBox = NULL;

	luaL_checktype(_State, _Index, LUA_TUSERDATA);
	if((_TextBox = luaL_checkudata(_State, _Index, "TextBox")) == NULL)
		return (struct TextBox*) luaL_error(_State, LUA_TYPERROR(_State, 1, "TextBox", "CheckTextBox"));
	return _TextBox;
}

SDL_Surface* LuaCheckSurface(lua_State* _State, int _Index) {
	SDL_Surface* _Surface = NULL;

	luaL_checktype(_State, _Index, LUA_TUSERDATA);
	if((_Surface = luaL_checkudata(_State, _Index, "Surface")) == NULL)
		return (struct SDL_Surface*) luaL_error(_State, LUA_TYPERROR(_State, 1, "Surface", "CheckSurface"));
	return _Surface;
}

int LuaWidgetId(lua_State* _State) {
	struct Widget* _Widget = LuaCheckWidget(_State, 1);

	lua_pushinteger(_State, _Widget->Id);
	return 1;
}

int LuaWidgetGetX(lua_State* _State) {
	struct Widget* _Widget = LuaCheckWidget(_State, 1);

	lua_pushinteger(_State, _Widget->Rect.y);
	return 1;
}

int LuaWidgetGetY(lua_State* _State) {
	struct Widget* _Widget = LuaCheckWidget(_State, 1);

	lua_pushinteger(_State, _Widget->Rect.y);
	return 1;
}

int LuaWidgetGetWidth(lua_State* _State) {
	struct Widget* _Widget = LuaCheckWidget(_State, 1);

	lua_pushinteger(_State, _Widget->Rect.w);
	return 1;
}

int LuaWidgetGetHeight(lua_State* _State) {
	struct Widget* _Widget = LuaCheckWidget(_State, 1);

	lua_pushinteger(_State, _Widget->Rect.h);
	return 1;
}

int LuaWidgetGetParent(lua_State* _State) {
	struct Widget* _Widget = LuaCheckWidget(_State, 1);
	struct Widget* _Parent = LuaWidgetPush(_State);

	_Parent->Id = _Widget->Parent->Id;
	_Parent->Rect = _Widget->Parent->Rect;
	_Parent->Children = _Widget->Parent->Children;
	_Parent->OnDraw = _Widget->Parent->OnDraw;
	return 1;
}

int LuaWidgetGetChildren(lua_State* _State) {
	struct Widget* _Widget = LuaCheckWidget(_State, 1);
	int i;

	lua_newtable(_State);
	for(i = 0; _Widget->Children[i] != NULL; ++i) {
		lua_pushlightuserdata(_State, _Widget->Children[i]);
		lua_rawseti(_State, -2, i);
	}
	return 1;
}

int LuaContainerGetSpacing(lua_State* _State) {
	struct Container* _Container = LuaCheckContainer(_State, 1);

	lua_pushinteger(_State, _Container->Spacing);
	return 1;
}

int LuaContainerGetMargins(lua_State* _State) {
	struct Container* _Container = LuaCheckContainer(_State, 1);

	lua_createtable(_State, 4, 0);
	lua_rawseti(_State, -1, _Container->Margins.Top);
	lua_rawseti(_State, -1, _Container->Margins.Left);
	lua_rawseti(_State, -1, _Container->Margins.Right);
	lua_rawseti(_State, -1, _Container->Margins.Bottom);
	return 1;
}

int LuaTextBoxSetText(lua_State* _State) {
	struct TextBox* _TextBox = LuaCheckTextBox(_State, 1);
	SDL_Surface* _Surface = LuaCheckSurface(_State, 2);

	_TextBox->SetText((struct Widget*)_TextBox, _Surface);
	return 0;
}

int LoadGUILua(lua_State* _State) {
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;
	struct Container* _Screen = NULL;

	if(LuaRegisterWidget(_State) == 0 ||
			LuaRegisterContainer(_State) == 0 ||
			LuaRegisterTextBox(_State) == 0 ||
			LuaRegisterSurface(_State) == 0)
		return 0;
	luaL_newlib(_State, g_LuaFuncsGUI);
	lua_setglobal(_State, "GUI");
	if(LuaLoadFile(_State, "data/video.lua") != LUA_OK)
		return 0;
	chdir("data/gui");
	_Dir = opendir("./");
	while((_Dirent = readdir(_Dir)) != NULL) {
		if(!strcmp(_Dirent->d_name, ".") || !strcmp(_Dirent->d_name, ".."))
			continue;
		if(LuaLoadFile(_State, _Dirent->d_name) != LUA_OK)
			goto error;
	}
	chdir("../..");;
	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Screen");
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "EventIds");
	lua_createtable(_State, g_GUIEvents.TblSz, 0);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Init");
	lua_rawget(_State, -2);
	if(LuaCallFunc(_State, 0, 0, 0) == 0)
		goto error;
	closedir(_Dir);
	lua_pop(_State, 1);
	_Screen = GetScreen(_State);
	g_Focus.Parent = (struct Widget*)_Screen;
	g_Focus.Index = 0;
	if(_Screen->ChildrenSz > 0)
		_Screen->Children[0]->OnFocus(_Screen->Children[0]);
	return 1;
	error:
	lua_pop(_State, 1);
	return 0;
}

struct Container* GetScreen(lua_State* _State) {
	struct Container* _Screen = NULL;

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Screen");
	lua_rawget(_State, -2);
	_Screen = lua_touserdata(_State, -1);
	lua_pop(_State, 2);
	return _Screen;
}

int LuaKeyState(lua_State* _State, int _Index) {
	const char* _Type = NULL;

	if(lua_type(_State, _Index) != LUA_TSTRING)
		return -1;
	_Type = lua_tostring(_State, _Index);
	if(strcmp(_Type, "Released") == 0)
		return SDL_RELEASED;
	else if(strcmp(_Type, "Pressed") == 0)
		return SDL_PRESSED;
	return -1;
}

void LuaCallEvent(lua_State* _State, int _EvntIndx) {
	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "EventIds");
	lua_rawget(_State, -2);
	lua_rawgeti(_State, -1, _EvntIndx + 1);
	LuaCallFunc(_State, 0, 0, 0);
	lua_pop(_State, 2);
}
