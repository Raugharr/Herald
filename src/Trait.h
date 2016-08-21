#ifndef __TRAIT_H
#define __TRAIT_H

#include "sys/Array.h"

#define TraitLikes(_One, _Two) NArrayExists((const void**)(_One)->Likes, (_Two))
#define TraitDislikes(_One, _Two) NArrayExists((const void**)(_One)->Dislikes, (_Two))

typedef struct lua_State lua_State;

struct Trait {
	int Id;
	const char* Name;
	struct Trait** Likes;
	struct Trait** Dislikes;
	struct Trait** Prevents; //Traits a person cannot gain if they have this trait.
};

struct Trait* CreateTrait(const char* _Name);
void DestroyTrait(struct Trait* _Trait);

struct Trait* TraitLoad(lua_State* _State, int _Index);
void TraitLoadRelations(lua_State* _State, struct Trait* _Trait);
#endif
