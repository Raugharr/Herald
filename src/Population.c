/*
 * File: Population.c
 * Author: David Brotz
 */

#include "Population.h"

#include "sys/LuaHelper.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>

struct Population* CreatePopulation(const char* _Name, int _AdultFood, int _ChildFood, int _AdultAge) {
	struct Population* _Population = (struct Population*) malloc(sizeof(struct Population));

	_Population->Name = _Name;
	_Population->AdultFood = _AdultFood;
	_Population->ChildFood = _ChildFood;
	_Population->AdultAge = _AdultAge;
	return _Population;
}

struct Population* CopyPopulation(const struct Population* _Population, int _Quantity) {
	struct Population* _NewPopulation = (struct Population*) malloc(sizeof(struct Population));

	_NewPopulation->Name = _Population->Name;
	_NewPopulation->AdultFood = _Population->AdultFood;
	_NewPopulation->ChildFood = _Population->AdultFood;
	_NewPopulation->AdultAge = _Population->AdultFood;
	_NewPopulation->Quantity = _Population->AdultFood;
	return _NewPopulation;
}

void DestroyPopulation(struct Population* _Population) {
	free(_Population);
}

struct Population* PopulationLoad(lua_State* _State, int _Index) {
	const char* _Key = NULL;
	const char* _Name = NULL;
	int _AdultAge = 0;
	int _AdultFood = 0;
	int _ChildFood = 0;
	int _Return = -2;
	int _Top = lua_gettop(_State);

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("AdultAge", _Key))
			_Return = AddInteger(_State, -1, &_AdultAge);
		else if (!strcmp("AdultFood", _Key))
			_Return = AddInteger(_State, -1, &_AdultFood);
		else if (!strcmp("ChildFood", _Key))
			_Return = AddInteger(_State, -1, &_ChildFood);
		else if (!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		if(!(_Return > 0)) {
			lua_settop(_State, _Top);
			return NULL;
		}
		lua_pop(_State, 1);
	}
	return CreatePopulation(_Name, _AdultFood, _ChildFood, _AdultAge);
}
