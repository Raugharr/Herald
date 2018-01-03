/*
 * File: Profession.c 
 * Author: David Brotz
 */
#include "Profession.h"

#include "Herald.h"

#include "sys/Array.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"
#include "sys/LuaCore.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <malloc.h>

static uint8_t g_ProfType = PROF_SIZE;
static struct Profession* g_ProfList[0xFF] = {0};

struct ProfTool* CreateProfTool(const struct GoodBase* Good, int Quantity) {
	struct ProfTool* Tool = malloc(sizeof(struct ProfTool));

	Tool->Good = Good;
	Tool->Quantity = Quantity;
	return Tool;
}

struct Profession* CreateProfession(int Type, const char* Name, const struct GoodBase** CraftedGoods, const struct GoodBase** ToolGoods, struct ProfSelector** Selectors, int Payment) {
	struct Profession* Profession = NULL;
	int GoodSz = 0;
	unsigned int InGdSz = 0;
	uint16_t* CraftAmt = NULL;

	GoodSz = ArrayLen(CraftedGoods);
	Profession = (struct Profession*) malloc(sizeof(struct Profession));
	Profession->Name = calloc(strlen(Name) + 1, sizeof(char));
	if(Type == -1)
		Profession->Id = g_ProfType++;
	else
		Profession->Id = Type;
	strcpy((char*)Profession->Name, Name);
	Profession->CraftedGoods = calloc(GoodSz + 1, sizeof(const struct GoodBase*));
	Profession->CraftedGoodCt = GoodSz;
	Profession->InputGoodCt = 0;
	Profession->ProfReqCt = 0;
	//Profession->SelCt = ArrayLen(Selectors);
	Profession->ProdListSz = 0;
	CtorArray(&Profession->Tools, 2);
	for(int i = 0; i < GoodSz; ++i) {
		Profession->CraftedGoods[i] = CraftedGoods[i];
	}
	if(ToolGoods != NULL) {
		for(int i = 0; ToolGoods[i] != NULL; ++i) {
			ArrayInsert_S(&Profession->Tools, (void*)ToolGoods[i]);
		}
	}
	Profession->Payment = Payment;
	Profession->CraftedGoods[GoodSz] = NULL;
	for(int i = 0; i < GoodSz; ++i) {
		InGdSz += CraftedGoods[i]->IGSize;
	}
	Profession->InputGoods = malloc(sizeof(struct GoodBase*) * InGdSz);
	for(int i = 0; i < GoodSz; ++i) {
		for(int j = 0; j < CraftedGoods[i]->IGSize; ++j) {
			Profession->InputGoods[Profession->InputGoodCt++] = CraftedGoods[i]->InputGoods[j]->Req;
		}
	}
	//Find all necessary professions required for the input goods.
	uint32_t Size = Profession->InputGoodCt;

	uint16_t ProdListSz = 0;

	ArrRmDup(Profession->InputGoods, &Size);
	Size = Profession->ProfReqCt;
	ArrRmDup(Profession->ProfReqs, &Size);
	g_ProfList[Profession->Id] = Profession;

	CtorArray(&Profession->CraftAmt, Profession->CraftedGoodCt);
	CraftAmt = alloca(sizeof(uint16_t) * Profession->CraftedGoodCt);
	for(int i = 0; i < Profession->CraftedGoodCt; ++i) {
		CraftAmt[i] = 1;
	}	
	//Establish selectors.
	/*for(int i = 0; i < Profession->CraftedGoodCt; ++i) {
		for(int j = 0; j < Profession->SelCt; ++j) {
			const struct ProfSelector* Sel = &Profession->SelArr[j];

			if(Sel->Good == Profession->CraftedGoods[i]) {
				if(Sel->Rgood != -1) {
					CraftAmt[i] += (CraftAmt[Sel->Rgood] * Sel->Ratio);
				}
			}
		}
		//Get ProdListSz
		ProdListSz += Profession->CraftedGoods[i]->IGSize + 1;
	}*/
	//Create enough input goods for output goods.
	/*
	 * For every good this profession can create we check if the profession can
	 * create any of the good's input goods. If the profession can then we add the amount
	 * needed to create the good in the profession's CraftAmt.
	 * FIXME: Will this work if you have a good that has a depth of 2 where the Profession
	 * can produce both of the goods? An example would be good x requires good y which requires
	 * good z where x y and z can all be produced by this profession.
	 */
	uint16_t GdOffset = 0;

	for(int i = 0; i < Profession->CraftedGoodCt; ++i) {
		ProdListSz += Profession->CraftedGoods[i]->IGSize + 1;
	}
	Profession->ProdListSz = ProdListSz;
	Profession->ProdList = calloc(ProdListSz, sizeof(struct ProdOrder));
	for(int i = 0; i < Profession->CraftedGoodCt; ++i) {
		const struct GoodBase* OutGood = Profession->CraftedGoods[i];
		struct ProdOrder* Order = &Profession->ProdList[GdOffset];
		struct Array* Array = CreateArray(1 + OutGood->IGSize);

		Order->Quantity = CraftAmt[i];
		Order->Good = OutGood;
		Order->Type = ORDER_MAKE;
		//ArrayInsert_S(&Profession->CraftAmt, Order);
		ArrayInsert_S(&Profession->CraftAmt, Array);
		for(int j = 0; j < OutGood->IGSize; ++j) {
			struct ProdOrder* Order = &Profession->ProdList[GdOffset + j + 1];

			Assert(GdOffset + j + 1 < ProdListSz);
			Order->Quantity = OutGood->InputGoods[j]->Quantity * CraftAmt[i];
			Order->Good = OutGood->InputGoods[j]->Req;
			Order->Type = ORDER_BUY;
			ArrayInsert_S(Array, Order);
		}
		ArrayInsert_S(Array, Order);
		GdOffset += OutGood->IGSize + 1;
	}
	/**
	 * Each input good found to be made by this profession is marked so in InGdFd.
	 * Now we need to reiterate everything to find goods this profession needs but 
	 * cannot produce so we can create a buy order for it.
	 */
	for(int i = 0; i < Profession->CraftedGoodCt; ++i) {
		const struct GoodBase* Base = Profession->CraftedGoods[i];

		for(int j = 0; j < ProdListSz; ++j) {
			struct ProdOrder* Prod = &Profession->ProdList[j];

			if(Prod->Good == Base) Prod->Type = ORDER_MAKE;
		}
	}
	return Profession;
}

void DestroyProfession(struct Profession* Profession) {
	free((char*)Profession->Name);
	free(Profession->CraftedGoods);
	free(Profession);
}

struct Profession* ProfessionLoad(lua_State* State, int Index) {
	struct LinkedList List = {0, NULL, NULL};
	const struct GoodBase* Good = NULL;
	int GoodSize = 0;
	const struct GoodBase** GoodList = NULL;
	const char* Name = NULL;
	struct Profession* Profession = NULL;
	struct ProfSelector** SelList = NULL;
	int Payment = 0;

	Index = lua_absindex(State, Index);
	if(lua_type(State, Index) != LUA_TTABLE)
		return NULL;
	lua_pushstring(State, "Name");
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TSTRING) {
		Log(ELOG_WARNING, "Profession's name is not a string.");
		return NULL;
	}
	Name = lua_tostring(State, -1);
	lua_pop(State, 1);

	lua_pushstring(State, "Goods");
	lua_rawget(State, Index);
	if(lua_type(State, -1) != LUA_TTABLE) {
		Log(ELOG_WARNING, "Profession %s's Good table is not a table.", Name);
		return NULL;
	}
	//Iterating through goods.
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(lua_type(State, -1) != LUA_TSTRING) {
			Log(ELOG_WARNING, "Profession %s's good table contains a non-string element.", Name);
			lua_pop(State, 1);
			continue;
		}
		if((Good = HashSearch(&g_Goods, lua_tostring(State, -1))) == NULL) {
			Log(ELOG_WARNING, "Profession %s's good table contains a non good %s.", Name, lua_tostring(State, -1));
			lua_pop(State, 1);
			continue;
		}
		LnkLstPushBack(&List, (struct GoodBase*) Good);
		lua_pop(State, 1);
	}
	lua_pop(State, 1);

	/*
	 * Iterating through selectors
	 */
	/*{
		int TblSz = 0;
		int Type = 0;
		int Ct = 0;
		const char* Str = NULL;
		const struct GoodBase* SelGood = NULL;
		struct ProfSelector* Selector = NULL;

		lua_pushstring(State, "Selectors");
		lua_rawget(State, Index);
		if((Type = lua_type(State, -1)) != LUA_TTABLE) {
			lua_pop(State, 1);
			goto skip_selectors;
		}
		TblSz = lua_rawlen(State, -1);
		SelList = alloca(sizeof(struct ProfSelector*) * (TblSz + 1));
		SelList[TblSz] = NULL;
			for(int i = 1; i <= TblSz; ++i) {
				lua_rawgeti(State, -1, i);
				if(lua_type(State, -1) != LUA_TTABLE) {
					lua_pop(State, 1);
					continue;
				}
				SelList[i - 1] = malloc(sizeof(struct ProfSelector));
				Selector = SelList[i - 1];
				lua_pushstring(State, "Good");
				lua_rawget(State, -2);
				if(lua_type(State, -1) != LUA_TSTRING) goto no_good;
				Str = lua_tostring(State, -1);
				Selector->Good = HashSearch(&g_Goods, Str);
				no_good:
				lua_pop(State, 1);

				lua_pushstring(State, "Rgood");
				lua_rawget(State, -2);
				if(lua_type(State, -1) != LUA_TSTRING) goto no_good;
				Str = lua_tostring(State, -1);
				SelGood = HashSearch(&g_Goods, Str);
				for(struct LnkLst_Node* Itr = List.Front; Itr != NULL; Itr = Itr->Next, ++Ct) {
					struct Good* Good = Itr->Data;
					
					if(GoodBaseCmp(Good, SelGood) == 0) {
						Selector->Rgood = Ct;
						goto selgood_found;
					}
				}
				goto no_rgood;
				selgood_found:
				lua_pop(State, 1);

				lua_pushstring(State, "Ratio");
				lua_rawget(State, -2);
				if(lua_type(State, -1) != LUA_TNUMBER) {
					Selector->Ratio = 2;
					goto no_rgood;
				}
				Selector->Ratio = lua_tointeger(State, -1);
				no_rgood:
				lua_pop(State, 1);
				lua_pop(State, 1);
			}
		lua_pop(State, 1);
	}*/
skip_selectors:
	/*
	 *
	 */
	GoodSize = List.Size;
	GoodList = alloca(sizeof(const struct GoodBase*) * (GoodSize + 1));
	for(int i = 0; i < GoodSize; ++i) {
		GoodList[i] = List.Front->Data;
		LnkLstPopFront(&List);
	}
	GoodList[GoodSize] = NULL;
	Profession = CreateProfession(-1, Name, GoodList, NULL, SelList, Payment);
	return Profession;
}

const struct GoodBase** LoadInKind(lua_State* State, int Index) {
	int Size = 0;
	const char* Key = NULL;
	struct LinkedList List = {0, NULL, NULL};
	const struct GoodBase* Good = NULL;
	const struct GoodBase** GoodList = NULL;

	Index = lua_absindex(State, Index);
	if(lua_type(State, Index) != LUA_TTABLE) {
		return NULL;
	}
	lua_pushnil(State);
	while(lua_next(State, Index) != 0) {
		if(lua_type(State, -1) != LUA_TSTRING) {
			goto loop_end;
		}
		Key = lua_tostring(State, -1);
		if((Good = HashSearch(&g_Goods, Key)) == NULL) {
			goto loop_end;
		} else {
			LnkLstPushBack(&List, (struct GoodBase*) Good);
		}
		loop_end:
		lua_pop(State, 1);
	}
	Size = List.Size;
	GoodList = calloc(Size + 1, sizeof(const struct GoodBase*));
	for(int i = 0; i < Size; ++i) {
		GoodList[i] = List.Front->Data;
		LnkLstPopFront(&List);
	}
	GoodList[Size] = NULL;
	return GoodList;
}

int ProfessionPaymentType(const char* Str) {
	if(strcmp(Str, "All") == 0) {
		return PAYMENT_MONEY | PAYMENT_INKIND | PAYMENT_SERVICE;			
	} else if(strcmp(Str, "Money") == 0) {
		return PAYMENT_MONEY;
	} else if(strcmp(Str, "InKind") == 0) {
		return PAYMENT_INKIND;
	} else if(strcmp(Str, "Service") == 0) {
		return PAYMENT_SERVICE;
	} else if(strcmp(Str, "NoService") == 0) {
		return PAYMENT_MONEY | PAYMENT_INKIND;
	}
	return 0;
}

bool CanProduce(const struct Profession* Prof, const struct GoodBase* Base) {
	for(int i = 0; i < Prof->CraftedGoodCt; ++i) {
		if(Prof->CraftedGoods[i] == Base) return true;
	}
	return false;
}

const struct Profession* GetProf(uint8_t Type) {
	return g_ProfList[Type];
}

uint8_t ProfCount() {return g_ProfType;}

const char* ProfessionName(uint8_t Id) {
/*	switch(Id) {
		case PROF_FARMER: return "Farmer";
		case PROF_WARRIOR: return "Warrior";
		case PROF_LUMBERJACK: return "Lumberjack";
		case PROF_MERCHANT: return "Merchant";
	}
	return g_ProfList[Id]->Name;*/
	return g_ProfList[Id]->Name;
}
