/*
 * File: VideoLua.h
 * Author: David Brotz
 */
#ifndef __VIDEOLUA_H
#define __VIDEOLUA_H

typedef struct lua_State lua_State;

void InitVideoLua(lua_State* _State);

int LuaCreateSprite(lua_State* _State);

int LuaCreateAnimation(lua_State* _State);
void AnimationLoad(lua_State* _State, const char* _AnimFile);
int LuaAnimationSetImage(lua_State* _State);
int LuaAnimationCreateFrames(lua_State* _State);
int LuaAnimationSetSpeed(lua_State* _State);

#endif
