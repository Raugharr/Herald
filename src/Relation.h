/**
 * Author: David Brotz
 * File: BigGuyRelation.h
 */

#ifndef __RELATION_H
#define __RELATION_H

#include "Herald.h"

#include "sys/Constraint.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define REL_MAX (100)
#define REL_MIN (-100)
#define REL_HATEMIN (-75)
#define REL_DISLIKEMIN (-25)
#define REL_LIKEMIN (25)
#define REL_LOVEMIN (75)
#define REL_TRAIT (20)

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
	ACTTYPE_RAPE,
	ACTTYPE_MURDER,
	ACTTYPE_THEFT,
	ACTTYPE_GENERAL,
	ACTTYPE_TRAIT,
	ACTTYPE_RAISEFYRD,
	ACTTYPE_ATTACK,
	ACTTYPE_GIFT,
	ACTTYPE_WARLACK,
	ACTTYPE_POLICY,
	//Government relations.
	ACTTYPE_CULTURE,
	ACTTYPE_WAR,
	ACTTYPE_ENVOY,
	ACTTYPE_SIZE
};

extern const char* g_OpinionActions[ACTTYPE_SIZE];

struct Opinion {
	struct Opinion* Next;
	int8_t Action;
	int8_t ActionExtra; //Extra byte to declare a specific action such as a type of trait.
	int8_t RelMod;
	int8_t RelLength; //Filled with a value from the enum OPNLEN_* that determines how quickly this relation decays.
	int8_t RelStrength : 4; //Filled with a value from the enum OPINION_* that determines how much opinion is required to change this opinion.
	int8_t MonthTick : 4; //How many months have occured since the last RelMod change.
};

struct Relation {
	void* Target;
	struct Opinion* Opinions;
	struct Relation* Next;
	int16_t Modifier;
	uint8_t Relation; //Filled with a value from BGREL_*
};

struct Relation* CreateRelation(void* Owner, void* Target, struct Relation** Next);
/**
 * \brief Adds an opinion to Relation. If an opinion already exists that has the Same parameters, then
 * Modifier is appdended to that opinion otherwise a new opinion is created.
 * If Owner and Target currently have no existing relation a relation will be created and the opinion added to it.
 * \param Relation Which relation to modify.
 * \param Action The action that has been done to justify the opinion change. Use ACTTYPE_* enumeration.
 * \param RelLen How fast this opinion will decay. Use OPNLEN_* enumeration.
 * \param Strength How difficult it is for opposing opinions to remove this opinion. Use OPINION_* enumeration.
 */
void AddOpinion(void* Owner, void* Target, int Action, int Modifier, int RelLen, int Strength, struct Relation** Relation);
/**
 * \brief Does the same as AddOpinion except is given a Relation instead of two BigGuys.
 */
void ChangeRelation(struct Relation* Relation, int Action, int Modifier, int RelLen, int Strength);
/**
 * \brief Recalculates the modifier variable of Relation and then updates Relation if applicable.
 */
void RelUpdate(struct Relation* Relation);
/**
 * \return The relation Guy has with Target if one exists or NULL if no relation exists.
 * Relation is a pointer to the first Relation in the Relation list.
 */
struct Relation* GetRelation(struct Relation* Relation, const void* Target);
static inline bool RelAtLeast(const struct Relation* Rel, int RelType) {
	if(Rel == NULL)
		return true;
		//return Fuzify(g_OpinionMods, Rel->Modifier) <= BGREL_NEUTURAL;
	return Fuzify(g_OpinionMods, Rel->Modifier) >= RelType;
}

static inline bool RelAtMost(const struct Relation* Rel, int RelType) {
	if(Rel == NULL)
		return true;
		//return Fuzify(g_OpinionMods, Rel->Modifier) <= BGREL_NEUTURAL;
	return Fuzify(g_OpinionMods, Rel->Modifier) <= RelType;
}
/**
 * \return The value that repesents if the two people represented in the BigGuyRelation
 * like or hate each other.
 */
static inline int Relation(const struct Relation* Rel) {
	return (Rel == NULL) ? (0) : (Rel->Modifier);
}
#endif

