/*
 * File: Profession.h 
 * Author: David Brotz
 */
#ifndef __PAYMENT_H
#define __PAYMENT_H

#include "sys/Array.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

/**
 * FIXME: Each PROF_* profession should have its own Profession* to prevent
 * code having special conditions for built in professions.
 */

struct GoodBase;
typedef struct lua_State lua_State;

enum PAYMENT {
	PAYMENT_MONEY = (1 << 0),
	PAYMENT_INKIND = (1 << 1),
	PAYMENT_SERVICE = (1 << 2)
};

enum {
	PROF_FARMER,
	PROF_WARRIOR,
	PROF_LUMBERJACK,
	PROF_MERCHANT,
	PROF_MINER,
	PROF_SIZE
};

enum {
	PROFTYPE_INPUT,
	PROFTYPE_OUTPUT,
	PROFTYPE_SPEC,
	PROFTYPE_SIZE
};

enum {
	PROF_FINPUT = (1 << 0),
	PROF_FOUTPUT = (1 << 1)
};

/**
 * How much time should be spent making a specific good.
 */
struct ProfSelector {
	const struct GoodBase* Good;
	uint16_t Rgood;
	uint8_t Ratio;
};

struct ProdOrder {
	const struct GoodBase* Good;
	uint16_t Quantity;
	uint16_t GoodIdx; //Index for good from either CraftedGoods or InputGoods.
	enum {
		ORDER_MAKE,
		ORDER_BUY
	} Type;
};

struct ProfTool {
	const struct GoodBase* Good;
	int Quantity;
};

struct Profession {
	uint8_t Id;
	uint8_t Type;
	const char* Name;
	const struct GoodBase** CraftedGoods;
	const struct GoodBase** InputGoods;
	const struct Profession** ProfReqs; //Professions this profession depends on. A cloth's maker depends on farmers for wool.
	const struct Profession** Parents; //A parent is a profession that uses a good this profession can create.
	/*
	 * CraftAmt is a table of ProdOrders.
	 * CraftAmt should be arranged as a topography so all the goods required for a single
	 * produced good should be before that good in the list. This also means that the same
	 * good can be in the list multiple times as an input to different produced goods.
	 */
	struct Array CraftAmt;
	const struct ProfTool* Tools; //Array of struct GoodBase*.
	struct ProdOrder* ProdList;
	uint32_t Payment;
	uint8_t CraftedGoodCt;
	uint8_t InputGoodCt;
	uint8_t ProfReqCt;
	//int8_t SelCt;
	uint8_t ToolSz;
	uint8_t ProdListSz;
	uint8_t ParentSz;
	uint8_t Flags;
};

struct ProfTool* CreateProfTool(const struct GoodBase* Good, int Quantity);

struct Profession* CreateProfession(int type, const char* Name, const struct GoodBase** CraftedGoods, int Payment);
void DestroyProfession(struct Profession* Profession);

struct Profession* ProfessionLoad(lua_State* State, int Index);
const struct GoodBase** LoadInKind(lua_State* State, int Index);
int ProfessionPaymentType(const char* Str);
//bool CanProduce(const struct Profession* Prof);
const struct Profession* GetProf(uint8_t Type);
/**
 * How many professions there are.
 */
uint8_t ProfCount();
/*
 * Size size of the returned list.
 */
//const struct Profession** ProfCanMake(const struct GoodBase* Base, uint8_t* Size);
//Returns -1 if the profession cannot make the Good, otherwise the good's index in the CraftedGoods array.
static inline int ProfCanMake(const struct Profession* Prof, const struct GoodBase* Good) {
	for(int i = 0; i < Prof->CraftedGoodCt; ++i) {
		if(Prof->CraftedGoods[i] == Good) return i;
	}
	return -1;
}

const char* ProfessionName(uint8_t Id);
static inline void ProfessionToolList(struct Profession* Prof, const struct ProfTool* Tools, uint8_t ToolSz) {
	Prof->Tools = calloc(ToolSz, sizeof(struct ProfTool));

	for(int i = 0; i < ToolSz; ++i) {
		*(struct ProfTool*)&Prof->Tools[i] = Tools[i];
	}
	Prof->ToolSz = ToolSz;
}
#endif
