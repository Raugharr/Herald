/**
 * Author: David Brotz
 * File: BigGuyRelation.h
 */

#ifndef __BIGGUY_RELATION_H
#define __BIGGUY_RELATION_H

#include "Herald.h"

#include "sys/Constraint.h"

#include <stdlib.h>
#include <stdint.h>

#define BIGGUY_RELMAX (100)
#define BIGGUY_RELMIN (-100)
#define BIGGUY_HATEMIN (-75)
#define BIGGUY_DISLIKEMIN (-25)
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
	OPINION_GREAT,
	OPINION_STATIC
};

enum {
	OPNLEN_SMALL,
	OPNLEN_MEDIUM,
	OPNLEN_LARGE,
	OPNLEN_FOREVER
};

enum {
	ACTTYPE_THEFT,
//	ACTTYPE_RUMOR,
	ACTTYPE_TRAIT,
	ACTTYPE_RAISEFYRD,
	ACTTYPE_ATTACK,
	ACTTYPE_GIFT,
	ACTTYPE_WARLACK,
	ACTTYPE_POLICY,
	ACTTYPE_SIZE
};

extern const char* g_BigGuyOpinionActions[ACTTYPE_SIZE];

struct BigGuyOpinion {
	struct BigGuyOpinion* Next;
	int8_t Action;
	int8_t ActionExtra; //Extra byte to declare a specific action such as a type of trait.
	int8_t RelMod;
	int8_t RelLength; //Filled with a value from the enum OPNLEN_* that determines how quickly this relation decays.
	int8_t RelStrength; //Filled with a value from the enum OPINION_* that determines how much opinion is required to change this opinion.
	int8_t MonthTick; //How many months have occured since the last RelMod change.
};

struct BigGuyRelation {
	int Relation;
	int Modifier;
	struct BigGuy* Person;
	struct BigGuyOpinion* Opinions;
	struct BigGuyRelation* Next;
};

struct BigGuyRelation* CreateBigGuyRelation(struct BigGuy* _Guy, struct BigGuy* _Actor);
/**
 * \brief Adds an opinion to _Relation. If an opinion already exists that has the Same parameters, then
 * _Modifier is appdended to that opinion otherwise a new opinion is created.
 * \param _Relation Which relation to modify.
 * \param _Action The action that has been done to justify the opinion change. Use ACTTYPE_* enumeration.
 * \param _RelLen How fast this opinion will decay. Use OPNLEN_* enumeration.
 * \param _Strength How difficult it is for opposing opinions to remove this opinion. Use OPINION_* enumeration.
 */
void BigGuyAddOpinion(struct BigGuy* _Guy, struct BigGuy* _Target, int _Action, int _Modifier, int _RelLen, int _Strength);
/**
 * \brief Does the same as BigGuyAddOpinion except is given a BigGuyRelation instead of two BigGuys.
 */
void BigGuyChangeRelation(struct BigGuyRelation* _Relation, int _Action, int _Modifier, int _RelLen, int _Strength);
/**
 * \brief Recalculates the modifier variable of _Relation and then updates Relation if applicable.
 */
void BigGuyRelationUpdate(struct BigGuyRelation* _Relation);
/**
 * \return The relation _Guy has with _Target if one exists or NULL if no relation exists.
 */
struct BigGuyRelation* BigGuyGetRelation(const struct BigGuy* _Guy, const struct BigGuy* _Target);
static inline int BigGuyRelAtLeast(const struct BigGuyRelation* _Rel, int _RelType) {
	if(_Rel == NULL)
		return 1;
		//return Fuzify(g_OpinionMods, _Rel->Modifier) <= BGREL_NEUTURAL;
	return Fuzify(g_OpinionMods, _Rel->Modifier) >= _RelType;
}

static inline int BigGuyRelAtMost(const struct BigGuyRelation* _Rel, int _RelType) {
	if(_Rel == NULL)
		return 1;
		//return Fuzify(g_OpinionMods, _Rel->Modifier) <= BGREL_NEUTURAL;
	return Fuzify(g_OpinionMods, _Rel->Modifier) <= _RelType;
}
/**
 * \return The value that repesents if the two people represented in the BigGuyRelation
 * like or hate each other.
 */
static inline int BigGuyRelation(const struct BigGuyRelation* _Rel) {
	return (_Rel == NULL) ? (0) : (_Rel->Modifier);
}
#endif
