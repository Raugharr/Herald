#include "Trait.h"

#include "Herald.h"

#include "sys/LinkedList.h"
#include "sys/HashTable.h"
#include "sys/LuaCore.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <lua/lua.h> 
#include <lua/lauxlib.h>
#include <string.h>

struct Trait* CreateTrait(const char* _Name) {
	struct Trait* _Trait = malloc(sizeof(struct Trait));

	_Trait->Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy((char* restrict) _Trait->Name, _Name);
	_Trait->Likes = NULL;
	_Trait->Dislikes = NULL;
	_Trait->Prevents = NULL;
	return _Trait;
}

void DestroyTrait(struct Trait* _Trait) {
	free((char*) _Trait->Name);
	free(_Trait->Likes);
	free(_Trait->Dislikes);
	free(_Trait->Prevents);
	free(_Trait);
}

struct Trait** TraitList(lua_State* _State, int _Index) {
	struct Trait** _TraitList = NULL;
	struct Trait* _Trait = NULL;
	struct LinkedList _List = {0, NULL, NULL};
	const char* _TraitName = NULL;

	_Index = lua_absindex(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, _Index) != 0) {
		if(LuaGetString(_State, -1, &_TraitName) == 0)
			goto loop_end;
		if((_Trait = HashSearch(&g_Traits, _TraitName)) == NULL) {
			luaL_error(_State, "%d is not a trait.", _TraitName);
		}
		LnkLstPushBack(&_List, _Trait);
		loop_end:
		lua_pop(_State, 1);
	}
	_TraitList = (struct Trait**) LnkLstToList(&_List);
	LnkLstClear(&_List);
	return _TraitList;
}

struct Trait* TraitLoad(lua_State* _State, int _Index) {
	int _Top = lua_gettop(_State);
	const char* _Name = NULL;
	struct Trait* _Trait = NULL;

	_Index = lua_absindex(_State, _Index);
	if(LuaRawString(_State, _Index, "Name", &_Name) == 0)
		goto error;
	_Trait = CreateTrait(_Name);
	error:
	lua_settop(_State, _Top);
	return _Trait;
}

struct Trait** TraitLoadGetList(lua_State* _State, const char* _Name, struct Trait** _List) {				
	lua_pushstring(_State, (_Name));						
	lua_rawget(_State, -2);									
	if(lua_type(_State, -1) != LUA_TTABLE)					
		goto error;											
	if(((_List) = TraitList(_State, -1)) == NULL)			
		goto error;											
	lua_pop(_State, 1);
	return _List;
	error:
	lua_pop(_State, 1);
	Log(ELOG_WARNING, "Error loading trait: invalid relation.");	
	return NULL;
}

void TraitLoadRelations(lua_State* _State, struct Trait* _Trait) {
	enum {
		LIKES,
		DISLIKES,
		PREVENTS,
		TRAITLIST_SIZE
	};
	struct Trait** _TraitList[TRAITLIST_SIZE];
	const char* _TraitNames[TRAITLIST_SIZE] = {
		"Likes",
		"Dislikes",
		"Prevents"
	};

	lua_pushstring(_State, _Trait->Name);
	lua_rawget(_State, -2);
	
	for(int i = 0; i < TRAITLIST_SIZE; ++i) {
		_TraitList[i] = TraitLoadGetList(_State, _TraitNames[i], _TraitList[i]);
	}
	_Trait->Likes = _TraitList[LIKES];
	_Trait->Dislikes = _TraitList[DISLIKES];
	_Trait->Prevents = _TraitList[PREVENTS];
	lua_pop(_State, 1);
	Log(ELOG_INFO, "Loaded Trait %s.", _Trait->Name);
}
