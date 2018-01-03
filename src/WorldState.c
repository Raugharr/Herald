/*
 * File: WorldState.c
 * Author: David Brotz
 */

#include "WorldState.h"

#include "sys/Math.h"

#include <limits.h>

void WorldStateCopy(struct WorldState* To, const struct WorldState* From) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		To->State[i] = From->State[i];
		To->DontCare[i] = From->DontCare[i];
		To->OpCode[i] = From->OpCode[i];
	}
}

void WorldStateClear(struct WorldState* State) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		State->State[i] = 0;
		State->DontCare[i] = -1;
		State->OpCode[i] = 0;
		//_State->OpCode[i] = 0x0101010101010101;
	}
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			State->OpCode[i] = State->OpCode[i] | (WSOP_ADD << (sizeof(WorldState_t) * j * 2));
		}
	}
}

void WorldStateSetState(struct WorldState* To, const struct WorldState* From) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
			To->State[i] = From->State[i];
		}
}

void WorldStateSetDontCare(struct WorldState* To, const struct WorldState* From) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		To->DontCare[i] = From->DontCare[i];
	}
}

void WorldStateCare(struct WorldState* State) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		State->DontCare[i] = 0;
	}
}

/*
 * TODO: Prevent integer overflow for the individual bytes.
 */
void WorldStateAdd(struct WorldState* To, const struct WorldState* From) {
	WorldStateAtom_t FromAtom = 0;
	WorldStateAtom_t ToAtom = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(((From->DontCare[i] >> (j * CHAR_BIT)) & 0xFF) == 0) {
				FromAtom = WSToByte(From, i, j);
				ToAtom = WSToByte(To, i, j);
				switch(((To->OpCode[i] >> STATEOPCODE_BITS) & STATEOPCODE_MAX)) {
				case WSOP_ADD:
					ToAtom = ToAtom + FromAtom;
					break;
				case WSOP_SUB:
					ToAtom = ToAtom - FromAtom;
					break;
				case WSOP_MULT:
					ToAtom = ToAtom * FromAtom;
					break;
				case WSOP_DIV:
					ToAtom = ToAtom / FromAtom;
					break;
				}
				To->State[i] = To->State[i] | (((To->State[i] & (~To->DontCare[i])) & (~(0xFF << (j * CHAR_BIT)))) | (ToAtom << (j * CHAR_BIT)));
			}
		}
	}
}

void WorldStateSetAtom(struct WorldState* State, int Atom, int Value) {
	int Idx = WSGetIndex(Atom);
	int Offset = WSAtomOffset(Atom);

	if(Idx >= WORLDSTATESZ)
		return;
	State->DontCare[Idx] = State->DontCare[Idx] /*^*/ & (~(0xFF << Offset));
	State->State[Idx] = (Value == 0) ? (State->State[Idx] & (~(0xFF << Idx))) : (State->State[Idx] | ((0xFF & Value) << Offset));
}

void WorldStateAddAtom(struct WorldState* State, int Atom, int Value) {
	int Idx = WSGetIndex(Atom);
	int Offset = WSAtomOffset(Atom);

	if(Idx >= WORLDSTATESZ)
		return;
	State->DontCare[Idx] = State->DontCare[Idx] ^ (0xFF << Offset);
	Value += WSToByte(State, Idx, Atom);
	State->State[Idx] = (Value == 0) ? (State->State[Idx] & (~(0xFF << Idx))) : (State->State[Idx] | ((0xFF & Value) << Offset));
}

void WorldStateSetOpCode(struct WorldState* State, int Atom, int OpCode) {
	int Idx = WSGetIndex(Atom);
	int Offset = WSAtomOffset(Atom);
	int Temp = 0;

	if(Idx >= WORLDSTATESZ)
		return;
	Temp = ((State->OpCode[Idx] >> Offset) & STATEOPCODE_MAX);
	Temp = OpCode;
	State->OpCode[Idx] = ((State->OpCode[Idx] & (~(STATEOPCODE_MAX << Offset))) | ((STATEOPCODE_MAX & Temp) << Offset));
}

int WorldStateGetOpCode(const struct WorldState* State, int Atom) {
	int Idx = WSGetIndex(Atom);

	if(Idx >= WORLDSTATESZ)
		return 0;
	return WSAtomOpCode(State, Idx, Atom);
}

int WorldStateEqual(const struct WorldState* One, const struct WorldState* Two) {
	for(int i = 0; i < WORLDSTATESZ; ++i)
		if((One->State[i] & (~One->DontCare[i])) != (Two->State[i] & (~Two->DontCare[i])))
			return 0;
	return 1;
}

int WorldStateAtomCare(const struct WorldState* State, int Atom) {
	int Index = WSGetIndex(Atom);

	Atom = Atom - Index * sizeof(WorldState_t);
	return ~WSAtomDontCare(State, Index, Atom);
}

int WorldStateEmpty(const struct WorldState* State) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		if(State->DontCare[i] != INT_MIN)
			return 0;
	}
	return 1;
}

/*
 * FIXME: Instead of State being a composite of the two ffs, Check if the two WorldState's WorldStateFirstAtom are equal,
 * if they are then compare the values of the atom by encasing them into a byte sized variable.
 * Comparing by (One->State[i] & (~One->DontCare[i])) - (Two->State[i] & (~Two->DontCare[i])); might give undisered results as
 * if One's state is 0 and its ~DontCare is 1 while Two's State is 0 and its ~DontCare is 0 they will be considered equal.
 */
int WorldStateCmp(const struct WorldState* One, const struct WorldState* Two) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		int State = (One->State[i] & (~One->DontCare[i])) - (Two->State[i] & (~Two->DontCare[i]));

		if(State < 0)
			return -1;
		else if(State > 0)
			return 1;
	}
	return 0;
}

int WorldStateOpCmp(const struct WorldState* One, const struct WorldState* Two) {
	int Cmp = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if((Cmp = One->OpCode[i] - Two->OpCode[j]) != 0)
				return Cmp;
		}
	}
	return Cmp;
}

int WorldStateFirstAtom(const struct WorldState* State) {
	int Ct = 0;
	int ffs = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		ffs = ffs(~State->DontCare[i]);
		if(ffs == 0)
			Ct += sizeof(WorldState_t);
		else {
			return (Ct + ((ffs + CHAR_BIT) / CHAR_BIT));
		}
	}
	return 0;
}

void WorldStateClearAtom(struct WorldState* State, int Atom) {
	int Idx = WSGetIndex(Atom);
	int Offset = WSAtomOffset(Atom);

	if(Idx >= WORLDSTATESZ)
		return;
	State->State[Idx] = (State->State[Idx] & (~(0xFF << Offset)));
	State->DontCare[Idx] = (State->DontCare[Idx] ^ (0xFF << Offset));
}

int WorldStateDist(const struct WorldState* One, const struct WorldState* Two) {
	int Ct = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(WSAtomDontCare(One, i, j) != 0 || WSAtomDontCare(Two, i, j) != 0)
				continue;
			if(WSToByte(One, i, j) == WSToByte(Two, i, j))
				++Ct;
		}
	}
	return Ct;
}

int WorldStateTruth(const struct WorldState* Input, const struct WorldState* State) {
	int Check = -1;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(WSAtomDontCare(State, i, j) != 0)
				continue;
			/*
			 * NOTE: Will fail because the other opcodes have not been masked out.
			 */
			switch(WSAtomOpCode(Input, i, j)) {
			case WSOP_EQUAL:
				Check = WSToByte(Input, i, j) == WSToByte(State, i, j);
				break;
			case WSOP_GREATERTHAN:
				Check = WSToByte(Input, i, j) > WSToByte(State, i, j);
				break;
			case WSOP_GREATERTHANEQUAL:
				Check = WSToByte(Input, i, j) >= WSToByte(State, i, j);
				break;
			case WSOP_LESSTHAN:
				Check = WSToByte(Input, i, j) < WSToByte(State, i, j);
				break;
			case WSOP_LESSTHANEQUAL:
				Check = WSToByte(Input, i, j) <= WSToByte(State, i, j);
				break;
			}
			if(Check == 0)
				return 0;
		}
	}
	return Check;
}

int WorldStateTruthAtom(const struct WorldState* Input, const struct WorldState* State, int Atom) {
	int Idx = WSGetIndex(Atom);
	int Check = 0;

	Atom = Atom % sizeof(WorldState_t);
	if(Idx >= WORLDSTATESZ)
		return 0;
	switch(WSAtomOpCode(State, Idx, Atom)) {
		case WSOP_EQUAL:
			Check = WSToByte(Input, Idx, Atom) == WSToByte(State, Idx, Atom);
			break;
		case WSOP_GREATERTHAN:
			Check = WSToByte(Input, Idx, Atom) > WSToByte(State, Idx, Atom);
			break;
		case WSOP_GREATERTHANEQUAL:
			Check = WSToByte(Input, Idx, Atom) >= WSToByte(State, Idx, Atom);
			break;
		case WSOP_LESSTHAN:
			Check = WSToByte(Input, Idx, Atom) < WSToByte(State, Idx, Atom);
			break;
		case WSOP_LESSTHANEQUAL:
			Check = WSToByte(Input, Idx, Atom) <= WSToByte(State, Idx, Atom);
			break;
		}
	return Check;
}

int WSDntCrCmp(const struct WorldState* One, const struct WorldState* Two) {
	int DCOne = 0;
	int DCTwo = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			DCOne = WSAtomDontCare(One, i, j);
			DCTwo = WSAtomDontCare(Two, i, j);
			if(DCOne == 0 && DCTwo != 0)
				return -1;
			else if(DCTwo == 0 && DCOne != 0)
				return 1;
		}
	}
	return 0;
}

int WSDntCrComp(const struct WorldState* State) {
	int NewState = 0;

	for(int i = 0; i < sizeof(WorldState_t); ++i) {
		for(int j = 0; j < WORLDSTATESZ; ++j) {
			NewState = NewState | ((((0xFF << (i * CHAR_BIT)) & State->DontCare[j]) == 0) << (i + (sizeof(WorldState_t) * j)));
		}
	}
	return NewState;
}

