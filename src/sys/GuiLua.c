/*
 * File: LuaLib.c
 * Author: David Brotz
 */

#include "GuiLua.h"

#include "Video.h"
#include "LuaHelper.h"
#include "Array.h"

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
		{"CloseMenu", LuaCloseMenu},
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

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Widget");
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

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Container");
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

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "TextBox");
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

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Surface");
	lua_rawset(_State, -3);
	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_setglobal(_State, "Surface");
	return 1;
}

int LuaRegisterFont(lua_State* _State) {
	if(luaL_newmetatable(_State, "Font") == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Font");
	lua_rawset(_State, -2);
	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_setglobal(_State, "Font");
	return 1;
}

TTF_Font* LuaCreateFont(lua_State* _State, const char* _Name, int _Size) {
	TTF_Font* _Font = NULL;

	chdir("Fonts");
	_Font = lua_newuserdata(_State, sizeof(TTF_Font*));
	_Font = TTF_OpenFont(_Name, _Size);
	luaL_getmetatable(_State, "Font");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "Name");
	lua_pushstring(_State, _Name);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "Size");
	lua_pushinteger(_State, _Size);
	lua_rawset(_State, -3);
	chdir("..");
	return NULL;
}

int LuaCreateTextBox(lua_State* _State) {
	struct TextBox* _TextBox = NULL;
	struct Container* _Parent = LuaCheckContainer(_State, 1);
	const char* _Text = luaL_checkstring(_State, 2);
	SDL_Rect _Rect;
	SDL_Surface* _Surface = NULL;
	if(g_GUIDefs.Font == NULL)
		return luaL_error(_State, "Default font is NULL");
	_Surface = TTF_RenderText_Solid(g_GUIDefs.Font, _Text, g_GUIDefs.FontUnfocus);
	_Rect.x = 0;
	_Rect.y = 0;
	_Rect.w = _Surface->w;
	_Rect.h = _Surface->h;
	lua_newtable(_State);
	_TextBox = CreateTextBox();
	ConstructTextBox(_TextBox, _Parent, &_Rect, _State, _Surface);
	lua_getglobal(_State, "TextBox");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _TextBox);
	lua_rawset(_State, -3);
	return 1;
}

struct Container* LuaContainer(lua_State* _State) {
	int i = 0;
	struct Container* _Container = NULL;
	SDL_Rect _Rect = {luaL_checkint(_State, 1), luaL_checkint(_State, 2), luaL_checkint(_State, 3), luaL_checkint(_State, 4)};
	struct Margin _Margins;

	luaL_checktype(_State, 6, LUA_TTABLE);
	lua_pushnil(_State);
	while(lua_next(_State, 6) != 0 && i <= 4) {
		((int*)&_Margins)[i] = lua_tointeger(_State, 1);
		lua_pop(_State, 1);
		++i;
	}
	luaL_argcheck(_State, (i == 4), 6, "Table requires 4 elements.");
	lua_newtable(_State);
	_Container = CreateContainer();
	if(lua_gettop(_State) > 7) {
		ConstructContainer(_Container, LuaCheckClass(_State, 7, "Widget"), &_Rect, _State, luaL_checkint(_State, 5), &_Margins);
	} else
		ConstructContainer(_Container, NULL, &_Rect, _State, luaL_checkint(_State, 5), &_Margins);
	lua_getglobal(_State, "Container");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Container);
	lua_rawset(_State, -3);

	/* NOTE: Is this if statement needed? */
	if(_Container->Parent == NULL) {
		struct Container* _Screen = GetScreen(_State);

		if(_Screen != NULL)
			_Screen->OnDestroy((struct Widget*)_Screen);
		lua_getglobal(_State, "GUI");
		lua_pushstring(_State, "Screen");
		lua_pushvalue(_State, -3);
		lua_rawset(_State, -3);
		lua_pop(_State, 1);
	}
	return _Container;
}

int LuaHorizontalContainer(lua_State* _State) {
	struct Container* _Container = LuaContainer(_State);

	_Container->NewChild = HorzConNewChild;
	return 1;
}

int LuaVerticalContainer(lua_State* _State) {
	struct Container* _Container = LuaContainer(_State);

	_Container->NewChild = VertConNewChild;
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

	if(GetScreen(_State) != NULL)
		LuaCloseMenu(_State);
	g_Focus.Parent = NULL;
	g_Focus.Index = 0;
	g_Focus.Id = 0;
	lua_getglobal(_State, _Name);
	if(lua_type(_State, -1) != LUA_TTABLE)
		return luaL_error(_State, "%s is not a table.", _Name);
	lua_pushstring(_State, "Init");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TFUNCTION || lua_iscfunction(_State, -1) != 0)
		return luaL_error(_State, "%s is not a function.", _Name);
	lua_pushinteger(_State, SDL_WIDTH);
	lua_pushinteger(_State, SDL_HEIGHT);
	if(LuaCallFunc(_State, 2, 1, 0) == 0)
		luaL_error(_State, "%s.Init function call failed", _Name);
	if(lua_type(_State, -1) != LUA_TBOOLEAN)
		luaL_error(_State, "%s.Init function did not return a boolean", _Name);
	if(lua_toboolean(_State, -1) > 0) {
		lua_getglobal(_State, "GUI");
		lua_pushstring(_State, "ScreenStack");
		lua_rawget(_State, -2);
		lua_getglobal(_State, _Name);
		luaL_ref(_State, -2);
		lua_pop(_State, 2);
	}
	lua_pop(_State, 2);
	g_GUIOk = 1;
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
	struct Widget* _Widget = LuaCheckWidget(_State, 1);
	int _Key = SDLK_RETURN; //LuaStringToKey(_State, 2);
	int _KeyState = LuaKeyState(_State, 3);
	int _KeyMod = KMOD_NONE;
	SDL_Event _Event;
	struct WEvent _WEvent;

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
	_WEvent.Event = _Event;
	_WEvent.WidgetId = _Widget->Id;
	g_GUIEvents.Events[g_GUIEvents.Size++] = _WEvent;
	//qsort(g_GUIEvents.Events, g_GUIEvents.Size, sizeof(struct WEvent), SDLEventCmp);
	return 0;
}

int LuaCloseMenu(lua_State* _State) {
	struct Container* _Container = GetScreen(_State);
	int _Len = 0;

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "ScreenStack");
	lua_rawget(_State, -2);
	_Len = lua_rawlen(_State, -1);
	lua_pop(_State, 1);
	if(_Len > 0) {
		luaL_unref(_State, -1, _Len);
		if(_Len == 1) {
			g_GUIOk = 0;
		} else {
			lua_pushstring(_State, "Screen");
			lua_pushstring(_State, "ScreenStack");
			lua_rawget(_State, -3);
			lua_rawset(_State, -3);
		}
	} else
		g_GUIOk = 0;
	lua_pushstring(_State, "Screen");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_pop(_State, 1);
	if(_Container != NULL)
		_Container->OnDestroy((struct Widget*)_Container);
	return 0;
}

struct Widget* LuaCheckWidget(lua_State* _State, int _Index) {
	struct Widget* _Widget = NULL;

	if((_Widget = LuaTestClass(_State, _Index, "Widget")) == NULL)
		return (struct Widget*) LuaCheckClass(_State, _Index, "Widget");
	return _Widget;
}

struct Container* LuaCheckContainer(lua_State* _State, int _Index) {
	struct Container* _Container = NULL;

	if((_Container = LuaTestClass(_State, _Index, "Container")) == NULL)
		return (struct Container*) LuaCheckClass(_State, _Index, "Container");
	return _Container;
}

struct TextBox* LuaCheckTextBox(lua_State* _State, int _Index) {
	struct TextBox* _TextBox = NULL;

	if((_TextBox = LuaTestClass(_State, _Index, "TextBox")) == NULL)
		return (struct TextBox*) LuaCheckClass(_State, _Index, "TextBox");
	return _TextBox;
}

SDL_Surface* LuaCheckSurface(lua_State* _State, int _Index) {
	SDL_Surface* _Surface = NULL;

	if((_Surface = LuaTestClass(_State, _Index, "Surface")) == NULL)
		return (SDL_Surface*) LuaCheckClass(_State, _Index, "SDL_Surface");
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

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Widgets");
	lua_rawget(_State, -2);
	lua_rawgeti(_State, -1, _Widget->Parent->LuaRef);
	lua_insert(_State, -2);
	lua_pop(_State, -2);
	return 1;
}

int LuaWidgetGetChildren(lua_State* _State) {
	struct Container* _Container = LuaCheckContainer(_State, 1);
	int i;

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Widgets");
	lua_newtable(_State);
	for(i = 0; _Container->Children[i] != NULL; ++i) {
		lua_rawgeti(_State, -2, _Container->Children[i]->LuaRef);
		lua_rawseti(_State, -2, i);
	}
	lua_insert(_State, -2);
	lua_pop(_State, 2);
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

int InitGUILua(lua_State* _State) {
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;

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

	lua_pushstring(_State, "Widgets");
	lua_newtable(_State);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "ScreenStack");
	lua_newtable(_State);
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
	return 1;
	error:
	lua_pop(_State, 1);
	return 0;
}

int QuitGUILua(lua_State* _State) {
	struct Container* _Screen = GetScreen(_State);

	if(_Screen != NULL)
		_Screen->OnDestroy((struct Widget*)_Screen);
	return 1;
}

struct Container* GetScreen(lua_State* _State) {
	struct Container* _Screen = NULL;

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Screen");
	lua_rawget(_State, -2);
	_Screen = LuaToClass(_State, -1);
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
	lua_rawgeti(_State, -1, _EvntIndx);
	LuaCallFunc(_State, 0, 0, 0);
	lua_pop(_State, 2);
}

int LuaWidgetRef(lua_State* _State) {
	int _Ref = 0;

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Widgets");
	lua_rawget(_State, -2);
	lua_pushvalue(_State, -3);
	_Ref = luaL_ref(_State, -2);
	lua_pop(_State, 2);
	return _Ref;
}

void LuaWidgetUnref(lua_State* _State, int _Ref) {
	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Widgets");
	lua_rawget(_State, -2);
	luaL_unref(_State, -1, _Ref);
	lua_pop(_State, 1);
}
