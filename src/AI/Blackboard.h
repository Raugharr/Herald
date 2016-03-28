/*
 * File: Blackboard.h
 * Author: David Brotz
 */
#ifndef __BLACKBOARD_H
#define __BLACKBOARD_H

struct BigGuy;

struct Blackboard {
	struct BigGuy* Target;	
	int ShouldReplan;
};

void InitBlackboard(struct Blackboard* _Blackboard);
void BlackboardClear(struct Blackboard* _Blackboard);

#endif
