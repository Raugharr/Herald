/**
 * Author: David Brotz
 * File: Policy.h
 */

#include "Policy.h"

#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>

void ConstructPolicy(struct Policy* _Policy, const char* _Name, const char* _Description, int _Category) {
	_Policy->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Policy->Description = calloc(strlen(_Description) + 1, sizeof(char));
	_Policy->Category = _Category;
	_Policy->OptionsSz = 0;
	for(int i = 0; i < POLICY_SUBSZ; ++i)
		_Policy->Options.Size[i] = POLICYCAT_UNUSED;
	for(int i = 0; i < POLICY_MAXOPTIONS; ++i) {
		for(int j = 0; j < CASTE_SIZE; ++j) {
			_Policy->Options.Options[i].CastePreference[j] = POLPREF_NETURAL;
		}
	}
	strcpy((char*)_Policy->Name, _Name);
	strcpy((char*)_Policy->Description, _Description);
}

void DestroyPolicy(struct Policy* _Policy) {
	for(int i = 0; i < POLICY_SUBSZ; ++i)
		free(_Policy->Options.Name[i]);
	free((char*) _Policy->Name);
	free((char*) _Policy->Description);
}

void PolicyAddOption(struct Policy* _Policy, int _Row, const char* _Name, PolicyOptFunc _CallFunc, PolicyOptUtility _Utility) {
	struct PolicyOption* _Opt = &_Policy->Options.Options[_Policy->OptionsSz];

	Assert(_Row < 0 || _Row > POLICY_SUBSZ || _Policy->Options.Size[_Row] == POLICYCAT_UNUSED);
	++_Policy->OptionsSz;
	++_Policy->Options.Size[_Row];
	_Opt->Name = calloc(sizeof(char), strlen(_Name) + 1);
	_Opt->OnPass= _CallFunc;
	_Opt->OnRemove = NULL;
	_Opt->Utility = _Utility;
	strcpy((char*) _Opt->Name, _Name);
}

void PolicyAddCategory(struct Policy* _Policy, const char* _Name) {
	int _Category = 0;

	for(_Category = 0; _Category < POLICY_SUBSZ; ++_Category) {
		if(_Policy->Options.Size[_Category] == POLICYCAT_UNUSED) {
			_Policy->Options.Size[_Category] = 0;	
			break;
		}
	}
	_Policy->Options.Name[_Category] = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Policy->Options.Name[_Category], _Name);
}

const struct PolicyOption* PolicyRow(const struct Policy* _Policy, int _Row, int _Col) {
	int _Idx = 0;

	Assert(_Row >= 0 && _Row < POLICY_SUBSZ);
	Assert(_Col >= 0 && _Col < POLICY_MAXOPTIONS);
	if(_Row == 0) {
		if(_Col >= _Policy->Options.Size[0])
			return NULL;
		return &_Policy->Options.Options[_Col];
	}
	for(int i = 1; i < POLICY_SUBSZ; ++i) {
		_Idx += _Policy->Options.Size[i];
		if(i == _Row) {
			if(_Col >= _Policy->Options.Size[i])
				return NULL;
			return &_Policy->Options.Options[_Idx + _Col];
		}
	}
	return NULL;
}

void DestroyPolicyOption(struct PolicyOption* _Opt) {
	free((char*) _Opt->Name);
}

const struct PolicyOption* PolicyChange(const struct ActivePolicy* _Policy) {
	for(int i = 0; i < POLICY_SUBSZ; ++i) {
		if(_Policy->OptionSel[i] != POLICYACT_IGNORE) {
			return PolicyRow(_Policy->Policy, i, _Policy->OptionSel[i]);
		}
	}
	return NULL;
}
