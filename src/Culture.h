/**
 * File: Culture.h
 * Author David Brotz
 */

#ifndef __CULTURE_H
#define __CULTURE_H

#include "Good.h"
#include "Warband.h"

#include <stdint.h>

struct WeaponBase;

struct WarRoleRule {
	uint8_t Rule;
	uint8_t Quantity;
	uint8_t GoodCat;
	uint8_t Type;
	uint8_t Rarity; //Chance for this role to start with this type of equipment.
};


struct Culture {
	struct ArmsBase* Arms[ARMS_SIZE];
	struct WarRoleRule* WarriorRules[WARROLE_SIZE];
};

#endif
