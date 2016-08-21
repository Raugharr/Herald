/*
 * File: ActionHelper.c
 * Author: David Brotz
 */

#include "ActionHelper.h"

#include "../Agent.h"

struct Policy* HatedPolicy(const struct Agent* _Actor) {
	struct Policy* _Policy = NULL;
	struct ActivePolicy* _ActPol = NULL;
	struct Settlement* _Settlement = _Actor->Agent->Person->Family->HomeLoc;
	int _Score = 0;
	int _TempScore = 0;
	int _Caste = _Actor->Agent->Person->Family->Caste;
	
	for(struct LnkLst_Node* _Itr = _Settlement->PolicyList; _Itr != NULL; _Itr = _Itr->Next) {
		_ActPol = _Itr->Data;
		_TempScore = _ActPol->Policy->Options.Options[_ActPol->OptionSel[0]]->CastePreference[_Caste];
		if(_TempScore > _Score) {
			_Policy = _ActPol->Policy;
			_Score = _TempScore;
		}
	}
	return _Policy;
}

int JoinPolicyPlot(struct Agent* _Agent, struct Plot* _Plot, struct ActivePolicy* _Policy) {
	int _Preference = PolicyRow(PolicyChange(_Policy))->CastePreference[PERSON_CASTE(_Agent->Agent)]
	int _Opinion = BigGuyGetRelation(_Agent->Agent, PlotLeader(_Plot))->Modifier;

	if(_Plot == NULL || _Policy == NULL)
		return -1;
	if(PlotTarget(_Plot) != NULL) {
		_Opinion += BigGuyGetRelation(_Agent->Agent, PlotTarget(_Plot))->Modifier;
	}
}
