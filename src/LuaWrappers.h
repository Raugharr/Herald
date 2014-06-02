/*
 * File: LuaWrappers.h
 * Author: David Brotz
 */

#ifndef __LUAWRAPPERS_H
#define __LUAWRAPPERS_H

struct Constraint;
typedef struct lua_State lua_State;

struct Constraint* ConstraintFromLua(lua_State* _State, int _Index);

#endif
