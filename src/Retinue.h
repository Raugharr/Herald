/**
 * File: Retinue.h
 * Author: David Brotz
 */

#ifndef __RETINUE_H
#define __RETINUE_H

#include "Herald.h"

#include "sys/Array.h"
#include "sys/Event.h"

#define RETINUE_RECRUITMOD (-30)

struct BigGuy;
struct Person;
struct GameWorld;

struct Retinue {
	struct Object Object;
	struct BigGuy* Leader;
	struct Array Warriors;
	struct Array Children;//Retinues that pay homage to this one.
	struct Array EquipNeed;
	struct Settlement* Home;
	struct Retinue* Parent;
	int8_t IsRecruiting;
};

void RetinueGetPos(const void* One, SDL_Point* Pos);
/**
 * Creates a retinue and adds it to the World list of retinues.
 */
struct Retinue* CreateRetinue(struct BigGuy* Leader, struct GameWorld* World);
void DestroyRetinue(struct Retinue* Retinue, struct GameWorld* World);

/**
 *\brief Determines if Warrior is already in Retinue.
 *\return a positive non-zero integer if Warrior is in the retinue and 0 if he is not.
 */
static inline int RetinueIsWarrior(const struct Retinue* Retinue, const struct Person* Warrior) {
	return (BinarySearch(Warrior, Retinue->Warriors.Table, Retinue->Warriors.Size, ObjectCmp) != 0);
}
/**
 * \brief Adds Warrior to Retinue if they are of the warrior caste and not
 * already in this retinue.
 */
void RetinueAddWarrior(struct Retinue* Retinue, struct Person* Warrior);
void RetinueRemoveWarrior(struct Retinue* Retinue, struct Person* Warrior);
void RetinuePayWarriors(struct Retinue* Retinue);
void RetinueThink(struct Retinue* Retinue);
void RetinueAddChild(struct Retinue* Parent, struct Retinue* Child);
#endif
