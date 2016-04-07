/*
 * File: Blackboard.h
 * Author: David Brotz
 */
#ifndef __BLACKBOARD_H
#define __BLACKBOARD_H

struct BigGuy;
struct Good;
struct Animal;
struct Building;
struct Location;

struct Blackboard {
	struct BigGuy* Target;	
	struct Good* Item;
	struct Animal* Animal;
	struct Building* Building;
	struct Location* Location;
	int ShouldReplan;
};

void InitBlackboard(struct Blackboard* _Blackboard);
void BlackboardClear(struct Blackboard* _Blackboard);

#endif
