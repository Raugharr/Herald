#ifndef __PAYMENT_H
#define __PAYMENT_H

struct GoodBase;
struct Behavior;
typedef struct lua_State lua_State;

enum PAYMENT {
	PAYMENT_MONEY = (1 << 0),
	PAYMENT_INKIND = (1 << 1),
	PAYMENT_SERVICE = (1 << 2)
};

struct Profession {
	int Type;
	const char* Name;
	double Markup; //How much to markup the price of the created goods.
	const struct GoodBase** CraftedGoods; //NULL terminated.
	const struct GoodBase** InKind;
	int Payment;
	struct Profession* SecondaryJob;
	struct Behavior* Behavior;
};

struct Profession* CreateProfession(const char* _Name, double _Markup, const struct GoodBase** _CraftedGoods, const struct GoodBase** _InKind, int _Payment);
void DestroyProfession(struct Profession* _Profession);

struct Profession* LoadProfession(lua_State* _State, int _Index);
const struct GoodBase** LoadInKind(lua_State* _State, int _Index);
int ProfessionPaymentType(const char* _Str);
#endif
