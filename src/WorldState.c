/*
 * File: WorldState.c
 * Author: David Brotz
 */

#include "WorldState.h"

#include "sys/Math.h"

#include <limits.h>

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
				_FromAtom = WSToByte(_From, i, j);
				_ToAtom = WSToByte(_To, i, j);
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
	int _Idx = WSGetIndex(_Atom);
	int _Offset = WSAtomOffset(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return;
	_State->DontCare[_Idx] = _State->DontCare[_Idx] /*^*/ & (~(0xFF << _Offset));
	_State->State[_Idx] = (_Value == 0) ? (_State->State[_Idx] & (~(0xFF << _Idx))) : (_State->State[_Idx] | ((0xFF & _Value) << _Offset));
}

void WorldStateAddAtom(struct WorldState* _State, int _Atom, int _Value) {
	int _Idx = WSGetIndex(_Atom);
	int _Offset = WSAtomOffset(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return;
	_State->DontCare[_Idx] = _State->DontCare[_Idx] ^ (0xFF << _Offset);
	_Value += WSToByte(_State, _Idx, _Atom);
	_State->State[_Idx] = (_Value == 0) ? (_State->State[_Idx] & (~(0xFF << _Idx))) : (_State->State[_Idx] | ((0xFF & _Value) << _Offset));
}

void WorldStateSetOpCode(struct WorldState* _State, int _Atom, int _OpCode) {
	int _Idx = WSGetIndex(_Atom);
	int _Offset = WSAtomOffset(_Atom);
	int _Temp = 0;

	if(_Idx >= WORLDSTATESZ)
		return;
	_Temp = ((_State->OpCode[_Idx] >> _Offset) & STATEOPCODE_MAX);
	_Temp = _OpCode;
	_State->OpCode[_Idx] = ((_State->OpCode[_Idx] & (~(STATEOPCODE_MAX << _Offset))) | ((STATEOPCODE_MAX & _Temp) << _Offset));
}

int WorldStateGetOpCode(const struct WorldState* _State, int _Atom) {
	int _Idx = WSGetIndex(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return 0;
	return WSAtomOpCode(_State, _Idx, _Atom);
}

int WorldStateEqual(const struct WorldState* _One, const struct WorldState* _Two) {
	for(int i = 0; i < WORLDSTATESZ; ++i)
		if((_One->State[i] & (~_One->DontCare[i])) != (_Two->State[i] & (~_Two->DontCare[i])))
			return 0;
	return 1;
}

int WorldStateEmpty(const struct WorldState* _State) {
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		if(_State->DontCare[i] != INT_MIN)
			return 0;
	}
	return 1;
}

/*
 * FIXME: Instead of _State being a composite of the two ffs, Check if the two WorldState's WorldStateFirstAtom are equal,
 * if they are then compare the values of the atom by encasing them into a byte sized variable.
 * Comparing by (_One->State[i] & (~_One->DontCare[i])) - (_Two->State[i] & (~_Two->DontCare[i])); might give undisered results as
 * if _One's state is 0 and its ~DontCare is 1 while _Two's State is 0 and its ~DontCare is 0 they will be considered equal.
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

int WorldStateAtomCmp(const struct WorldState* _Input, const struct WorldState* _State) {
	int _Check = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(WSAtomDontCare(_State, i, j) != 0)
				continue;
			/*
			 * NOTE: Will fail because the other opcodes have not been masked out.
			 */
			switch(WSAtomOpCode(_State, i, j)) {
			case WSOP_EQUAL:
				_Check = WSToByte(_Input, i, j) - WSToByte(_State, i, j);
				break;
			case WSOP_GREATERTHAN:
				_Check = WSToByte(_Input, i, j) > WSToByte(_State, i, j);
				_Check = (_Check == 0);
				break;
			case WSOP_GREATERTHANEQUAL:
				_Check = WSToByte(_Input, i, j) >= WSToByte(_State, i, j);
				_Check = (_Check == 0);
				break;
			case WSOP_LESSTHAN:
				_Check = WSToByte(_Input, i, j) < WSToByte(_State, i, j);
				_Check = (_Check == 0);
				break;
			case WSOP_LESSTHANEQUAL:
				_Check = WSToByte(_Input, i, j) <= WSToByte(_State, i, j);
				_Check = (_Check == 0);
				break;
			}
			if(_Check != 0)
				return _Check;
		}
	}
	return 0;
}

int WorldStateOpCmp(const struct WorldState* _One, const struct WorldState* _Two) {
	int _Cmp = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if((_Cmp = _One->OpCode[i] - _Two->OpCode[j]) != 0)
				return _Cmp;
		}
	}
	return _Cmp;
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
	int _Idx = WSGetIndex(_Atom);
	int _Offset = WSAtomOffset(_Atom);

	if(_Idx >= WORLDSTATESZ)
		return;
	_State->State[_Idx] = (_State->State[_Idx] & (~(0xFF << _Offset)));
	_State->DontCare[_Idx] = (_State->DontCare[_Idx] ^ (0xFF << _Offset));
}

int WorldStateDist(const struct WorldState* _One, const struct WorldState* _Two) {
	int _Ct = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(WSAtomDontCare(_One, i, j) != 0 || WSAtomDontCare(_Two, i, j) != 0)
				continue;
			if(WSToByte(_One, i, j) == WSToByte(_Two, i, j))
				++_Ct;
		}
	}
	return _Ct;
}

int WorldStateTruth(const struct WorldState* _Input, const struct WorldState* _State) {
	int _Check = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(WSAtomDontCare(_State, i, j) != 0)
				continue;
			/*
			 * NOTE: Will fail because the other opcodes have not been masked out.
			 */
			switch(WSAtomOpCode(_State, i, j)) {
			case WSOP_EQUAL:
				_Check = WSToByte(_Input, i, j) == WSToByte(_State, i, j);
				break;
			case WSOP_GREATERTHAN:
				_Check = WSToByte(_Input, i, j) > WSToByte(_State, i, j);
				break;
			case WSOP_GREATERTHANEQUAL:
				_Check = WSToByte(_Input, i, j) >= WSToByte(_State, i, j);
				break;
			case WSOP_LESSTHAN:
				_Check = WSToByte(_Input, i, j) < WSToByte(_State, i, j);
				break;
			case WSOP_LESSTHANEQUAL:
				_Check = WSToByte(_Input, i, j) <= WSToByte(_State, i, j);
				break;
			}
			if(_Check == 0)
				return 0;
		}
	}
	return 1;
}

int WorldStateTruthAtom(const struct WorldState* _Input, const struct WorldState* _State, int _Atom) {
	int _Idx = WSGetIndex(_Atom);
	int _Check = 0;

	_Atom = _Atom % sizeof(WorldState_t);
	if(_Idx >= WORLDSTATESZ)
		return 0;
	switch(WSAtomOpCode(_State, _Idx, _Atom)) {
		case WSOP_EQUAL:
			_Check = WSToByte(_Input, _Idx, _Atom) == WSToByte(_State, _Idx, _Atom);
			break;
		case WSOP_GREATERTHAN:
			_Check = WSToByte(_Input, _Idx, _Atom) > WSToByte(_State, _Idx, _Atom);
			break;
		case WSOP_GREATERTHANEQUAL:
			_Check = WSToByte(_Input, _Idx, _Atom) >= WSToByte(_State, _Idx, _Atom);
			break;
		case WSOP_LESSTHAN:
			_Check = WSToByte(_Input, _Idx, _Atom) < WSToByte(_State, _Idx, _Atom);
			break;
		case WSOP_LESSTHANEQUAL:
			_Check = WSToByte(_Input, _Idx, _Atom) <= WSToByte(_State, _Idx, _Atom);
			break;
		}
	return _Check;
}

int WSDntCrCmp(const struct WorldState* _One, const struct WorldState* _Two) {
	int _DCOne = 0;
	int _DCTwo = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			_DCOne = WSAtomDontCare(_One, i, j);
			_DCTwo = WSAtomDontCare(_Two, i, j);
			if(_DCOne == 0 && _DCTwo != 0)
				return -1;
			else if(_DCTwo == 0 && _DCOne != 0)
				return 1;
		}
	}
	return 0;
}
