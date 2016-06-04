/**
 * Author: David Brotz
 * File: BigGuyRelation.h
 */

#ifndef __BIGGUY_RELATION_H
#define __BIGGUY_RELATION_H

#define BIGGUY_RELMAX (100)
#define BIGGUY_RELMIN (-100)
#define BIGGUY_HATEMIN (-76)
#define BIGGUY_DISLIKEMIN (-26)
#define BIGGUY_LIKEMIN (25)
#define BIGGUY_LOVEMIN (75)
#define BIGGUY_TRAITREL (20)

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
/**
 * Recalculates the modifier variable of _Relation and then updates Relation if applicable.
 */
void BigGuyRelationUpdate(struct BigGuyRelation* _Relation);
struct BigGuyRelation* BigGuyGetRelation(const struct BigGuy* _Guy, const struct BigGuy* _Target);
int BigGuyRelAtLeast(const struct BigGuyRelation* _Rel, int _RelType);
/**
 * \return The value that repesents if the two people represented in the BigGuyRelation
 * like or hate each other.
 */
int BigGuyRelation(const struct BigGuyRelation* _Rel);
#endif
