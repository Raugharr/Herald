/*
 * File: Government.c
 * Author: David Brotz
 */

#include "Government.h"

#include "Location.h"
#include "BigGuy.h"
#include "sys/Log.h"

struct Government* CreateGovernment(int _GovType) {
	struct Government* _Gov = (struct Government*) malloc(sizeof(struct Government));

	return _Gov;
}

int GovernmentLeaderElection(const struct Reform* _Reform, struct Settlement* _Settlement) {
	struct LnkLst_Node* _Itr = _Settlement->BigGuys.Front;
	struct BigGuy* _Best = NULL;
	int _BestAuth = 0;

	while(_Itr != NULL) {
		if(((struct BigGuy*)_Itr->Data)->Authority > _BestAuth) {
			_Best = (struct BigGuy*)_Itr->Data;
			_BestAuth = ((struct BigGuy*)_Itr->Data)->Authority;
		}
		_Itr = _Itr->Next;
	}
	if(_Best == NULL) {
		Log(ELOG_WARNING, "GovernmentLeaderElection has no leader possible.");
		return 1;
	}
	_Settlement->Government->Leader = _Best;
	return 1;
}
