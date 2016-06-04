/**
 * Author: David Brotz
 * File: Plot.c
 */
#include "Plot.h"

#include "BigGuy.h"
#include "Date.h"
#include "World.h"
#include "Person.h"

#include "sys/Math.h"
#include "sys/Event.h"

#include <stdlib.h>
#include <assert.h>

#define PLOT_CURRACTLIST(_Plot) ((_Plot)->ActionList[(_Plot)->CurrActList])
#define PLOT_PREVACTLIST(_Plot) ((_Plot)->ActionList[(_Plot)->CurrActList == 0])
#define PLOT_SWAPACTLIST(_Plot) ((_Plot)->CurrActList = (_Plot)->CurrActList == 0)

static const char* g_PlotTypeStr[] = {
	"Overthrow",
	"Pass Policy",
	"Remove Policy"
};	

const char* PlotTypeStr(const struct Plot* _Plot) {
	return g_PlotTypeStr[_Plot->PlotType];
}

struct Plot* CreatePlot(int _Type, struct BigGuy* _Owner, struct BigGuy* _Target) {
	struct Plot* _Plot = (struct Plot*) malloc(sizeof(struct Plot));

	assert(_Type >=0 && _Type < PLOT_SIZE);
	assert(_Owner != NULL);
	_Plot->PlotType = _Type;
	ConstructLinkedList(&_Plot->Side[0]);
	ConstructLinkedList(&_Plot->SideAsk[0]);
	ConstructLinkedList(&_Plot->Side[1]);
	ConstructLinkedList(&_Plot->SideAsk[1]);
	ConstructLinkedList(&_Plot->ActionList[0]);
	ConstructLinkedList(&_Plot->ActionList[1]);
	LnkLstPushBack(&_Plot->Side[PLOT_ATTACKERS], _Owner);
	LnkLstPushBack(&_Plot->Side[PLOT_DEFENDERS], _Target);
	_Plot->Threat[PLOT_ATTACKERS] = 0;
	_Plot->Threat[PLOT_DEFENDERS] = 0;
	_Plot->WarScore = 0;
	_Plot->MaxScore = PLOT_OVERTHROW_MAXSCORE;
	_Plot->CurrActList = 0;
	CreateObject((struct Object*) _Plot, OBJECT_PLOT, (ObjectThink) PlotThink);
	PushEvent(EVENT_NEWPLOT, BigGuyHome(_Owner), _Plot);
	if(_Target != NULL)
		BigGuyPlotTarget(_Target, _Plot);
	RBInsert(&g_GameWorld.PlotList, _Plot);
	return _Plot;
}

void DestroyPlot(struct Plot* _Plot) {
	DestroyObject((struct Object*)_Plot);
	free(_Plot);
}

int PlotCanAsk(const struct Plot* _Plot, int _Side, struct BigGuy* _Guy) {
	struct LnkLst_Node* _Itr = NULL;

	_Itr = _Plot->SideAsk[_Side].Front;
	while(_Itr != NULL) {
		if(((struct BigGuy*)_Itr->Data) == _Guy)
			return 0;
		_Itr = _Itr->Next;
	}
	return 1;
}

void PlotJoin(struct Plot* _Plot, int _Side, struct BigGuy* _Guy) {
	assert(PlotCanAsk(_Plot, _Side, _Guy) == 1);
	assert(_Side == PLOT_ATTACKERS || _Side == PLOT_DEFENDERS);
	LnkLstPushBack(&_Plot->Side[_Side], _Guy);
}

int PlotInsert(const struct Plot* _One, const struct Plot* _Two) {
	return PlotLeader(_One)->Id - PlotLeader(_Two)->Id;
}

int PlotSearch(const struct BigGuy* _One, const struct Plot* _Two) {
	return _One->Id - PlotLeader(_Two)->Id;
}

int IsInPlot(const struct Plot* _Plot, struct BigGuy* _Guy) {
	struct LnkLst_Node* _Itr = NULL;

	_Itr = _Plot->Side[0].Front;
	while(_Itr != NULL) {
		if(_Itr->Data == _Guy)
			return 1;
		_Itr = _Itr->Next;	
	}
	_Itr = _Plot->Side[1].Front;
	while(_Itr != NULL) {
		if(_Itr->Data == _Guy)
			return 2;
		_Itr = _Itr->Next;	
	}
	return 0;
}

int PlotWarScoreMods(struct Plot* _Plot, struct BigGuy* _Guy, struct LinkedList* _Actions) {
	struct LnkLst_Node* _Itr = _Actions->Front;
	struct PlotAction* _Action = NULL;
	int _ActCt = 0;
	int _ActDone = 0;

	while(_Itr != NULL) {
		_Action = (struct PlotAction*) _Itr->Data;
		if(_Action->Actor != _Guy)
			goto loop_end;
		++_ActCt;
		++_ActDone;
		switch( _Action->Type) {
			case PLOTACT_PREVENT:
				_Action->DmgDelt = 1;
			break;
			case PLOTACT_DOUBLEDMG:
				if(BigGuySkillCheck(_Guy, BGSKILL_WARFARE, SKILLCHECK_DEFAULT) == 0)
					break;	
				_Action->DmgDelt = 2;
			break;
			case PLOTACT_REDUCETHREAT:
				if(BigGuySkillCheck(_Guy, BGSKILL_CHARISMA, SKILLCHECK_DEFAULT) == 0)
					break;
				_ActCt -= 2;
				--_ActDone;
			break;
		}
		loop_end:
		_Itr = _Itr->Next;
	};
	if(_ActDone == 0) {
		PlotAddAction(_Plot, PLOTACT_ATTACK, _Guy, NULL);
		((struct PlotAction*)PlotCurrActList(_Plot)->Back->Data)->DmgDelt = BigGuySkillCheck(_Guy, BGSKILL_INTRIGUE, SKILLCHECK_DEFAULT);
	}
	if(_ActCt < 0)
		_ActCt = 0;
	return _ActCt;
}

int PlotWarScore(struct Plot* _Plot, struct LinkedList* _List, struct LinkedList* _Actions, int* _Threat) {
	struct LnkLst_Node* _Itr = _List->Front; 
	struct BigGuy* _Guy = NULL;
	int _Score = 0;

	while(_Itr != NULL) {
		_Guy = _Itr->Data;
		*_Threat += PlotWarScoreMods(_Plot, _Guy, _Actions);
		_Itr = _Itr->Next;
	}

	_Itr = _List->Front; 
	while(_Itr != NULL) {
		struct PlotAction* _Action = NULL;

		for(struct LnkLst_Node* j = _Actions->Front; j != NULL; j = j->Next) {
			_Action = (struct PlotAction*) j->Data;
			if(_Action->Actor != _Guy)
				continue;
			_Score += _Action->DmgDelt;
		}
		_Itr = _Itr->Next;
	}
	return _Score;
}

void PlotThink(struct Plot* _Plot) {
	int _Diff = 0;
	int _ScoreDefender = 0;
	int _ScoreAttacker = 0;
	struct BigGuy* _Looser = NULL;

	if(DAY(g_GameWorld.Date) != 0)
		return;
	if(PlotTarget(_Plot) != NULL)
		_ScoreDefender = PlotWarScore(_Plot, &_Plot->Side[PLOT_DEFENDERS], &PLOT_CURRACTLIST(_Plot), &_Plot->Threat[PLOT_DEFENDERS]);
	_ScoreAttacker = PlotWarScore(_Plot, &_Plot->Side[PLOT_ATTACKERS], &PLOT_CURRACTLIST(_Plot), &_Plot->Threat[PLOT_ATTACKERS]);
	_Diff = _ScoreAttacker - _ScoreDefender; 
	_Plot->WarScore += _Diff;
	if(_Plot->WarScore <= -_Plot->MaxScore) {
		switch(_Plot->PlotType) {
			case PLOT_OVERTHROW:
				//PersonDeath(PlotLeader(_Plot)->Person);
			break;
		}
		_Looser = PlotLeader(_Plot);
		goto warscore_end;
	}
	if(_Plot->WarScore >= _Plot->MaxScore) {
		switch(_Plot->PlotType) {
			case PLOT_OVERTHROW:
				PushEvent(EVENT_NEWLEADER, PlotTarget(_Plot), PlotLeader(_Plot));
			break;
		}
		_Looser = PlotTarget(_Plot);
		goto warscore_end;
	}
	PLOT_SWAPACTLIST(_Plot);
	LnkLstClear(&PLOT_CURRACTLIST(_Plot));
	return;
	warscore_end:
	PushEvent(EVENT_ENDPLOT, _Plot, _Looser);
}

const struct LinkedList* PlotPrevActList(const struct Plot* _Plot) {
	return &PLOT_PREVACTLIST(_Plot);
}

const struct LinkedList* PlotCurrActList(const struct Plot* _Plot) {
	return &PLOT_CURRACTLIST(_Plot);	
}

struct BigGuy* PlotLeader(const struct Plot* _Plot) {
	return _Plot->Side[PLOT_ATTACKERS].Front->Data;
}

struct BigGuy* PlotTarget(const struct Plot* _Plot) {
	return _Plot->Side[PLOT_DEFENDERS].Front->Data;
}

int PlotOnSide(const struct Plot* _Plot, int _Side, const struct BigGuy* _Guy) {
	struct LnkLst_Node* _Itr = _Plot->Side[_Side].Front;

	assert(_Side == PLOT_ATTACKERS || _Side == PLOT_DEFENDERS);
	while(_Itr != NULL) {
		if(_Guy == _Itr->Data)
			return 1;
		_Itr = _Itr->Next;
	}
	return 0;
}

void PlotAddAction(struct Plot* _Plot, int _Type, struct BigGuy* _Actor, struct BigGuy* _Target) {
	int _ActorSide = 0;
	struct PlotAction* _Action = NULL;

	if(_Type < 0 || _Type >= PLOTACT_SIZE || _Actor == NULL || (_Actor == _Target))
		return;
	if(_Target != NULL) {
		if((_ActorSide = PlotOnSide(_Plot, PLOT_ATTACKERS, _Actor)) == PlotOnSide(_Plot, PLOT_ATTACKERS, _Target))
			return;
	}
	_Action = malloc(sizeof(struct PlotAction));
	_Action->Type = _Type;
	_Action->Actor = _Actor;
	_Action->ActorSide = _ActorSide;
	_Action->Target = _Target;
	_Action->DmgDelt = 0;
	(_ActorSide == PLOT_ATTACKERS)	
		? (LnkLstPushFront(&PLOT_CURRACTLIST(_Plot), _Action))
		: (LnkLstPushBack(&PLOT_CURRACTLIST(_Plot), _Action));
}	

int PlotGetThreat(const struct Plot* _Plot) {
	return Abs(_Plot->Threat[PLOT_ATTACKERS] - _Plot->Threat[PLOT_DEFENDERS]);
}

int PlotCanUseAction(const struct Plot* _Plot, const struct BigGuy* _Guy) {
	const struct LnkLst_Node* _Itr = PlotCurrActList(_Plot)->Front;
	struct PlotAction* _Action = NULL;

	while(_Itr != NULL) {
		_Action = (struct PlotAction*)_Itr->Data;
		if(_Action->Actor == _Guy)
			return 0;
		_Itr = _Itr->Next;
	}
	return 1;
}

void PlotSetTarget(struct Plot* _Plot, struct BigGuy* _Target) {
	LnkLstPushFront(&_Plot->Side[PLOT_DEFENDERS], _Target);
	BigGuyPlotTarget(_Target, _Plot);
}

void PlotActionEventStr(const struct PlotAction* _Action, char** _Buffer, size_t _Size) {
	#define __FUNC_EXTRASZ (128)

	char _Extra[__FUNC_EXTRASZ];

	switch(_Action->Type) {
		case PLOTACT_ATTACK:
			snprintf(*_Buffer, _Size, "%s(%i%%) did %i damage.", _Action->Actor->Person->Name, _Action->Actor->Stats[BGSKILL_INTRIGUE], _Action->DmgDelt);
		break;
		case PLOTACT_DOUBLEDMG:
			snprintf(_Extra, __FUNC_EXTRASZ, "delt double damage totaling %i", _Action->DmgDelt);
			snprintf(*_Buffer, _Size, "%s %s.", _Action->Actor->Person->Name, _Extra);
		break;
		case PLOTACT_PREVENT:
			snprintf(_Extra, __FUNC_EXTRASZ, "prevented %i damage done to ", _Action->DmgDelt);
			snprintf(*_Buffer, _Size, "%s %s %s.", _Action->Actor->Person->Name, _Extra, _Action->Target->Person->Name);
		break;
		case PLOTACT_REDUCETHREAT:
			snprintf(*_Buffer, _Size, "%s reduced threat.", _Action->Actor->Person->Name);
		break;
		default:
		break;
	}
#undef __FUNC_EXTRASZ
}
