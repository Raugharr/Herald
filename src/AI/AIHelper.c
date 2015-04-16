/*
 * File: AIHelper.c
 * Author: David Brotz
 */

#include "AIHelper.h"

#include "../sys/Array.h"
#include "../sys/Log.h"
#include "../Good.h"
#include "../Person.h"
#include "../Family.h"
#include "../Crop.h"

#include <stdlib.h>
#include <string.h>

struct ActorJobBase g_ActorJobBases[] = {
		{ACTORJOB_EAT, ActorJobEat, ActorJobTempTime},
		{ACTORJOB_PLANT, ActorJobPlant, ActorJobTempTime},
		{ACTORJOB_HARVEST, ActorJobHarvest, ActorJobTempTime}
		};

int ActorJobTempTime(struct Actor* _Worker, struct Object* _NoObj, void* _NoVoid) {
	return 0;
}

int ActorJobEat(struct Actor* _Worker, struct ActorJobPair* _Pair) {
/*	struct Food* _FoodPtr = _Pair->Object;
	struct Person* _Person = (struct Person*)_Worker;
	int _Size = _Person->Family->Goods->Size;
	struct Good** _Tbl = (struct Good**)_Person->Family->Goods->Table;
	struct Food* _Food = (struct Food*) _FoodPtr;
	int _Nut = 0;
	int _NutReq = NUTRITION_LOSS;
	int _Div = 0;
	int i;

	for(i = 0; i < _Size; ++i) {
		_Food = (struct Food*)_Tbl[i];
		if(_Food->Base->Category != EFOOD)
			continue;
		_Div = _Food->Base->Nutrition / FOOD_MAXPARTS;
		while(_Nut < _NutReq && _Food->Quantity > 0 && _Food->Parts != 0) {
			if(_Food->Base->Nutrition > _NutReq) {
				--_Food->Parts;
				if(_Food->Parts <= 0) {
					if(_Food->Quantity > 0) {
						--_Food->Quantity;
						_Food->Parts = FOOD_MAXPARTS;
					}
				}
				_Nut += _Div;
			} else {
				if(_Food->Quantity == 0) {
					_Nut += _Food->Base->Nutrition  * (_Food->Parts / FOOD_MAXPARTS);
					_Food->Parts = 0;
				} else {
					_Nut += _Food->Base->Nutrition;
					--_Food->Quantity;
				}
			}
		}
	}
	if(_Nut == 0)
		Log(ELOG_WARNING, "Day %i: %i has no food to eat.", DateToDays(g_Date), _Person->Id);
	_Person->Nutrition += _Nut * (((double)3) / log10(_Person->Nutrition) + .15f);
	_Worker->Action = NULL;
	return 0;*/
}

int ActorJobPlant(struct Actor* _Worker, struct ActorJobPair* _Pair) {
	FieldPlant((struct Field*)_Pair->Object, _Pair->Extra);
	return 1;
}

int ActorJobHarvest(struct Actor* _Worker, struct ActorJobPair* _Pair) {
	FieldHarvest((struct Field*)_Pair->Object, (struct Array*)_Pair->Extra);
	return 1;
}
