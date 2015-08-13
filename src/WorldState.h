/*
 * File: WorldState.h
 * Author: David Brotz
 */
#ifndef __WORLDSTATE_H
#define __WORLDSTATE_H

#include <inttypes.h>

#define WORLDSTATE_ATOMSZ (16)
#define WORLDSTATESZ (WORLDSTATE_ATOMSZ / sizeof(WorldState_t))

typedef int /*int64_t*/ WorldState_t;
typedef int8_t WorldStateAtom_t;

enum {
	WSOP_NOT = 0,
	WSOP_EQUAL,
	WSOP_GREATERTHAN,
	WSOP_GREATERTHANEQUAL,
	WSOP_LESSTHAN,
	WSOP_LESSTHANEQUAL,
	WSOP_SET= 0,
	WSOP_ADD,
	WSOP_SUB,
	WSOP_MULT,
	WSOP_DIV
};

struct WorldState {
	WorldState_t State[WORLDSTATESZ];
	WorldState_t OpCode[WORLDSTATESZ];
	/*
	 int State[64]
	 int OpCode; //EQUAL GREATERTHAN LESSTHAN BOOLEAN
	 int Value;
	*/
	WorldState_t DontCare[WORLDSTATESZ];
};

/*
 * Copies _From to the WorldState _To.
 */
void WorldStateCopy(struct WorldState* _To, const struct WorldState* _From);
/*
 * Clears the WorldState _State.
 */
void WorldStateClear(struct WorldState* _State);
void WorldStateSetState(struct WorldState* _To, const struct WorldState* _From);
void WorldStateSetDontCare(struct WorldState* _To, const struct WorldState* _From);
/*
 * Set _State to care about all atoms.
 */
void WorldStateCare(struct WorldState* _State);
void WorldStateAdd(struct WorldState* _To, const struct WorldState* _From);
void WorldStateSetAtom(struct WorldState* _State, int _Atom, int _Value);
void WorldStateAddAtom(struct WorldState* _State, int _Atom, int _Value);
void WorldStateSetOpCode(struct WorldState* _State, int _Atom, int _OpCode);
int WorldStateGetOpCode(const struct WorldState* _State, int _Atom);
int WorldStateEqual(const struct WorldState* _One, const struct WorldState* _Two);
/*
 * Compares _One and _Two. Returns -1 if _One is less than _Two, 1 if _Two is less than _One, and 0 if they are equal.
 */
int WorldStateCmp(const struct WorldState* _One, const struct WorldState* _Two);
/*
 * Returns the index of the first atom that is not masked by _State's DontCare variable.
 */
int WorldStateFirstAtom(const struct WorldState* _State);
void WorldStateClearAtom(struct WorldState* _State, int _Atom);
/*
 * Returns the number of atoms that are not masked in both DontCares in _One and _Two that are not equal to each other.
 */
int WorldStateDist(const struct WorldState* _One, const struct WorldState* _Two);
int WorldStateTruth(const struct WorldState* _Input, const struct WorldState* _State);
int WorldStateTruthAtom(const struct WorldState* _Input, const struct WorldState* _State, int _Atom);

#endif
