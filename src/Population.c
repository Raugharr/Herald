/*
 * File: Population.c
 * Author: David Brotz
 */

#include "Population.h"

#include "Person.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LuaHelper.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>

struct Population* CreatePopulation(const char* _Name, int _AdultFood, int _ChildFood, struct Constraint** _Ages, int _Meat, int _Milk) {
	struct Population* _Population = (struct Population*) malloc(sizeof(struct Population));

	_Population->Id = NextId();
	_Population->Name = (char*) calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Population->Name, _Name);
	_Population->AdultFood = _AdultFood;
	_Population->ChildFood = _ChildFood;
	_Population->Ages = _Ages;
	_Population->Meat = _Meat;
	_Population->Milk = _Milk;
	return _Population;
}

struct Population* CopyPopulation(const struct Population* _Population) {
	struct Population* _NewPopulation = (struct Population*) malloc(sizeof(struct Population));

	_NewPopulation->Id = NextId();
	_NewPopulation->Name = (char*) calloc(strlen(_Population->Name) + 1, sizeof(char));
	strcpy(_NewPopulation->Name, _Population->Name);
	_NewPopulation->AdultFood = _Population->AdultFood;
	_NewPopulation->ChildFood = _Population->AdultFood;
	_NewPopulation->Ages = _Population->Ages;
	_NewPopulation->Meat = _Population->Meat;
	_NewPopulation->Milk = _Population->Milk;
	return _NewPopulation;
}

void DestroyPopulation(struct Population* _Population) {
	free(_Population->Name);
	DestroyConstrntBnds(_Population->Ages);
	DestroyArray(_Population->Output);
	free(_Population);
}

struct Population* PopulationLoad(lua_State* _State, int _Index) {
	const char* _Key = NULL;
	const char* _Name = NULL;
	struct Constraint** _Ages = NULL;
	int _Young = 0;
	int _Old = 0;
	int _Death = 0;
	int _AdultFood = 0;
	int _ChildFood = 0;
	int _Meat = 0;
	int _Milk = 0;
	double _MalePercent = 0;
	int _Return = -2;
	int _Top = lua_gettop(_State);

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if (!strcmp("AdultFood", _Key))
			_Return = AddInteger(_State, -1, &_AdultFood);
		else if(!strcmp("ChildFood", _Key))
			_Return = AddInteger(_State, -1, &_ChildFood);
		else if(!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("MalePercent", _Key))
			_Return = AddNumber(_State, -1, &_MalePercent);
		else if(!strcmp("Meat", _Key))
			_Return = AddInteger(_State, -1, &_Meat);
		else if(!strcmp("Milk", _Key))
			_Return = AddInteger(_State, -1, &_Milk);
		else if(!strcmp("MatureAge", _Key))
			_Return = LuaIntPair(_State, -1, &_Young, &_Old);
		else if(!strcmp("DeathAge", _Key)) {
			_Return = AddInteger(_State, -1, &_Death);
		}
		if(!(_Return > 0)) {
			lua_settop(_State, _Top);
			return NULL;
		}
		lua_pop(_State, 1);
	}
	_Ages = CreateConstrntBnds(4, 0, _Young, _Old, _Death);
	return CreatePopulation(_Name, _AdultFood, _ChildFood, _Ages, _Meat, _Milk);
}

struct Animal* CreateAnimal(struct Population* _Pop, int _Age) {
	struct Animal* _Animal = (struct Animal*) malloc(sizeof(struct Animal*));

	_Animal->Id = NextId();
	if(Random(0, 999) < _Pop->MalePercent) {
		_Animal->Gender = EMALE;
	} else
		_Animal->Gender = EFEMALE;
	_Animal->Age = _Age;
	_Animal->PopType = _Pop;
	return _Animal;
}

void DestroyAnimal(struct Animal* _Animal) {
	free(_Animal);
}

void AnimalUpdate(struct Animal* _Animal) {
	NextDay(&_Animal->Age);
}

