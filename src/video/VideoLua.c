/*
 * File: VideoLua.c
 * Author: David Brotz
 */

#include "VideoLua.h"

#include "Sprite.h"
#include "Animation.h"

#include "../sys/ResourceManager.h"
#include "../sys/LuaCore.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define ANIM_LUASTR ("AnimationLoading")
#define ANIM_MAXFRAMES (128)
#define ANIM_MAXKEYS (32)

struct AnimationLoading {
	int y;
	int DefaultSpeed;
	struct AnimationFrame Frames[ANIM_MAXFRAMES];
	int FrameSz;
	struct AnimationFrameKey Keys[ANIM_MAXKEYS];
	int KeySz;
	const char* ImageName;
};

static const luaL_Reg g_LuaFuncsSprite[] = {
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsAnimation[] = {
		{"CreateFrames", LuaAnimationCreateFrames},
		{"SetSpeed", LuaAnimationSetSpeed},
		{NULL, NULL}
};

static const struct LuaObjectReg g_VideoLuaObjects[] = {
		{"Sprite", NULL, g_LuaFuncsSprite},
		{"Animation", NULL, g_LuaFuncsAnimation},
		{NULL, NULL}
};

const luaL_Reg g_LuaVideoFuncs[] = {
		{"CreateSprite", LuaCreateSprite},
		{"CreateAnimation", LuaCreateAnimation},
		{NULL, NULL}
};

void InitVideoLua(lua_State* _State) {
	luaL_newlib(_State, g_LuaVideoFuncs);
	lua_setglobal(_State, "Video");
	RegisterLuaObjects(_State, g_VideoLuaObjects);
}

int LuaCreateSprite(lua_State* _State) {
	struct Sprite* _Sprite = NULL;
	const char* _File = luaL_checkstring(_State, 1);
	struct Resource* _Resource = ResourceGet(_File);
	SDL_Point _Pos = {0, 0};

	if(_Resource == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	_Sprite = CreateSprite(ResourceGetData(_Resource), 0, &_Pos);
	LuaCtor(_State, "Sprite", _Sprite);
	return 1;
}


int LuaCreateAnimation(lua_State* _State) {
	struct Sprite* _Sprite = NULL;
	const char* _File = luaL_checkstring(_State, 1);\
	struct Resource* _Resource = ResourceGet(_File);
	SDL_Point _Pos = {0, 0};

	_Sprite = CreateSprite(ResourceGetData(_Resource), 0, &_Pos);
	LuaCtor(_State, "Sprite", _Sprite);
	return 1;
}

void AnimationLoad(lua_State* _State, const char* _AnimFile) {
	struct AnimationLoading _Data;

	_Data.y = 0;
	_Data.DefaultSpeed = 20;
	_Data.FrameSz = 0;
	_Data.KeySz = 0;
	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, ANIM_LUASTR);
	lua_pushlightuserdata(_State, &_Data);
	lua_rawset(_State, -3);
	LuaLoadFile(_State, _AnimFile, "Animation");
	//luaL_loadbufferx(_State, _Resource->Data, _Resource->Size, "main", NULL);
	//lua_pcall(_State, 0, 0, 0);
}

int LuaAnimationCreateFrames(lua_State* _State) {
	int _Width = luaL_checkinteger(_State, 1);
	int _Height = luaL_checkinteger(_State, 2);
	int _FrameCt = luaL_checkinteger(_State, 3);
	const char* _Temp = luaL_checkstring(_State, 4);
	struct AnimationLoading* _Data = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, ANIM_LUASTR);
	lua_rawget(_State, -2);
	_Data = lua_touserdata(_State, -1);
	for(int i = _Data->FrameSz; i < _FrameCt && i < ANIM_MAXFRAMES; ++i) {
		_Data->Frames[i + _Data->FrameSz].Speed = _Data->DefaultSpeed;
		_Data->Frames[i + _Data->FrameSz].Rect.x = i * _Width;
		_Data->Frames[i + _Data->FrameSz].Rect.y = i * _Height;
		_Data->Frames[i + _Data->FrameSz].Rect.w = _Width;
		_Data->Frames[i + _Data->FrameSz].Rect.h = _Height;
		++_Data->FrameSz;
	}
	_Data->y = _Data->y + _Height;
	_Data->Keys[_Data->KeySz].Name = calloc(strlen(_Temp) + 1, sizeof(char));
	strcpy((char*)_Data->Keys[_Data->KeySz].Name, _Temp);
	++_Data->KeySz;
	return 0;
}

int LuaAnimationSetSpeed(lua_State* _State) {
	int _Speed = luaL_checkinteger(_State, 1);
	int _Frame = 0;
	struct AnimationLoading* _Data = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, ANIM_LUASTR);
	lua_rawget(_State, -2);
	_Data = lua_touserdata(_State, -1);
	lua_pop(_State, 2);
	if(lua_gettop(_State) > 1) {
		_Frame = luaL_checkinteger(_State, 2);
		_Data->Frames[_Frame].Speed = _Speed;
	} else {
		_Data->DefaultSpeed = _Speed;
	}
	return 0;
}
