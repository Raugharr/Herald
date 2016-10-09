/**
 * Author: David Brotz
 * File: Retinue.h
 */

#ifndef __RETINUE_H
#define __RETINUE_H

#include "Herald.h"

#include "sys/Array.h"
#include "sys/Event.h"

#define RETINUE_RECRUITMOD (-30)

struct BigGuy;
struct Person;

struct Retinue {
	struct BigGuy* Leader;
	struct Retinue* Next; //Implicit linked list of all retinues in a settlement.
	struct Array Warriors;
	int16_t RecruitMod;
	int16_t FamilySz;//The number of people in all the families of the retinue minus the leader.
	int8_t IsRecruiting;
};

/**
 *\note Should be added to a settlement by calling SettlementAddRetinue.
 */
struct Retinue* CreateRetinue(struct BigGuy* _Leader);
void DestroyRetinue(struct Retinue* _Retinue);

/**
 *\brief Determines if _Warrior is already in _Retinue.
 *\return a positive non-zero integer if _Warrior is in the retinue and 0 if he is not.
 */
static inline int RetinueIsWarrior(const struct Retinue* _Retinue, const struct Person* _Warrior) {
	return (BinarySearch(_Warrior, _Retinue->Warriors.Table, _Retinue->Warriors.Size, ObjectCmp) != 0);
}
/**
 * \brief Adds _Warrior to _Retinue if they are of the warrior caste and not
 * already in this retinue.
 */
void RetinueAddWarrior(struct Retinue* _Retinue, struct Person* _Warrior);
void RetinueRemoveWarrior(struct Retinue* _Retinue, struct Person* _Warrior);
void RetinuePayWarriors(struct Retinue* _Retinue);
void RetinueThink(struct Retinue* _Retinue);
#endif
