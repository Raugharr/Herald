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

#define TraitLoadGetList(_Name, _Index, _List)				\
	lua_pushstring(_State, (_Name));						\
	lua_rawget(_State, -2);									\
	if(lua_type(_State, -1) != LUA_TTABLE)					\
		goto error;											\
	if(((_List) = TraitList(_State, -1)) == NULL)			\
		goto error;											\
	_List = TraitList(_State, -1);							\
	lua_pop(_State, 1)

void TraitLoadRelations(lua_State* _State, struct Trait* _Trait) {
	struct Trait** _Likes = NULL;
	struct Trait** _Dislikes = NULL;
	struct Trait** _Prevents = NULL;

	lua_pushstring(_State, _Trait->Name);
	lua_rawget(_State, -2);

	TraitLoadGetList("Likes", _Index, _Likes);
	TraitLoadGetList("Dislikes", _Index, _Dislikes);
	TraitLoadGetList("Prevents", _Index, _Prevents);
	_Trait->Likes = _Likes;
	_Trait->Dislikes = _Dislikes;
	_Trait->Prevents = _Prevents;
	lua_pop(_State, 1);
	Log(ELOG_INFO, "Loaded Trait %s.", _Trait->Name);
	return;
	error:
	lua_pop(_State, 1);
	Log(ELOG_WARNING, "Error loading trait: invalid relation.");	
}
