/**
 * Author: David Brotz
 * File: Retinue.c
 */

#include "Retinue.h"

#include "Person.h"
#include "BigGuy.h"
#include "Date.h"
#include "Location.h"

#include <stdlib.h>

struct Retinue* CreateRetinue(struct BigGuy* _Leader) {
	struct Retinue* _Retinue = malloc(sizeof(struct Retinue));

	_Retinue->Leader = _Leader;
	_Retinue->IsRecruiting = 0;
	_Retinue->FamilySz = 0;
	CtorArray(&_Retinue->Warriors, 8);
	_Retinue->RecruitMod = 0;
	return _Retinue;
}

void DestroyRetinue(struct Retinue* _Retinue) {
	free(_Retinue);
}

void RetinuePayWarriors(struct Retinue* _Retinue) {

}

void RetinueThink(struct Retinue* _Retinue) {
	if(NEWYEAR(g_GameWorld.Date) != 0) {
		RetinuePayWarriors(_Retinue);		
	}
	if(_Retinue->IsRecruiting != 0) {
		struct Settlement* _Home = PersonHome(_Retinue->Leader->Person);
		if(_Home->FreeWarriors.Size <= 0)
			goto not_recruit;
		//if(BigGuySkillCheck(_Retinue->Leader, BGSKILL_CHARISMA, SKILLCHECK_DEFAULT) == 0)
		//	goto not_recruit;
		RetinueAddWarrior(_Retinue, _Home->FreeWarriors.Front->Data);
		LnkLstPopFront(&_Home->FreeWarriors);	
	}
	not_recruit:
	return;
}

void RetinueAddWarrior(struct Retinue* _Retinue, struct Person* _Warrior) {
	if(RetinueIsWarrior(_Retinue, _Warrior) != 0 || _Retinue->Leader->Person == _Warrior)
		return;
	ArrayInsertSort_S(&_Retinue->Warriors, (void*)_Warrior, ObjectCmp);	
	_Retinue->FamilySz += FamilySize(_Warrior->Family);
	IntInsert(&g_GameWorld.PersonRetinue, _Warrior->Object.Id, _Retinue);
	PushEvent(EVENT_JOINRETINUE, _Warrior, _Retinue);
}

void RetinueRemoveWarrior(struct Retinue* _Retinue, struct Person* _Warrior) {
	for(int i = 0; i < _Retinue->Warriors.Size; ++i) {
		if(_Retinue->Warriors.Table[i] == _Warrior) {
			ArrayRemove(&_Retinue->Warriors, i);
		}
	}
	PushEvent(EVENT_QUITRETINUE, _Warrior, _Retinue);
}
