#ifndef __PAYMENT_H
#define __PAYMENT_H

#include <inttypes.h>

struct GoodBase;
typedef struct lua_State lua_State;

enum PAYMENT {
	PAYMENT_MONEY = (1 << 0),
	PAYMENT_INKIND = (1 << 1),
	PAYMENT_SERVICE = (1 << 2)
};

struct Profession {
	uint32_t Type;
	const char* Name;
	const struct GoodBase** CraftedGoods;
	const struct GoodBase** InputGoods;
	uint32_t Payment;
	uint8_t CraftedGoodCt;
	uint8_t InputGoodCt;
};

struct Profession* CreateProfession(const char* _Name, const struct GoodBase** _CraftedGoods, int _Payment);
void DestroyProfession(struct Profession* _Profession);

struct Profession* LoadProfession(lua_State* _State, int _Index);
const struct GoodBase** LoadInKind(lua_State* _State, int _Index);
int ProfessionPaymentType(const char* _Str);
#endif
