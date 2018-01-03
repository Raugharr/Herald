/*
 * File: Grammar.h
 * Author: David Brotz
 */

#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include "sys/Array.h"

typedef struct lua_State lua_State;

struct GramRl {
	struct Array Symbols; //const char**
	struct Array NextRules; //struct GramRl**
};

int LuaLoadGramSet(lua_State* State);
struct GramRl* LuaLoadGramRule(lua_State* State, int Idx);

#endif

