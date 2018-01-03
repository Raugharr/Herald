/*
 * File: Grammar.c
 * Author: David Brotz
 */

#include "Grammar.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

int LuaLoadGramSet(lua_State* State) {
	struct GramRl* Head = NULL;
	struct GramRl* Prev = NULL;
	struct GramRl* Rule = NULL;

	if(lua_type(State, 1) != LUA_TTABLE) return NULL;
	lua_pushnil(State);
	if(lua_next(State, 1) == 0) return 0;
	Head = LuaLoadGramRule(State, -1);
	Prev = Head;
	lua_pop(State, 1);

	while(lua_next(State, 1) != 0) {
		Rule = LuaLoadGramRule(State, -1);
		ArrayInsert(&Prev->NextRules, Rule);
		Prev = Rule;
		lua_pop(State, 1);
	}
//	return Head;
	return 0;
}

struct GramRl* LuaLoadGramRule(lua_State* State, int Idx) {
	struct GramRl* Rule = NULL;
	char* Symbol = NULL;
	const char* Tmp = NULL;

	Idx = lua_absindex(State, Idx);
	if(lua_type(State, Idx) != LUA_TTABLE) return NULL;
	Rule = malloc(sizeof(struct GramRl));
	CtorArray(&Rule->Symbols, 4);
	CtorArray(&Rule->NextRules, 4);
	lua_pushnil(State);
	while(lua_next(State, Idx) != 0) {
		if(lua_isstring(State, -1) == 0) goto end;
		Tmp = lua_tostring(State, -1);
		Symbol = calloc(strlen(Tmp) + 1, sizeof(char*));
		strcpy(Symbol, Tmp);
		ArrayInsert_S(&Rule->Symbols, Symbol);
		end:
		lua_pop(State, 1);
	}
	
	return Rule;	
}

