/*
 * File: LuaLib.c
 * Author: David Brotz
 */

#include "GuiLua.h"

#include "Video.h"
#include "LuaHelper.h"
#include "Array.h"

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

static const luaL_Reg g_LuaFuncsGUI[] = {
		{"HorizontalContainer", LuaHorizontalContainer},
		{"VerticalContainer", LuaVerticalContainer},
		{"BackgroundColor", LuaBackgroundColor},
		{"GetFont", LuaGetFont},
		{"SetFont", LuaDefaultFont},
		{"DefaultFont", LuaDefaultFont},
		{"SetMenu", LuaSetMenu},
		{"SetFocusColor", LuaSetFocusColor},
		{"SetUnfocusColor", LuaSetUnfocusColor},
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
		{"SetChild", LuaContainerSetChild},
		{"GetChildCt", LuaContainerGetChildCt},
		{"Spacing", LuaContainerGetSpacing},
		{"Margins", LuaContainerGetMargins},
		{"CreateTextBox", LuaCreateTextBox},
		{"CreateTable", LuaCreateTable},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsTextBox[] = {
		{"SetText", LuaTextBoxSetText},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsTable[] = {
		{"GetCellIndex", LuaTableGetCellIndex},
		{"GetFont", LuaTableGetFont},
		{"SetCellWidth", LuaTableSetCellWidth},
		{"SetCellHeight", LuaTableSetCellHeight},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsFont[] = {
		{"FontWidth", LuaFontWidth},
		{"FontHeight", LuaFontHeight},
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

int LuaRegisterTable(lua_State* _State) {
	lua_getglobal(_State, "Container");
	if(luaL_newmetatable(_State, "Table") == 0) {
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
	lua_pushstring(_State, "Table");
	lua_rawset(_State, -3);
	lua_pushliteral(_State, "__baseclass");
	lua_getglobal(_State, "Container");
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsTable, 0);
	lua_setglobal(_State, "Table");
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
	lua_rawset(_State, -3);
	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsFont, 0);
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
	struct Font* _Font = g_GUIDefs.Font;
	const char* _Name = NULL;
	int _Size = 0;

	if(lua_gettop(_State) >= 4) {
		_Name = luaL_checkstring(_State, 3);
		_Size = luaL_checkinteger(_State, 4);
		_Font = g_GUIFonts;
		while(_Font != NULL) {
			if(strcmp(_Font->Name, _Name) == 0 && _Font->Size == _Size)
				break;
			_Font = _Font->Next;
		}
	}
	_Surface = ConvertSurface(TTF_RenderText_Solid(_Font->Font, _Text, g_GUIDefs.FontUnfocus));
	_Rect.x = _Parent->Rect.x;
	_Rect.y = _Parent->Rect.y;
	_Rect.w = _Surface->w;
	_Rect.h = _Surface->h;
	lua_newtable(_State);
	_TextBox = CreateTextBox();
	ConstructTextBox(_TextBox, _Parent, &_Rect, _State, _Surface, _Font);
	lua_getglobal(_State, "TextBox");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _TextBox);
	lua_rawset(_State, -3);
	return 1;
}

int LuaCreateTable(lua_State* _State) {
	int i = 0;
	int _Rows = luaL_checkinteger(_State, 2);
	int _Columns = luaL_checkinteger(_State, 3);
	int _Spacing = luaL_checkinteger(_State, 4);
	struct Margin _Margins;
	struct Container* _Parent = LuaCheckContainer(_State, 1);
	struct Table* _Table = NULL;
	struct Font* _Font = g_GUIDefs.Font;
	SDL_Rect _Rect = {0, 0, 0, 0};

	luaL_checktype(_State, 5, LUA_TTABLE);
	if(lua_gettop(_State) == 6)
		_Font = LuaCheckFont(_State, 6);
	lua_pushnil(_State);
	while(lua_next(_State, 5) != 0 && i < 4) {
		((int*)&_Margins)[i] = lua_tointeger(_State, 1);
		lua_pop(_State, 1);
		++i;
	}
	_Table = CreateTable();
	if(_Font == NULL)
		luaL_error(_State, "No default font or font passed as argument.");
	ConstructTable(_Table, _Parent, &_Rect,_State, _Spacing, &_Margins, _Columns, _Rows, _Font);
	lua_newtable(_State);
	lua_getglobal(_State, "Table");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Table);
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
		ConstructContainer(_Container, LuaCheckClass(_State, 7, "Container"), &_Rect, _State, luaL_checkint(_State, 5), &_Margins);
	} else
		ConstructContainer(_Container, NULL, &_Rect, _State, luaL_checkint(_State, 5), &_Margins);
	lua_getglobal(_State, "Container");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Container);
	lua_rawset(_State, -3);

	/* NOTE: Can this if statement be removed by
	 * having more sound logic? */
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
	g_GUIDefs.Background.r = luaL_checkint(_State, 1);
	g_GUIDefs.Background.g = luaL_checkint(_State, 2);
	g_GUIDefs.Background.b = luaL_checkint(_State, 3);
	return 0;
}

int LuaGetFont(lua_State* _State) {
	const char* _Name = luaL_checkstring(_State, 1);
	int _Size = luaL_checkint(_State, 2);
	struct Font* _Font = g_GUIFonts;

	chdir("fonts");
	while(_Font != NULL) {
		if(strcmp(_Font->Name, _Name) == 0 && _Font->Size == _Size)
			goto no_new_font;
		_Font = _Font->Next;
	}
		_Font = CreateFont(_Name, _Size);
	no_new_font:
	lua_newtable(_State);
	lua_getglobal(_State, "Font");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Font);
	lua_rawset(_State, -3);
	chdir("..");
	return 1;
}

int LuaDefaultFont(lua_State* _State) {
	if(g_GUIDefs.Font != NULL) {
		g_GUIDefs.Font->RefCt = -1;
		DestroyFont(g_GUIDefs.Font);
	}
	g_GUIDefs.Font = LuaCheckFont(_State, 1);
	return 0;
}

int LuaSetMenu(lua_State* _State) {
	const char* _Name = luaL_checkstring(_State, 1);
	struct Font* _Font = NULL;
	struct Font* _Prev = NULL;

	lua_getglobal(_State, _Name);
	if(lua_type(_State, -1) == LUA_TNIL)
		luaL_error(_State, "Menu %s not not exist", _Name);
	if(GetScreen(_State) != NULL)
		LuaCloseMenu(_State);
	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Menu");
	lua_pushvalue(_State, -3);
	lua_rawset(_State, -3);
	lua_pop(_State, 1);
	lua_pushstring(_State, "__name");
	lua_pushstring(_State, _Name);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "__savestate");
	lua_rawget(_State, -2);
	/* Is __savestate == 0 */
	if(lua_toboolean(_State, -1) == 0) {
		g_Focus = CreateGUIFocus();
		g_GUIEvents = CreateGUIEvents();
		_Font = g_GUIDefs.Font;
		while(_Font != NULL) {
			_Prev = _Font;
			_Font = _Font->Next;
			/*NOTE: widgets might stay alive when another menu is brought up,
		 	* we should only delete fonts to widgets that are dead.
		 	*/
			if(_Prev != g_GUIDefs.Font)
				DestroyFont(_Prev);
		}
	} else {
		/* Restore the previous menu then return. */
		lua_getglobal(_State, "GUI");
		lua_pushstring(_State, "ScreenStack");
		lua_rawget(_State, -2);
		lua_pushnil(_State);
		while(lua_next(_State, -2) != 0) {
			if(lua_type(_State, -1) != LUA_TTABLE) {
				lua_pop(_State, 1);
				continue;
			}
			lua_pushstring(_State, "__name");
			lua_rawget(_State, -2);

			if(strcmp(lua_tostring(_State, -1), _Name) == 0) {
				int i;
				/* Set GUI.Screen to the iterating table's value's __screen. */
				lua_getglobal(_State, "GUI");
				lua_pushstring(_State, "Screen");
				lua_pushstring(_State, "__screen");
				lua_rawget(_State, -5);
				lua_rawset(_State, -3);
				lua_pop(_State, 1);

				lua_pushstring(_State, "__focus");
				lua_rawget(_State, -3);
				g_Focus = lua_touserdata(_State, -1);
				lua_pushstring(_State, "__events");
				lua_rawget(_State, -4);
				g_GUIEvents = lua_touserdata(_State, -1);
				lua_getglobal(_State, "GUI");
				lua_pushstring(_State, "EventIds");
				lua_rawget(_State, -2);
				lua_remove(_State, -2);
				lua_pushstring(_State, "__ehooks");
				lua_rawget(_State, -6);
				lua_pushnil(_State);
				i = 0;
				while(lua_next(_State, -2) != 0) {
					g_GUIEvents->Events[i++].RefId = luaL_ref(_State, -4);
					_Top = lua_gettop(_State);
				}
					//luaL_unref(_State, -1, g_GUIEvents->Events[i].RefId);
				lua_pop(_State, 4);
				_Top = lua_gettop(_State);

				lua_pushstring(_State, "__refid");
				lua_rawget(_State, -5);
				luaL_unref(_State, -6, lua_tointeger(_State, -1));
				lua_pop(_State, 1);
				lua_pop(_State, 2);
				break;
			}
			lua_pop(_State, 2);
		}
		lua_pop(_State, 5);
		g_GUIOk = 1;
		g_GUIMenuChange = 1;
		return 0;
	}
	lua_pop(_State, 1);
	if(lua_type(_State, -1) != LUA_TTABLE)
		return luaL_error(_State, "%s is not a table.", _Name);
	lua_pushstring(_State, "Init");
	lua_rawget(_State, -2);
	lua_remove(_State, -2);
	if(lua_type(_State, -1) != LUA_TFUNCTION || lua_iscfunction(_State, -1) != 0)
		return luaL_error(_State, "%s is not a function.", _Name);
	lua_pushinteger(_State, SDL_WIDTH);
	lua_pushinteger(_State, SDL_HEIGHT);
	if(LuaCallFunc(_State, 2, 1, 0) == 0)
		luaL_error(_State, "%s.Init function call failed", _Name);
	if(lua_type(_State, -1) != LUA_TBOOLEAN)
		luaL_error(_State, "%s.Init function did not return a boolean", _Name);
	if(lua_toboolean(_State, -1) > 0) {
		int _Ref = 0;

		lua_getglobal(_State, _Name);
		lua_pushstring(_State, "__savestate");
		lua_pushboolean(_State, 1);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "__events");
		lua_pushlightuserdata(_State, g_GUIEvents);
		lua_rawset(_State, -3);

		lua_getglobal(_State, "GUI");
		lua_pushstring(_State, "ScreenStack");
		lua_rawget(_State, -2);
		lua_pushvalue(_State, -3); /* push global _Name onto stack. */
		lua_pushvalue(_State, -1); /* push global _Name again for setting __refid. */
		_Ref = luaL_ref(_State, -3);
		lua_pushstring(_State, "__refid");
		lua_pushinteger(_State, _Ref);
		lua_rawset(_State, -3);
		lua_pop(_State, 5);
	}
	g_Focus->Parent = GetScreen(_State);
	g_Focus->Index = 0;
	if(g_Focus->Parent->Children[0] != NULL) {
		g_Focus->Id = g_Focus->Parent->Children[0]->Id;
		g_Focus->Parent->Children[0]->OnFocus(g_Focus->Parent->Children[0]);
	} else {
		g_Focus->Parent->Children[0]->Id = -1;
	}
	lua_pop(_State, 2);
	g_GUIOk = 1;
	return 0;
}

void LuaSetColor(lua_State* _State, unsigned char* _RedPtr, unsigned char* _GreenPtr, unsigned char* _BluePtr) {
	int _Red = luaL_checkint(_State, 1);
	int _Blue = luaL_checkint(_State, 2);
	int _Green = luaL_checkint(_State, 3);

	if(_Red < 0 || _Red > 255)
		luaL_error(_State, "Red is not between 0 and 255");
	if(_Blue < 0 || _Blue > 255)
		luaL_error(_State, "Blue is not between 0 and 255");
	if(_Green < 0 || _Red > 255)
		luaL_error(_State, "Green is not between 0 and 255");
	*_RedPtr = _Red;
	*_GreenPtr = _Blue;
	*_BluePtr = _Green;
}

int LuaSetFocusColor(lua_State* _State) {
	LuaSetColor(_State, &g_GUIDefs.FontFocus.r, &g_GUIDefs.FontFocus.g, &g_GUIDefs.FontFocus.b);
	return 0;
}

int LuaSetUnfocusColor(lua_State* _State) {
	LuaSetColor(_State, &g_GUIDefs.FontUnfocus.r, &g_GUIDefs.FontUnfocus.g, &g_GUIDefs.FontUnfocus.b);
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

	if(g_GUIEvents->Size == g_GUIEvents->TblSz) {
		g_GUIEvents->Events = realloc(g_GUIEvents->Events, sizeof(SDL_Event) * g_GUIEvents->TblSz * 2);
		g_GUIEvents->TblSz *= 2;
	}
	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "EventIds");
	lua_rawget(_State, -2);
	lua_pushvalue(_State, 4);
	_WEvent.Event = _Event;
	_WEvent.WidgetId = _Widget->Id;
	_WEvent.RefId = luaL_ref(_State, -2);
	lua_rawseti(_State, -2, _WEvent.RefId);
	lua_pop(_State, 1);
	g_GUIEvents->Events[g_GUIEvents->Size++] = _WEvent;
	return 0;
}

int LuaCloseMenu(lua_State* _State) {
	int i;
	struct Container* _Container = GetScreen(_State);
	int _Len = 0;

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Menu");
	lua_rawget(_State, -2);
	lua_pushstring(_State, "__name");
	lua_rawget(_State, -2);
	lua_getglobal(_State, lua_tostring(_State, -1));
	lua_pushstring(_State, "__savestate");
	lua_rawget(_State, -2);
	if(lua_toboolean(_State, -1) == 0) {
		lua_pop(_State, 4);
		goto destroy;
	}
	lua_pushstring(_State, "__saved");
	lua_rawget(_State, -3);
	if(lua_toboolean(_State, -1) == 1) {
		lua_pop(_State, 5);
		goto no_destroy;
	}

	lua_pop(_State, 5);

	lua_pushstring(_State, "ScreenStack");
	lua_rawget(_State, -2);
	_Len = lua_rawlen(_State, -1);
	if(_Len > 0) {
		lua_rawgeti(_State, -1, _Len);
		lua_pushstring(_State, "__saved");
		lua_pushboolean(_State, 1);
		lua_rawset(_State, -3);
		lua_pushstring(_State, "__ehooks");
		lua_newtable(_State);
		lua_pushstring(_State, "EventIds");
		lua_rawget(_State, -6); /* -6 is global table "GUI" */
		for(i = 0; i < g_GUIEvents->Size; ++i) {
			int _Ref = g_GUIEvents->Events[i].RefId;

			lua_pushinteger(_State, i + 1);
			lua_rawgeti(_State, -2, _Ref);
			lua_rawset(_State, -4);
			g_GUIEvents->Events[i].RefId = -1;
			lua_pushnil(_State);
			lua_rawseti(_State, -2, _Ref);
		}
		lua_pop(_State, 1);
		lua_rawset(_State, -3);
		/* unref the Menu from ScreenStack. */
		//luaL_unref(_State, -2, _Ref);
		lua_pushstring(_State, "__events");
		lua_pushlightuserdata(_State, g_GUIEvents);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "__focus");
		lua_pushlightuserdata(_State, g_Focus);
		lua_rawset(_State, -3);
		/* Get the current menu(GUI["ScreenStack"].Len) and assign to the table's __screen field the value of GUI["Screen"]. GUI["ScreenStack"].(#GUI["ScreenStack"].__screen = GUI["Screen"]) */
		lua_pushstring(_State, "__screen");
		lua_getglobal(_State, "GUI");
		lua_pushstring(_State, "Screen");
		lua_rawget(_State, -2);
		lua_remove(_State, -2);
		lua_rawset(_State, -3);
		lua_pop(_State, 2);

		if(_Len == 0)
			g_GUIOk = 0;
		goto no_destroy;
	} else {
		g_GUIOk = 0;
		lua_pop(_State, 1);
	}
	destroy:
	lua_pushstring(_State, "EventIds");
	lua_rawget(_State, -2);
	for(i = 0; i < g_GUIEvents->Size; ++i) {
		luaL_unref(_State, -1, g_GUIEvents->Events[i].RefId);
	}
	lua_pop(_State, 1);
	if(_Container != NULL)
		_Container->OnDestroy((struct Widget*)_Container);
	DestroyFocus(g_Focus);
	DestroyGUIEvents(g_GUIEvents);
	no_destroy:
	g_Focus = NULL;
	g_GUIEvents = NULL;
	lua_pushstring(_State, "Screen");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_pop(_State, 1);
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

struct Table* LuaCheckTable(lua_State* _State, int _Index) {
	struct Table* _Table = NULL;

	if((_Table = LuaTestClass(_State, _Index, "Table")) == NULL)
		return (struct Table*) LuaCheckClass(_State, _Index, "Table");
	return _Table;
}

SDL_Surface* LuaCheckSurface(lua_State* _State, int _Index) {
	SDL_Surface* _Surface = NULL;

	if((_Surface = LuaTestClass(_State, _Index, "Surface")) == NULL)
		return (SDL_Surface*) LuaCheckClass(_State, _Index, "SDL_Surface");
	return _Surface;
}

struct Font* LuaCheckFont(lua_State* _State, int _Index) {
	struct Font* _Font = NULL;

	if((_Font = LuaTestClass(_State, _Index, "Font")) == NULL)
		return (struct Font*) LuaCheckClass(_State, _Index, "Font");
	return _Font;
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

int LuaContainerGetChild(lua_State* _State) {
	struct Container* _Container = LuaCheckContainer(_State, 1);
	int _Index = luaL_checkinteger(_State, 2);

	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "Widgets");
	lua_rawget(_State, -2);
	lua_rawgeti(_State, -1, _Container->Children[_Index]->LuaRef);
	lua_remove(_State, -2);
	return 1;
}

int LuaContainerSetChild(lua_State* _State) {
	struct Container* _Container = LuaCheckContainer(_State, 1);
	int _Index = luaL_checkinteger(_State, 2);
	struct Widget* _Child = LuaCheckWidget(_State, 3);

	luaL_argcheck(_State, (_Index >= 0 && _Index < _Container->ChildrenSz), 2, "Index is out of bounds.");
	if(_Container->Children[_Index] != NULL) {
		_Container->Children[_Index]->OnDestroy(_Container->Children[_Index]);
		_Container->Children[_Index] = _Child;
	} else {
		_Container->Children[_Index] = _Child;
		++_Container->ChildCt;
	}
	return 0;
}

int LuaContainerGetChildCt(lua_State* _State) {
	struct Container* _Container = LuaCheckContainer(_State, 1);

	lua_pushinteger(_State, _Container->ChildCt);
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

int LuaTableGetCellIndex(lua_State* _State) {
	struct Table* _Table = LuaCheckTable(_State, 1);
	int _Row = luaL_checkinteger(_State, 2);
	int _Col = luaL_checkinteger(_State, 3);

	lua_pushinteger(_State, _Row + _Col * _Table->Rows);
	return 1;
}

int LuaTableGetFont(lua_State* _State) {
	struct Table* _Table = LuaCheckTable(_State, 1);

	lua_newtable(_State);
	lua_getglobal(_State, "Font");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Table->Font);
	lua_rawset(_State, -3);
	return 1;
}

int LuaTableSetCellWidth(lua_State* _State) {
	struct Table* _Table = LuaCheckTable(_State, 1);

	_Table->CellMax.w = luaL_checkinteger(_State, 2);
	lua_pushvalue(_State, 1);
	return 1;
}

int LuaTableSetCellHeight(lua_State* _State) {
	struct Table* _Table = LuaCheckTable(_State, 1);

	_Table->CellMax.h = luaL_checkinteger(_State, 2);
	lua_pushvalue(_State, 1);
	return 1;
}

int LuaFontWidth(lua_State* _State) {
	struct Font* _Font = LuaCheckFont(_State, 1);
	int _Size = TTF_FontFaceIsFixedWidth(_Font->Font);

	if(_Size == 0)
		TTF_SizeText(_Font->Font, "w", &_Size, NULL);
	lua_pushinteger(_State, _Size);
	return 1;
}

int LuaFontHeight(lua_State* _State) {
	struct Font* _Font = LuaCheckFont(_State, 1);

	lua_pushinteger(_State, TTF_FontHeight(_Font->Font));
	return 1;
}

int InitGUILua(lua_State* _State) {
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;

	if(LuaRegisterWidget(_State) == 0 ||
			LuaRegisterContainer(_State) == 0 ||
			LuaRegisterTextBox(_State) == 0 ||
			LuaRegisterTable(_State) == 0 ||
			LuaRegisterSurface(_State) == 0 ||
			LuaRegisterFont(_State) == 0)
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
	lua_createtable(_State, LUA_EVENTIDSIZE, 0);
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
	struct Container* _Container = NULL;
	struct GUIEvents* _Events = NULL;

	/* Free all containers on the ScreenStack. */
	lua_getglobal(_State, "GUI");
	lua_pushstring(_State, "ScreenStack");
	lua_rawget(_State, -2);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		lua_pushstring(_State, "__screen");
		lua_rawget(_State, -2);
		_Container = LuaCheckContainer(_State, -1);
		_Container->OnDestroy((struct Widget*)_Container);
		lua_pushstring(_State, "__events");
		lua_rawget(_State, -3);
		_Events = (struct GUIEvents*) lua_touserdata(_State, -1);
		DestroyGUIEvents(_Events);
		lua_pop(_State, 3);
	}
	lua_pop(_State, 2);
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
	lua_pop(_State, 2);
}
