/*
 * File: Population.c
 * Author: David Brotz
 */

#include "Population.h"

#include "Actor.h"
#include "Person.h"
#include "Good.h"
#include "Herald.h"
#include "sys/HashTable.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LuaHelper.h"
#include "sys/Log.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <lua/lua.h>

int AnimalTypeCmp(const void* _One, const void* _Two) {
	return ((struct Animal*)_One)->PopType->Id - ((struct Population*)((struct InputReq*)_Two)->Req)->Id;
}

int FoodArrayAnDepArray(const void* _One, const void* _Two) {
	int _LenOne = ArrayLen(_One);
	int _LenTwo = ArrayLen(((const struct AnimalDep*)_Two)->Tbl);
	int _Return = 0;
	int i;

	if(_LenOne != _LenTwo) {
		_Return = GoodBaseCmp(((const void**)_One)[0], ((const struct AnimalDep*)_Two)->Tbl[0]);
		if(_Return == 0)
			return _LenOne - _LenTwo;
		return _Return;
	}
	for(i = 0; i < _LenOne; ++i) {
		if((_Return = GoodBaseCmp(((const void**)_One)[i], ((const struct AnimalDep*)_Two)->Tbl[i])) != 0)
			return _Return;
	}
	return 0;
}

int AnDepArrayArrayCmp(const void* _One, const void* _Two) {
	int _LenOne = ArrayLen(((const struct AnimalDep*)_One)->Tbl);
	int _LenTwo = ArrayLen(((const struct AnimalDep*)_Two)->Tbl);
	int _Result = 0;
	int i;

	if(_LenOne != _LenTwo)
		return _LenOne - _LenTwo;
	for(i = 0; i < _LenOne; ++i) {
		if((_Result = GoodBaseCmp(((const struct AnimalDep*)_One)->Tbl[i], ((const struct AnimalDep*)_Two)->Tbl[i])) != 0)
			return _Result;
	}
	return 0;
}

int AnDepArrayCmp(const void* _One, const void* _Two) {
	int _Result = GoodBaseCmp(_One, ((const struct AnimalDep*)_Two)->Tbl[0]);
	int _Len = ArrayLen(((const struct AnimalDep*)_Two)->Tbl);
	if(_Len == 1 && _Result == 0)
		return 0;
	else if(_Len != 1)
		return -1;
	return _Result;
}

struct Population* CreatePopulation(const char* _Name, int _Nutrition, int _Meat, int _Milk, struct Constraint** _Ages, double _MaleRatio) {
	struct Population* _Population = (struct Population*) malloc(sizeof(struct Population));

	_Population->Id = NextId();
	_Population->Name = (char*) calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Population->Name, _Name);
	_Population->Nutrition = _Nutrition;
	_Population->Meat = _Meat;
	_Population->Milk = _Milk;
	_Population->Ages = _Ages;
	_Population->Outputs = NULL;
	_Population->MaleRatio = _MaleRatio;
	return _Population;
}

struct Population* CopyPopulation(const struct Population* _Population) {
	struct Population* _New = (struct Population*) malloc(sizeof(struct Population));
	int i;

	_New->Id = NextId();
	_New->Name = (char*) calloc(strlen(_Population->Name) + 1, sizeof(char));
	strcpy(_New->Name, _Population->Name);
	_New->Nutrition = _Population->Nutrition;
	_New->Meat = _Population->Meat;
	_New->Milk = _Population->Milk;
	_New->MaleRatio = _Population->MaleRatio;
	_New->Ages = CopyConstraintBnds(_Population->Ages);
	for(i = 0; _Population->Outputs[i] != NULL; ++i)
		_New->Outputs[i] = _Population->Outputs[i];
	_New->Outputs[i] = _Population->Outputs[i];
	_New->EatsSize = _Population->EatsSize;

	_New->Eats = calloc(_New->EatsSize, sizeof(struct FoodBase*));
	for(i = 0; i < _New->EatsSize; ++i)
		_New->Eats[i] = _Population->Eats[i];
	return _New;
}

int PopulationCmp(const void* _One, const void* _Two) {
	return ((struct Population*)_One)->Id - ((struct Population*)_Two)->Id;
}

int PopulationFoodCmp(const void* _One, const void* _Two) {
	int _Result = 0;
	int i;

	if((_Result = ((struct Population*)_One)->EatsSize - ((struct Population*)_Two)->EatsSize) != 0)
		return _Result;
	for(i = 0; i < ((struct Population*)_One)->EatsSize; ++i) {
		if((_Result = ((struct Population*)_One)->Eats[i]->Id - ((struct Population*)_Two)->Eats[i]->Id) != 0)
			return _Result;
	}
	return 0;
}

void DestroyPopulation(struct Population* _Population) {
	int i;

	free(_Population->Name);
	DestroyConstrntBnds(_Population->Ages);
	for(i = 0; _Population->Outputs[i] != NULL; ++i)
		free(_Population->Outputs[i]);
	free(_Population->Outputs[i]);
	free(_Population->Outputs);
	free(_Population);
}

struct Population* PopulationLoad(lua_State* _State, int _Index) {
	int i;
	const char* _Key = NULL;
	const char* _Name = NULL;
	struct Constraint** _Ages = NULL;
	struct LinkedList* _List = CreateLinkedList();
	struct Population* _Pop = NULL;
	struct FoodBase** _Eats = NULL;
	struct LnkLst_Node* _Itr = NULL;
	int _Young = 0;
	int _Old = 0;
	int _Death = 0;
	int _Nutrition = 0;
	int _Meat = 0;
	int _Milk = 0;
	double _MaleRatio = 0;
	int _Return = -2;
	int _Top = lua_gettop(_State);

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if (!strcmp("Nutrition", _Key))
			_Return = AddInteger(_State, -1, &_Nutrition);
		else if(!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("MaleRatio", _Key))
			_Return = AddNumber(_State, -1, &_MaleRatio);
		else if(!strcmp("Meat", _Key))
			_Return = AddInteger(_State, -1, &_Meat);
		else if(!strcmp("Milk", _Key))
			_Return = AddInteger(_State, -1, &_Milk);
		else if(!strcmp("MatureAge", _Key))
			_Return = LuaIntPair(_State, -1, &_Young, &_Old);
		else if(!strcmp("DeathAge", _Key)) {
			_Return = AddInteger(_State, -1, &_Death);
		} else if(!strcmp("Eats", _Key)) {
			_Return = 1;
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				void* _Data = NULL;
				if(lua_isstring(_State, -1) == 0)
					goto EatsEnd;
				if((_Data = HashSearch(&g_Goods, lua_tostring(_State, -1))) == NULL) {
					Log(ELOG_WARNING, "Food %s could not be found.", lua_tostring(_State, -1));
					goto EatsEnd;
				}
				LnkLst_PushBack(_List, _Data);
				EatsEnd:
				lua_pop(_State, 1);
			}
		} else {
			Log(ELOG_WARNING, "%s is not a field of a Population.", _Key);
			goto fail;
		}
		if(!(_Return > 0)) {
			Log(ELOG_WARNING, "%s contains invalid data for a Population.", _Key);
			goto fail;
		}
		lua_pop(_State, 1);
	}
	if(_Young > _Old || _Old > _Death || _Death < 0) {
		Log(ELOG_WARNING, "%s age limits are invalid.", _Name);
		goto fail;
	}
	_Ages = CreateConstrntBnds(4, 0, _Young, _Old, _Death);
	_Pop = CreatePopulation(_Name, _Nutrition, _Meat, _Milk, _Ages, _MaleRatio);
	_Eats = calloc(_List->Size, sizeof(struct FoodBase*));
	_Itr = _List->Front;
	i = 0;
	while(_Itr != NULL) {
		_Eats[i] =_Itr->Data;
		InsertionSort(_Eats, i + 1, GoodBaseCmp);
		_Itr = _Itr->Next;
		++i;
	}
	_Pop->Outputs = malloc(sizeof(struct Good*));
	_Pop->Outputs[0] = NULL;
	_Pop->Eats = _Eats;
	_Pop->EatsSize = _List->Size;
	if(_Pop->EatsSize == 0)
		Log(ELOG_WARNING, "Population %s has zero food types to consume.", _Name);
	DestroyLinkedList(_List);
	return _Pop;
	fail:
	lua_settop(_State, _Top);
	DestroyLinkedList(_List);
	return NULL;
}

struct Animal* CreateAnimal(const struct Population* _Pop, int _Age, int _Nutrition, int _X, int _Y) {
	struct Animal* _Animal = (struct Animal*) malloc(sizeof(struct Animal));
	int _Gender = 0;

	if(Random(0, 999) < (int)(_Pop->MaleRatio * 1000 - 1)) {
		_Gender = EMALE;
	} else
		_Gender = EFEMALE;
	CtorActor(((struct Actor*)_Animal), OBJECT_ANIMAL,  _X, _Y, (int(*)(struct Object*))ActorThink, _Gender, _Nutrition, _Age);
	_Animal->PopType = _Pop;
	return _Animal;
}

int AnimalCmp(const void* _One, const void* _Two) {
	return ((struct Animal*)_One)->Id - ((struct Animal*)_Two)->Id;
}

void DestroyAnimal(struct Animal* _Animal) {
	free(_Animal);
}

void AnimalDepAddAn(const struct AnimalDep* _Dep, const struct Array* _Tbl) {
	int _Len = 0;
	int _DepLen = 0;
	int i;
	int j;

	for(i = 0; i < _Tbl->Size; ++i) {
		_Len = ArrayLen(((struct AnimalDep*)_Tbl->Table[i])->Animals->Table);
		if(_Len <= 1)
			continue;
		_DepLen = ArrayLen(_Dep->Tbl);
		for(j = 0; j < _DepLen; ++j) {
			if(BinarySearch(_Dep->Tbl[j], _Tbl->Table, _Tbl->Size, AnDepArrayCmp) == NULL)
				return;
		}
		((struct AnimalDep*)_Tbl->Table[i])->Nutrition += _Dep->Nutrition;
	}
}

struct Array* AnimalFoodDep(const struct HashTable* _Table) {
	int i;
	int _SetSize = 0;
	struct Array* _Array = CreateArray(_Table->Size);
	struct Population* _Pop = NULL;
	struct HashItrCons* _Itr = HashCreateItrCons(_Table);
	struct FoodBase** _Temp = NULL;
	struct AnimalDep* _Dep = NULL;
	struct AnimalDep* _Search = NULL;
	struct FoodBase*** _Set = NULL;

	while(_Itr != NULL) {
		_Pop = _Itr->Node->Pair;
		for(i = 0; i < _Pop->EatsSize; ++i) {
			if((_Dep = BinarySearch(_Pop->Eats[i], _Array->Table, _Array->Size, AnDepArrayCmp)) == NULL) {
				_Dep = (struct AnimalDep*) malloc(sizeof(struct AnimalDep));
				_Temp = calloc(2, sizeof(struct FoodBase*));
				_Temp[0] = _Pop->Eats[i];
				_Temp[1] = NULL;
				_Dep->Tbl = _Temp;
				_Dep->Animals = CreateArray(4);
				_Dep->Nutrition = 0;
				ArrayInsertSort_S(_Array, _Dep, AnDepArrayArrayCmp);
			}
			ArrayInsertSort_S(_Dep->Animals, _Pop, PopulationCmp);
		}
		if(_Pop->EatsSize == 0)
			goto loopend;
		_Temp = calloc(_Pop->EatsSize + 1, sizeof(struct FoodBase*));
		_Dep = (struct AnimalDep*) malloc(sizeof(struct AnimalDep));
		for(i = 0; i < _Pop->EatsSize; ++i) {
			_Temp[i] = _Pop->Eats[i];
		}
		_Temp[i] = NULL;
		_Dep->Nutrition = _Pop->Nutrition;
		_Dep->Tbl = _Temp;

		_Set = PowerSet(_Temp,_Pop->EatsSize);
		_SetSize = pow(2, _Pop->EatsSize);
		for(i = 1; i < _SetSize; ++i) {
			if((_Search = BinarySearch(_Set[i], _Array->Table, _Array->Size, FoodArrayAnDepArray)) != NULL)
				_Search->Nutrition += _Pop->Nutrition;
			free(_Set[i]);
		}
		free(_Set[0]);
		free(_Set[_SetSize]);
		free(_Set);
		if((_Search = BinarySearch(_Dep, _Array->Table, _Array->Size, AnDepArrayArrayCmp)) != NULL) {
			ArrayInsertSort_S(_Search->Animals, _Pop, PopulationCmp);
			free(_Dep);
			free(_Temp);
			goto loopend;
		}
		_Dep->Animals = CreateArray(4);
		ArrayInsertSort_S(_Dep->Animals, _Pop, PopulationCmp);
		ArrayInsertSort_S(_Array, _Dep, AnDepArrayArrayCmp);
		loopend:
		_Itr = HashNextCons(_Table, _Itr);
	}
	HashDeleteItrCons(_Itr);
	if(_Array->Size > 0) {
		_Array->Table = realloc(_Array->Table, _Array->Size * sizeof(struct FoodBase*));
		_Array->TblSize = _Array->Size;
	}
	return _Array;
}

struct InputReq** AnimalTypeCount(const struct Array* _Animals, int* _Size) {
	int i;
	struct InputReq** _AnimalTypes = (struct InputReq**)alloca(_Animals->Size * sizeof(struct InputReq*));
	struct InputReq** _Return = NULL;
	struct InputReq* _Req = NULL;
	struct Animal* _Animal = NULL;

	*_Size = 0;
	memset(_AnimalTypes, 0, sizeof(struct InputReq*));
	for(i = 0; i < _Animals->Size; ++i) {
		_Animal = _Animals->Table[i];
		if((_Req = LinearSearch(_Animal, _AnimalTypes, *_Size, AnimalTypeCmp)) == NULL) {
			_Req = CreateInputReq();
			_Req->Req = _Animal->PopType;
			_Req->Quantity = 1;
			_AnimalTypes[*_Size] = _Req;
			InsertionSort(_AnimalTypes, *_Size, AnimalTypeCmp);
			++(*_Size);
		} else
			++_Req->Quantity;
	}
	_Return = calloc(_Animals->Size, sizeof(struct InputReq*));
	memcpy(_Return, _AnimalTypes, sizeof(struct InputReq*) * _Animals->Size);
	return _Return;
}
