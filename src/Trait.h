#ifndef __TRAIT_H
#define __TRAIT_H

#include "sys/Array.h"

#define TraitLikes(_One, _Two) NArrayExists((const void**)(_One)->Likes, (_Two))
#define TraitDislikes(_One, _Two) NArrayExists((const void**)(_One)->Dislikes, (_Two))

typedef struct lua_State lua_State;

enum {
	TRAIT_MAN = (1 << 0),
	TRAIT_WOMAN = (1 << 1),
	TRAIT_CHILD = (1 << 2),
	TRAIT_ADULT = (1 << 3)
};

enum {
	TRAIT_MORAL,
	TRAIT_COHESION,
};

enum {
	TRAIT_ADD,
	TRAIT_SUB
};

struct TraitMod {
	uint8_t Type;
	uint8_t Effect;
};

struct Trait {
	uint32_t Id;
	uint32_t Chance; //Percentage between 0 and 2^32 - 1 to have a person have this trait.
	const char* Name;
	struct TraitMod** Mods;
	uint8_t ModSz;
	struct Trait** Likes;
	struct Trait** Dislikes;
	struct Trait** Prevents; //Traits a person cannot gain if they have this trait.
};

struct Trait* CreateTrait(const char* _Name);
void DestroyTrait(struct Trait* _Trait);

struct Trait* TraitLoad(lua_State* _State, int _Index);
void TraitLoadRelations(lua_State* _State, struct Trait* _Trait);
#endif
