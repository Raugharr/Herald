/**
 * Author: David Brotz
 * File: BigGuyRelation.h
 */

#ifndef __BIGGUY_RELATION_H
#define __BIGGUY_RELATION_H
enum {
	BGREL_HATE,
	BGREL_DISLIKE,
	BGREL_NEUTURAL,
	BGREL_LIKE,
	BGREL_LOVE
};

enum {
	OPINION_NONE,
	OPINION_TOKEN,
	OPINION_SMALL,
	OPINION_AVERAGE,
	OPINION_GREAT
};

enum {
	ACTTYPE_THEFT,
	ACTTYPE_RUMOR,
	ACTTYPE_TRAIT,
	ACTTYPE_RAISEFYRD,
	ACTTYPE_ATTACK,
	ACTTYPE_GIFT,
	ACTTYPE_WARLACK,
	ACTTYPE_SIZE
};

struct BigGuyOpinion {
	int Action;
	int RelMod;
	struct BigGuyOpinion* Next;
};

struct BigGuyRelation {
	int Relation;
	int Modifier;
	struct BigGuy* Person;
	struct BigGuyOpinion* Opinions;
	struct BigGuyRelation* Next;
};

struct BigGuyRelation* CreateBigGuyRelation(struct BigGuy* _Guy, struct BigGuy* _Actor);
struct BigGuyOpinion* CreateBigGuyOpinion(struct BigGuyRelation* _Relation, int _Action, int _Modifier);

void BigGuyAddRelation(struct BigGuy* _Guy, struct BigGuyRelation* _Relation, int _Action, int _Modifier);
void BigGuyChangeOpinion(struct BigGuy* _Guy, struct BigGuy* _Target, int _Action, int _Modifier);
/*
 * Recalculates the modifier variable of _Relation and then updates Relation if applicable.
 */
void BigGuyRelationUpdate(struct BigGuyRelation* _Relation);
struct BigGuyRelation* BigGuyGetRelation(const struct BigGuy* _Guy, const struct BigGuy* _Target);
#endif
