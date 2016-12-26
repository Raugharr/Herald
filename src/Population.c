/*
 * File: Population.c
 * Author: David Brotz
 */

#include "Population.h"

#include "Person.h"
#include "Good.h"
#include "Herald.h"
#include "sys/HashTable.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Math.h"
#include "sys/LuaCore.h"
#include "sys/Log.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <lua/lua.h>

int AnimalTypeCmp(const void* One, const void* Two) {
	return ((struct Animal*)One)->PopType->Id - ((struct Population*)((struct InputReq*)Two)->Req)->Id;
}

int FoodArrayAnDepArray(const void* One, const void* Two) {
	int LenOne = ((const struct AnimalDep*)One)->TblSz;
	int LenTwo = ((const struct AnimalDep*)Two)->TblSz;
	int Return = 0;
	int i;

	if(LenOne != LenTwo) {
		Return = GoodBaseCmp(((const void**)One)[0], ((const struct AnimalDep*)Two)->Tbl[0]);
		if(Return == 0)
			return LenOne - LenTwo;
		return Return;
	}
	for(i = 0; i < LenOne; ++i) {
		if((Return = GoodBaseCmp(((const void**)One)[i], ((const struct AnimalDep*)Two)->Tbl[i])) != 0)
			return Return;
	}
	return 0;
}

int AnDepArrayArrayCmp(const void* One, const void* Two) {
	int LenOne = ((const struct AnimalDep*)One)->TblSz;
	int LenTwo = ((const struct AnimalDep*)Two)->TblSz;
	int Result = 0;

	if(LenOne != LenTwo)
		return LenOne - LenTwo;
	for(int i = 0; i < LenOne; ++i) {
		if((Result = GoodBaseCmp(((const struct AnimalDep*)One)->Tbl[i], ((const struct AnimalDep*)Two)->Tbl[i])) != 0)
			return Result;
	}
	return 0;
}

int AnDepArrayCmp(const void* One, const void* Two) {
	int Result = GoodBaseCmp(One, ((const struct AnimalDep*)Two)->Tbl[0]);
	int Len = ((const struct AnimalDep*)Two)->TblSz;
	if(Len == 1 && Result == 0)
		return 0;
	else if(Len != 1)
		return -1;
	return Result;
}

struct Population* CreatePopulation(const char* Name, int Nutrition, int Meat, int Milk, struct Constraint** Ages, double MaleRatio, int FMRatio,
	double ReproduceMin, double ReproduceMax, int SpaceReq, uint8_t Wealth) {
	struct Population* Population = (struct Population*) malloc(sizeof(struct Population));

	Population->Id = NextId();
	Population->Name = (char*) calloc(strlen(Name) + 1, sizeof(char));
	strcpy(Population->Name, Name);
	Population->Nutrition = Nutrition;
	Population->Meat = Meat;
	Population->Milk = Milk;
	Population->Ages = Ages;
	Population->Outputs = NULL;
	Population->MaleRatio = MaleRatio;
	Population->FMRatio = FMRatio;
	Population->SpaceReq = SpaceReq;
	Population->MaxNutrition = Nutrition * 31;

	Population->Skin.Skin = NULL;
	Population->Skin.Pounds = 0.0;
	Population->Hair.Hair = NULL;
	Population->Hair.Pounds = 0.0;
	Population->Hair.Shearable = 0;
	Population->ReproduceRate.Min = ReproduceMin * 100;
	Population->ReproduceRate.Max = ReproduceMax * 100;
	Population->Wealth = Wealth;
	return Population;
}

struct Population* CopyPopulation(const struct Population* Population) {
	struct Population* New = (struct Population*) malloc(sizeof(struct Population));
	int i = 0;

	New->Id = NextId();
	New->Name = (char*) calloc(strlen(Population->Name) + 1, sizeof(char));
	strcpy(New->Name, Population->Name);
	New->Nutrition = Population->Nutrition;
	New->Meat = Population->Meat;
	New->Milk = Population->Milk;
	New->MaleRatio = Population->MaleRatio;
	New->Ages = CopyConstraintBnds(Population->Ages);
	for(i = 0; Population->Outputs[i] != NULL; ++i)
		New->Outputs[i] = Population->Outputs[i];
	New->Outputs[i] = Population->Outputs[i];
	New->EatsSize = Population->EatsSize;

	New->Eats = calloc(New->EatsSize, sizeof(struct FoodBase*));
	for(i = 0; i < New->EatsSize; ++i)
		New->Eats[i] = Population->Eats[i];
	return New;
}

int PopulationCmp(const void* One, const void* Two) {
	return ((struct Population*)One)->Id - ((struct Population*)Two)->Id;
}

int PopulationFoodCmp(const void* One, const void* Two) {
	int Result = 0;
	int i;

	if((Result = ((struct Population*)One)->EatsSize - ((struct Population*)Two)->EatsSize) != 0)
		return Result;
	for(i = 0; i < ((struct Population*)One)->EatsSize; ++i) {
		if((Result = ((struct Population*)One)->Eats[i]->Base.Id - ((struct Population*)Two)->Eats[i]->Base.Id) != 0)
			return Result;
	}
	return 0;
}

void DestroyPopulation(struct Population* Population) {
	free(Population->Name);
	DestroyConstrntBnds(Population->Ages);
	for(int i = 0; Population->Outputs[i] != NULL; ++i)
		free(Population->Outputs[i]);
	free(Population->Outputs);
	free(Population);
}

struct Population* PopulationLoad(lua_State* State, int Index) {
	int i;
	const char* Key = NULL;
	const char* Name = NULL;
	struct Constraint** Ages = NULL;
	struct LinkedList List = {0, NULL, NULL};
	struct Population* Pop = NULL;
	struct FoodBase** Eats = NULL;
	struct LnkLst_Node* Itr = NULL;
	int Young = 0;
	int Old = 0;
	int Death = 0;
	int Nutrition = 0;
	int Meat = 0;
	int Milk = 0;
	int IsShearable = 0;
	int FMRatio = 0;
	int SpaceReq = 0;
	const struct GoodBase* SkinGood = NULL;
	const struct GoodBase* HairGood = NULL;
	double SkinPounds = 0.0;
	double HairPounds = 0.0;
	double MaleRatio = 0;
	double ReproduceMin = 0.0f;
	double ReproduceMax = 0.0f;
	int Return = -2;
	int Top = lua_gettop(State);
	double Wealth = 0;

	lua_getmetatable(State, Index);
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(lua_isstring(State, -2))
			Key = lua_tostring(State, -2);
		else
			continue;
		if (!strcmp("Nutrition", Key))
			Return = LuaGetInteger(State, -1, &Nutrition);
		else if(!strcmp("Name", Key))
			Return = LuaGetString(State, -1, &Name);
		else if(!strcmp("MaleRatio", Key))
			Return = LuaGetNumber(State, -1, &MaleRatio);
		else if(!strcmp("Meat", Key))
			Return = LuaGetInteger(State, -1, &Meat);
		else if(!strcmp("Milk", Key))
			Return = LuaGetInteger(State, -1, &Milk);
		else if(!strcmp("MatureAge", Key)) {
			Return = LuaIntPair(State, -1, &Young, &Old);
			Young = YearToDays(Young);
			Old = YearToDays(Old);
		} else if(!strcmp("DeathAge", Key)) {
			Return = LuaGetInteger(State, -1, &Death);
			Death = YearToDays(Death);
		} else if (!strcmp("SpaceReq", Key)) { 
			Return = LuaGetInteger(State, -1, &SpaceReq);
		} else if(!strcmp("FMRatio", Key)) {
			Return = LuaGetInteger(State, -1, &FMRatio);
		} else if(!strcmp("Wealth", Key)) {
			Return = LuaGetNumber(State, -1, &Wealth);
		} else if(!strcmp("Reproduce", Key)) {
			if(lua_type(State, -1) != LUA_TTABLE)
				Return = 0;
			lua_rawgeti(State, -1, 1);
			if((Return = LuaGetNumber(State, -1, &ReproduceMin)) <= 0)
				goto loop_end;
			lua_rawgeti(State, -2, 2);
			if((Return = LuaGetNumber(State, -1, &ReproduceMax)) <= 0)
				goto loop_end;
			lua_pop(State, 2);
				
		} else if(!strcmp("Eats", Key)) {
			Return = 1;
			lua_pushnil(State);
			while(lua_next(State, -2) != 0) {
				void* Data = NULL;

				if(lua_isstring(State, -1) == 0)
					goto EatsEnd;
				if((Data = HashSearch(&g_Goods, lua_tostring(State, -1))) == NULL) {
					Log(ELOG_WARNING, "Food %s could not be found.", lua_tostring(State, -1));
					goto EatsEnd;
				}
				LnkLstPushBack(&List, Data);
				EatsEnd:
				lua_pop(State, 1);
			}
		} else if(!strcmp("Skin", Key)) {
			if(lua_type(State, -1) != LUA_TTABLE) {
				if(lua_isnil(State, -1) == 0)
					Log(ELOG_INFO, "Population skin variable is not a table.");
				goto loop_end;
			}
			lua_pushstring(State, "Type");
			lua_rawget(State, -2);
			if(lua_type(State, -1) != LUA_TSTRING) {
				Log(ELOG_INFO, "Population skin.Type variable is not a table.");
				goto loop_end;
			}
			if((SkinGood = HashSearch(&g_Goods, lua_tostring(State, -1))) == NULL) {
				Log(ELOG_INFO, "Population skin.Type: %s is not a good.", lua_tostring(State, -1));
				lua_pop(State, 2);
				continue;
			}
			lua_pop(State, 1);
			lua_pushstring(State, "Pounds");
			lua_rawget(State, -2);
			if(lua_type(State, LUA_TNUMBER) == 0) {
				Log(ELOG_INFO, "Population skin.Pounds variable is not a number.");
				lua_pop(State, 2);
				continue;;
			}
			SkinPounds = lua_tonumber(State, -1);
			Return = 1;
			lua_pop(State, 1);
		} else if(!strcmp("Hair", Key)) {
			if(lua_type(State, -1) != LUA_TTABLE) {
				if(lua_isnil(State, -1) == 0)
					Log(ELOG_INFO, "Population hair variable is not a table.");
				goto loop_end;
			}
			lua_pushstring(State, "Type");
			lua_rawget(State, -2);
			if(lua_type(State, -1) != LUA_TSTRING) {
				Log(ELOG_INFO, "Population hair.Type variable is not a table.");
				goto loop_end;
			}
				if((HairGood = HashSearch(&g_Goods, lua_tostring(State, -1))) == NULL) {
					Log(ELOG_INFO, "Population hair.Type: %s is not a good.", lua_tostring(State, -1));
					goto loop_end;
				}
				lua_pop(State, 1);
				lua_pushstring(State, "Pounds");
				lua_rawget(State, -2);
				if(lua_type(State, LUA_TNUMBER) == 0) {
					Log(ELOG_INFO, "Population hair.Pounds variable is not a number.");
					lua_pop(State, 2);
					continue;
				}
				HairPounds = lua_tonumber(State, -1);
				lua_pop(State, 1);
				lua_pushstring(State, "IsShearable");
				lua_rawget(State, -2);
				if(lua_type(State, -1) != LUA_TBOOLEAN) {
					Log(ELOG_INFO, "Population hair.IsShearable variable is not a number.");
					lua_pop(State, 2);
					continue;
				}
				IsShearable = lua_toboolean(State, -1);
				Return = 1;
				lua_pop(State, 1);
		} else {
			Log(ELOG_WARNING, "%s is not a field of a Population.", Key);
			goto fail;
		}
		if(!(Return > 0)) {
			Log(ELOG_WARNING, "%s contains invalid data for a Population.", Key);
			goto fail;
		}
		loop_end:
		lua_pop(State, 1);
		Return = 0;
	}
	if(Young > Old || Old > Death || Death < 0) {
		Log(ELOG_WARNING, "%s age limits are invalid.", Name);
		goto fail;
	}
	Ages = CreateConstrntBnds(4, 0, Young, Old, Death);
	Pop = CreatePopulation(Name, Nutrition, Meat, Milk, Ages, MaleRatio, FMRatio, ReproduceMin, ReproduceMax, SpaceReq, ((uint8_t)Wealth * 100));
	Eats = calloc(List.Size, sizeof(struct FoodBase*));
	Itr = List.Front;
	i = 0;
	while(Itr != NULL) {
		Eats[i] = Itr->Data;
		InsertionSort(Eats, i + 1, GoodBaseCmp, sizeof(*Eats));
		Itr = Itr->Next;
		++i;
	}
	Pop->Skin.Skin = SkinGood;
	Pop->Skin.Pounds = SkinPounds;
	Pop->Hair.Hair = HairGood;
	Pop->Hair.Pounds = HairPounds;
	Pop->Hair.Shearable = IsShearable;
	Pop->Outputs = malloc(sizeof(struct Good*));
	Pop->Outputs[0] = NULL;
	Pop->Eats = Eats;
	Pop->EatsSize = List.Size;
	if(Pop->EatsSize == 0)
		Log(ELOG_WARNING, "Population %s has zero food types to consume.", Name);
	return Pop;
	fail:
	lua_settop(State, Top);
	return NULL;
}

struct Animal* CreateAnimal(const struct Population* Pop, int Age, int Nutrition, int X, int Y) {
	struct Animal* Animal = (struct Animal*) malloc(sizeof(struct Animal));
	int Gender = 0;

	if(Random(0, 999) < (int)(Pop->MaleRatio * 1000 - 1)) {
		Gender = EMALE;
	} else
		Gender = EFEMALE;
	CreateObject(&Animal->Object, OBJECT_ANIMAL, AnimalThink);
	Animal->Age = Age;
	Animal->Pos.x = X;
	Animal->Pos.y = Y;
	Animal->Gender = Gender;
	Animal->Nutrition = Nutrition;
	*(const struct Population**)&Animal->PopType = Pop;
	return Animal;
}

int AnimalCmp(const void* One, const void* Two) {
	return ((struct Animal*)One)->Object.Id - ((struct Animal*)Two)->Object.Id;
}

void DestroyAnimal(struct Animal* Animal) {
	DestroyObject(&Animal->Object);
	free(Animal);
}

void AnimalThink(struct Object* Obj) {
	struct Animal* Animal = (struct Animal*) Obj;

	Animal->Nutrition -= (AnimalMature(Animal) == true) ? (Animal->PopType->Nutrition) : (Animal->PopType->Nutrition / 2);
	if(Animal->Nutrition < 0)
		Animal->Nutrition = 0;
	NextDay(&Animal->Age);
	if(Animal->Nutrition > MAX_NUTRITION)
		Animal->Nutrition = MAX_NUTRITION;
}

void AnimalDepAddAn(const struct AnimalDep* Dep, const struct Array* Tbl) {
	int Len = 0;
	int DepLen = 0;

	for(int i = 0; i < Tbl->Size; ++i) {
		Len = ArrayLen(((struct AnimalDep*)Tbl->Table[i])->Animals.Table);
		if(Len <= 1)
			continue;
		DepLen = Dep->TblSz;
		for(int j = 0; j < DepLen; ++j) {
			if(BinarySearch(Dep->Tbl[j], Tbl->Table, Tbl->Size, AnDepArrayCmp) == NULL)
				return;
		}
		((struct AnimalDep*)Tbl->Table[i])->Nutrition += Dep->Nutrition;
	}
}

struct Array* AnimalFoodDep(const struct HashTable* Table) {
	int SetSize = 0;
	struct Array* Array = CreateArray(Table->Size);
	struct Population* Pop = NULL;
	struct HashItrCons* Itr = HashCreateItrCons(Table);
	struct AnimalDep* Dep = NULL;
	struct AnimalDep* Search = NULL;
	struct FoodBase*** Set = NULL;

	while(Itr != NULL) {
		Pop = Itr->Node->Pair;
		//Check if the food that this animal eats is already in Array or not and add it if it is.
		for(int i = 0; i < Pop->EatsSize; ++i) {
			if((Dep = BinarySearch(Pop->Eats[i], Array->Table, Array->Size, AnDepArrayCmp)) == NULL) {
				Dep = (struct AnimalDep*) malloc(sizeof(struct AnimalDep));
				Dep->Tbl = calloc(1, sizeof(struct FoodBase*));
				Dep->Tbl[0] = Pop->Eats[i];
				Dep->TblSz = 1;
				CtorArray(&Dep->Animals, 4);
				Dep->Nutrition = 0;
				ArrayInsertSort_S(Array, Dep, AnDepArrayArrayCmp);
			}
			ArrayInsertSort_S(&Dep->Animals, Pop, PopulationCmp);
		}
		if(Pop->EatsSize == 0)
			goto loopend;
		Dep = (struct AnimalDep*) malloc(sizeof(struct AnimalDep));
		Dep->Tbl = calloc(Pop->EatsSize, sizeof(struct FoodBase*));
		for(int i = 0; i < Pop->EatsSize; ++i) {
			Dep->Tbl[i] = Pop->Eats[i];
		}
		Dep->Nutrition = Pop->Nutrition;
		Dep->TblSz = Pop->EatsSize;

		Set = PowerSet(Dep->Tbl, Pop->EatsSize);
		SetSize = pow(2, Pop->EatsSize);
		for(int i = 1; i < SetSize; ++i) {
			if((Search = BinarySearch(Set[i], Array->Table, Array->Size, FoodArrayAnDepArray)) != NULL)
				Search->Nutrition += Pop->Nutrition;
			free(Set[i]);
		}
		free(Set[0]);
		free(Set[SetSize]);
		free(Set);
		//Search if Array already contains an AnimalDep that has the exact same FoodBase Tbl as Dep.
		if((Search = BinarySearch(Dep, Array->Table, Array->Size, AnDepArrayArrayCmp)) != NULL) {
			ArrayInsertSort_S(&Search->Animals, Pop, PopulationCmp);
			free(Dep);
			goto loopend;
		}
		CtorArray(&Dep->Animals, 4);
		ArrayInsertSort_S(&Dep->Animals, Pop, PopulationCmp);
		ArrayInsertSort_S(Array, Dep, AnDepArrayArrayCmp);
		loopend:
		Itr = HashNextCons(Table, Itr);
	}
	HashDeleteItrCons(Itr);
	if(Array->Size > 0) {
		Array->Table = realloc(Array->Table, Array->Size * sizeof(struct FoodBase*));
		Array->TblSize = Array->Size;
	}
	return Array;
}

struct InputReq** AnimalTypeCount(const struct Array* Animals, int* Size) {
	struct InputReq** AnimalTypes = (struct InputReq**)alloca(Animals->Size * sizeof(struct InputReq*));
	struct InputReq** Return = NULL;
	struct InputReq* Req = NULL;
	struct Animal* Animal = NULL;

	*Size = 0;
	if(Animals->Size == 0)
		return NULL;
	memset(AnimalTypes, 0, sizeof(struct InputReq*));
	for(int i = 0; i < Animals->Size; ++i) {
		Animal = Animals->Table[i];
		if((Req = LinearSearch(Animal, AnimalTypes, *Size, AnimalTypeCmp)) == NULL) {
			Req = CreateInputReq();
			Req->Req = Animal->PopType;
			Req->Quantity = 1;
			AnimalTypes[*Size] = Req;
			InsertionSort(AnimalTypes, *Size, AnimalTypeCmp, sizeof(*AnimalTypes));
			++(*Size);
		} else
			++Req->Quantity;
	}
	Return = calloc(Animals->Size, sizeof(struct InputReq*));
	memcpy(Return, AnimalTypes, sizeof(struct InputReq*) * Animals->Size);
	return Return;
}

//NOTE: Test me.
void AnimalArrayInsert(struct Array* Array, struct Animal* Animal) {
	//const struct Population* PopType = Animal->PopType;
	struct Animal* InsertAn = Animal;
	struct Animal* PrevAn = NULL;
	struct Animal* CurrAn = NULL;
	const struct Population* CurrPop = NULL;
#ifdef DEBUG
	int Size = Array->Size;
#endif

	if(Array->Size >= Array->TblSize)
		ArrayResize(Array);
	if(Array->Size == 0) {
		Array->Table[0] = Animal;
		++Array->Size;
		return;
	}
	//Keep all animals who are of the same type to be adjacent to each other.
	CurrPop = ((struct Animal*) Array->Table[0])->PopType;
	for(int i = 1; i < Array->Size; ++i) {
		PrevAn = Array->Table[i - 1];
		CurrAn = Array->Table[i];

		if(CurrPop == PrevAn->PopType && CurrPop != CurrAn->PopType) {
			struct Animal* Temp = Array->Table[i];
			
			Array->Table[i] = InsertAn;
			InsertAn = Temp;	
			CurrPop = InsertAn->PopType;
		}
	}
	ArrayInsert(Array, InsertAn);
#ifdef DEBUG
	Assert(Size + 1 == Array->Size);
#endif
	/*for(int i = 0; i < Array->Size; ++i) {
		InsertAn = (struct Animal*) Array->Table[i];
		if(InsertAn->PopType == PopType) {
			FoundType = 1;
		} else if(FoundType == 1) {
			Array->Table[i] = Animal;
			Animal = InsertAn;
			FoundType = 2;
		}
	}
	if(FoundType <= 1) {
		ArrayInsert(Array, Animal);
	} else if(FoundType == 2) {
		++Array->Size;
	}*/
}

//FIXME: Change to ensure all animals of the same type are adjacent.
struct Animal* AnimalArrayRemove(struct Array* Array, int Index) {
	const struct Population* PopType = NULL;
	struct Animal* Animal = NULL;

	if(Index < 0 || Index >= Array->Size)
		return NULL;
	Animal = Array->Table[Index];
	if(Array->Size <= Index + 1) {
		--Array->Size;
		return Animal;
	}
	PopType = Animal->PopType;
	for(int i = Index + 1; i < Array->Size; ++i) {
		if(PopType != ((struct Animal*)Array->Table[i])->PopType) {
			Array->Table[Index] = Array->Table[i - 1];
			Index = i - 1;
			PopType = ((struct Animal*)Array->Table[i])->PopType;
		}
	}
	--Array->Size;
	Array->Table[Index] = Array->Table[Array->Size];
	return Animal;
}

int CountAnimal(const struct Population* PopType, const struct Animal** List, size_t ListSz) {
	int AnCt = 0;

	for(int i = 0; i < ListSz; ++i) {
		if(List[i]->PopType == PopType)
			++AnCt;
	}
	return AnCt;
}

int AnimalsReproduce(const struct Population* Population, int MaleCt, int FemaleCt) {
	int NewAnimals = 0;

	if(MaleCt * Population->FMRatio < FemaleCt) {
		FemaleCt = MaleCt * Population->FMRatio;
	}
	for(int i = 0; i < FemaleCt; ++i) {
		int Temp = Random(Population->ReproduceRate.Min, Population->ReproduceRate.Max);
		int Remain = 0;

		NewAnimals = Temp / 100;
		if((Remain = (Temp % 100)) != 0 && Random(1, 100) >= Remain) {
			++NewAnimals;
		}
	}
	return NewAnimals;
}
