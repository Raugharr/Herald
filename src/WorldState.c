/*
 * File: WorldState.c
 * Author: David Brotz
 */

#include "WorldState.h"

#include "sys/Math.h"

#define StateToByte(_State, _Idx, AtomIdx) ((((_State)->State[(_Idx)] & (~(_State)->DontCare[(_Idx)]))) >> ((AtomIdx) * CHAR_BITS) & 0xFF)
#define StateAtomOffset(_Atom) (((_Atom) % sizeof(WorldState_t)) * CHAR_BITS)
#define StateAtomOpCode(_State, _Idx, _Atom) ((((_State)->OpCode[(_Idx)]) & (STATEOPCODE_MAX << (_Atom * STATEOPCODE_BITS))) >> (_Atom * STATEOPCODE_BITS))
#define StateGetIndex(_Atom) ((_Atom) / sizeof(WorldState_t))
#define StateAtomDontCare(_State, _Idx, _Atom) ((0xFF << ((_Atom) * CHAR_BITS)) & (_State->DontCare[_Idx]))
#define STATEOPCODE_BITS (8)
#define STATEOPCODE_MAX (0xFF)

void WorldStateCopy(struct WorldState* _To, const struct WorldState* _From) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		_To->State[i] = _From->State[i];
		_To->DontCare[i] = _From->DontCare[i];
		_To->OpCode[i] = _From->OpCode[i];
	}
}

void WorldStateClear(struct WorldState* _State) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		_State->State[i] = 0;
		_State->DontCare[i] = -1;
		_State->OpCode[i] = 0x0101010101010101;
	}
}

void WorldStateSetState(struct WorldState* _To, const struct WorldState* _From) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
			_To->State[i] = _From->State[i];
		}
}

void WorldStateSetDontCare(struct WorldState* _To, const struct WorldState* _From) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		_To->DontCare[i] = _From->DontCare[i];
	}
}

void WorldStateCare(struct WorldState* _State) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		_State->DontCare[i] = 0;
	}
}

/*
 * TODO: Prevent integer overflow for the individual bytes.
 */
void WorldStateAdd(struct WorldState* _To, const struct WorldState* _From) {
	WorldStateAtom_t _FromAtom = 0;
	WorldStateAtom_t _ToAtom = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(((_From->DontCare[i] >> (j * CHAR_BITS)) & 0xFF) == 0) {
				_FromAtom = StateToByte(_From, i, j);
				_ToAtom = StateToByte(_To, i, j);
				switch(((_To->OpCode[i] >> STATEOPCODE_BITS) & STATEOPCODE_MAX)) {
				case WSOP_ADD:
					_ToAtom = _ToAtom + _FromAtom;
					break;
				case WSOP_SUB:
					_ToAtom = _ToAtom - _FromAtom;
					break;
				case WSOP_MULT:
					_ToAtom = _ToAtom * _FromAtom;
					break;
				case WSOP_DIV:
					_ToAtom = _ToAtom / _FromAtom;
					break;
				}
				_To->State[i] = _To->State[i] | (((_To->State[i] & (~_To->DontCare[i])) & (~(0xFF << (j * CHAR_BITS)))) | (_ToAtom << (j * CHAR_BITS)));
			}
		}
	}
}

void WorldStateSetAtom(struct WorldState* _State, int _Atom, int _Value) {
	int _Idx = StateGetIndex(_Atom);
	int _Offset = StateAtomOffset(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return;
	_State->DontCare[_Idx] = _State->DontCare[_Idx] /*^*/ & (~(0xFF << _Offset));
	_State->State[_Idx] = (_Value == 0) ? (_State->State[_Idx] & (~(0xFF << _Idx))) : (_State->State[_Idx] | ((0xFF & _Value) << _Offset));
}

void WorldStateAddAtom(struct WorldState* _State, int _Atom, int _Value) {
	int _Idx = StateGetIndex(_Atom);
	int _Offset = StateAtomOffset(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return;
	_State->DontCare[_Idx] = _State->DontCare[_Idx] ^ (0xFF << _Offset);
	_Value += StateToByte(_State, _Idx, _Atom);
	_State->State[_Idx] = (_Value == 0) ? (_State->State[_Idx] & (~(0xFF << _Idx))) : (_State->State[_Idx] | ((0xFF & _Value) << _Offset));
}

void WorldStateSetOpCode(struct WorldState* _State, int _Atom, int _OpCode) {
	int _Idx = StateGetIndex(_Atom);
	int _Offset = StateAtomOffset(_Atom);
	int _Temp = 0;

	if(_Idx >= WORLDSTATESZ)
		return;
	_Temp = ((_State->OpCode[_Idx] >> _Offset) & STATEOPCODE_MAX);
	_Temp = _OpCode;
	_State->OpCode[_Idx] = ((_State->OpCode[_Idx] & (~(STATEOPCODE_MAX << _Offset))) | ((STATEOPCODE_MAX & _Temp) << _Offset));
}

int WorldStateGetOpCode(const struct WorldState* _State, int _Atom) {
	int _Idx = StateGetIndex(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return 0;
	return StateAtomOpCode(_State, _Idx, _Atom);
}

int WorldStateEqual(const struct WorldState* _One, const struct WorldState* _Two) {
	for(int i = 0; i < WORLDSTATESZ; ++i)
		if((_One->State[i] & (~_One->DontCare[i])) != (_Two->State[i] & (~_Two->DontCare[i])))
			return 0;
	return 1;
}

/*
 * FIXME: Instead of _State being a composite of the two ffs, Check if the two WorldState's WorldStateFirstAtom are equal,
 * if they are then compare the values of the atom by encasing them into a byte sized variable.
 */
int WorldStateCmp(const struct WorldState* _One, const struct WorldState* _Two) {
	int _State = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		_State = (_One->State[i] & (~_One->DontCare[i])) - (_Two->State[i] & (~_Two->DontCare[i]));
		if(_State < 0)
			return -1;
		else if(_State > 0)
			return 1;
	}
	return 0;
}

int WorldStateFirstAtom(const struct WorldState* _State) {
	int _Ct = 0;
	int _ffs = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		_ffs = ffs(~_State->DontCare[i]);
		if(_ffs == 0)
			_Ct += sizeof(WorldState_t);
		else {
			return (_Ct + ((_ffs + CHAR_BITS) / CHAR_BITS));
		}
	}
	return 0;
}

void WorldStateClearAtom(struct WorldState* _State, int _Atom) {
	int _Idx = StateGetIndex(_Atom);
	int _Offset = StateAtomOffset(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return;
	_State->State[_Idx] = (_State->State[_Idx] & (~(0xFF << _Offset)));
	_State->DontCare[_Idx] = (_State->DontCare[_Idx] ^ (0xFF << _Offset));
}

int WorldStateDist(const struct WorldState* _One, const struct WorldState* _Two) {
	int _Ct = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(StateAtomDontCare(_One, i, j) != 0 || StateAtomDontCare(_Two, i, j) != 0)
				continue;
			if(StateToByte(_One, i, j) == StateToByte(_Two, i, j))
				++_Ct;
		}
	}
	return _Ct;
}

int WorldStateTruth(const struct WorldState* _Input, const struct WorldState* _State) {
	int _Check = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(StateAtomDontCare(_State, i, j) != 0)
				continue;
			/*
			 * NOTE: Will fail because the other opcodes have not been masked out.
			 */
			switch(StateAtomOpCode(_State, i, j)) {
			case WSOP_EQUAL:
				_Check = StateToByte(_Input, i, j) == StateToByte(_State, i, j);
				break;
			case WSOP_GREATERTHAN:
				_Check = StateToByte(_Input, i, j) > StateToByte(_State, i, j);
				break;
			case WSOP_GREATERTHANEQUAL:
				_Check = StateToByte(_Input, i, j) >= StateToByte(_State, i, j);
				break;
			case WSOP_LESSTHAN:
				_Check = StateToByte(_Input, i, j) < StateToByte(_State, i, j);
				break;
			case WSOP_LESSTHANEQUAL:
				_Check = StateToByte(_Input, i, j) <= StateToByte(_State, i, j);
				break;
			}
			if(_Check == 0)
				return 0;
		}
	}
	return 1;
}

int WorldStateTruthAtom(const struct WorldState* _Input, const struct WorldState* _State, int _Atom) {
	int _Idx = StateGetIndex(_Atom);
	int _Check = 0;

	_Atom = _Atom % sizeof(WorldState_t);
	if(_Idx >= WORLDSTATESZ)
		return 0;
	switch(StateAtomOpCode(_State, _Idx, _Atom)) {
		case WSOP_EQUAL:
			_Check = StateToByte(_Input, _Idx, _Atom) == StateToByte(_State, _Idx, _Atom);
			break;
		case WSOP_GREATERTHAN:
			_Check = StateToByte(_Input, _Idx, _Atom) > StateToByte(_State, _Idx, _Atom);
			break;
		case WSOP_GREATERTHANEQUAL:
			_Check = StateToByte(_Input, _Idx, _Atom) >= StateToByte(_State, _Idx, _Atom);
			break;
		case WSOP_LESSTHAN:
			_Check = StateToByte(_Input, _Idx, _Atom) < StateToByte(_State, _Idx, _Atom);
			break;
		case WSOP_LESSTHANEQUAL:
			_Check = StateToByte(_Input, _Idx, _Atom) <= StateToByte(_State, _Idx, _Atom);
			break;
		}
	return _Check;
}
