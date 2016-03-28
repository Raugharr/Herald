/*
 * File: Blackboard.c
 * Author: David Brotz
 */

#include "Blackboard.h"

#include <stdlib.h>

static inline void BlackboardWipe(struct Blackboard* _Blackboard) {
	_Blackboard->Target = NULL;
}

void InitBlackboard(struct Blackboard* _Blackboard) {
	BlackboardWipe(_Blackboard);
	_Blackboard->ShouldReplan = 1;
}

void BlackboardClear(struct Blackboard* _Blackboard) {
	BlackboardWipe(_Blackboard);
	_Blackboard->ShouldReplan = 0;
}
