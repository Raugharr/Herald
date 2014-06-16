/*
 * File: Occupation.c
 * Author: David Brotz
 */

#include "Occupation.h"

#include "Herald.h"
#include "LuaWrappers.h"
#include "sys/LuaHelper.h"
#include "sys/HashTable.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>

struct Occupation* CreateOccupation(const char* _Name, struct Good* _Output, struct Building* _Workplace, struct Constraint* _AgeConst) {
	struct Occupation* _Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = NextId();
	_Occupation->Name = _Name;
	_Occupation->Job.Output = _Output;
	_Occupation->Job.Workplace = _Workplace;
	_Occupation->SpecialJob = ENONE;
	_Occupation->AgeConst = _AgeConst;
	return 	_Occupation;
}

struct Occupation* CreateOccupationSpecial(const char* _Name, int _Job) {
	struct Occupation* _Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = NextId();
	_Occupation->Name = _Name;
	_Occupation->SpecialJob = _Job;
	return _Occupation;
}

struct Occupation* CopyOccupation(const struct Occupation* _Job) {
	struct Occupation* 	_Occupation = (struct Occupation*) malloc(sizeof(struct Occupation));

	_Occupation->Id = _Job->Id;
	_Occupation->Name = _Job->Name;
	_Occupation->SpecialJob = _Job->SpecialJob;
	if(_Job->SpecialJob == ENONE)
		_Occupation->Job.Workplace = _Job->Job.Workplace;
	_Occupation->AgeConst = _Job->AgeConst;
	return 	_Occupation;
}

void DestroyJob(struct Occupation* _Occupation) {
	free(_Occupation);
}

struct Occupation* OccupationLoad(lua_State* _State, int _Index) {
	int _Return = 0;
	const char* _Key = NULL;
	char* _Name = NULL;
	char* _Temp = NULL;
	struct Good* _Output = NULL;
	struct Building* _Workplace = NULL;
	struct Constraint* _AgeConst = NULL;

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("Output", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if((_Output = HashSearch(&g_Goods, _Temp)) == 0)
				return NULL;
		} else if(!strcmp("Workplace", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if((_Workplace = HashSearch(&g_Buildings, _Temp)) == 0)
				return NULL;
		} else if(!strcmp("AgeConst", _Key)) {
			if((_AgeConst = ConstraintFromLua(_State, -1)) == NULL)
				return NULL;
		}
		lua_pop(_State, 1);
		if(!(_Return > 0))
			return NULL;
	}
	return CreateOccupation(_Name, _Output, _Workplace, _AgeConst);
}

