/**
 * Author: David Brotz
 * File: Plot.c
 */
#include "Plot.h"

#include "BigGuy.h"
#include "Date.h"
#include "World.h"
#include "Person.h"
#include "Family.h"
#include "Location.h"
#include "Policy.h"

#include "sys/Math.h"
#include "sys/Event.h"

#include <stdlib.h>
#include <assert.h>

#define PLOT_CURRACTLIST(_Plot) ((_Plot)->ActionList[(_Plot)->CurrActList])
#define PLOT_PREVACTLIST(_Plot) ((_Plot)->ActionList[(_Plot)->CurrActList == 0])
#define PLOT_SWAPACTLIST(_Plot) ((_Plot)->CurrActList = (_Plot)->CurrActList == 0)
#define PlotDefenderWon(_Plot) ((_Plot)->WarScore <= -(_Plot)->MaxScore)
#define PlotAttackerWon(_Plot) ((_Plot)->WarScore >= (_Plot)->MaxScore)
#define PLOTACT_STATMOD (10)

static const char* g_PlotTypeStr[] = {
	"Overthrow",
	"Pass Policy",
	"Remove Policy"
};	

const char* PlotTypeStr(const struct Plot* _Plot) {
	return g_PlotTypeStr[_Plot->PlotType];
}

static struct {
	int SkillUsed;
	int Damage; 	
	const char* Name;
} g_PlotActionTypes[PLOTACT_SIZE] = {
	{BGSKILL_SIZE, 0, "None"},
	{BGSKILL_WIT, 1, "Attack"},
	{BGSKILL_WIT, 0, "Lower Stat"},
	{BGSKILL_COMBAT, 2, "Double Damage"},
	{BGSKILL_COMBAT, 1, "Double Attack"},
	{BGSKILL_WIT, 0, "Stop Attack"},
};

static inline int ActionDamage(const struct PlotAction* _Action) {
	return g_PlotActionTypes[_Action->Type].Damage;
	//return ((_Action->Flags & PLOTFLAG_HIT) == PLOTFLAG_HIT) ? (g_PlotActionTypes[_Action->Type].Damage) : (0);
}

struct PlotAction* CreatePlotAction(int _Type, const struct BigGuy* _Actor, int _ActorSide, struct PlotAction* _Next) {
	struct PlotAction* _Action = NULL;

	if(_Type < 0 || _Type >= PLOTACT_SIZE)
		return NULL;
	_Action = malloc(sizeof(struct PlotAction));
	*(int*)&_Action->Type = _Type;
	*(struct BigGuy**)&_Action->Actor = (struct BigGuy*)_Actor;
	*(int8_t*)&_Action->ActorSide = _ActorSide;
	*(const struct BigGuy**)&_Action->Target = NULL;
	*(struct PlotAction**)&_Action->Next = _Next;
	_Action->ActionStopped = NULL;
	return _Action;
}	

void DestroyPlotAction(struct PlotAction* _Action) {
	free(_Action);
}

struct Plot* CreatePlot(int _Type, void* _Data, struct BigGuy* _Owner, struct BigGuy* _Target) {
	struct Plot* _Plot = (struct Plot*) malloc(sizeof(struct Plot));

	assert(_Type >=0 && _Type < PLOT_SIZE);
	assert(_Owner != NULL);
	_Plot->PlotType = _Type;
	ConstructLinkedList(&_Plot->Side[0]);
	ConstructLinkedList(&_Plot->SideAsk[0]);
	ConstructLinkedList(&_Plot->Side[1]);
	ConstructLinkedList(&_Plot->SideAsk[1]);
	PlotJoin(_Plot, PLOT_ATTACKERS, _Owner);
	PlotJoin(_Plot, PLOT_DEFENDERS, _Target);
	_Plot->SidePower[PLOT_ATTACKERS] = 0;
	_Plot->SidePower[PLOT_DEFENDERS] = 0;
	_Plot->Threat[PLOT_ATTACKERS] = 0;
	_Plot->Threat[PLOT_DEFENDERS] = 0;
	_Plot->WarScore = 0;
	_Plot->MaxScore = PLOT_OVERTHROW_MAXSCORE;
	_Plot->CurrActList = 0;
	_Plot->PlotData = _Data;
	for(int i = 0; i < PLOT_SIDES; ++i) {
		for(int j = 0; j < BGSKILL_SIZE; ++j) {
			_Plot->StatMods[i][j] = 0;
		}
	}
	CreateObject((struct Object*) _Plot, OBJECT_PLOT, (ObjectThink) PlotThink);
	PushEvent(EVENT_NEWPLOT, BigGuyHome(_Owner), _Plot);
	if(_Target != NULL)
		BigGuyPlotTarget(_Target, _Plot);
	RBInsert(&g_GameWorld.PlotList, _Plot);
	PLOT_CURRACTLIST(_Plot) = NULL;
	PLOT_PREVACTLIST(_Plot) = NULL;
	return _Plot;
}

void DestroyPlot(struct Plot* _Plot) {
	DestroyObject((struct Object*)_Plot);
	RBDelete(&g_GameWorld.PlotList, PlotLeader(_Plot));
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
	if(_Guy == NULL)
		return;
	assert(PlotCanAsk(_Plot, _Side, _Guy) == 1);
	assert(_Side == PLOT_ATTACKERS || _Side == PLOT_DEFENDERS);
	LnkLstPushBack(&_Plot->Side[_Side], _Guy);
	_Plot->SidePower[_Side] += BigGuyPlotPower(_Guy);
}

int PlotInsert(const struct Plot* _One, const struct Plot* _Two) {
	return PlotLeader(_One)->Object.Id - PlotLeader(_Two)->Object.Id;
}

int PlotSearch(const struct BigGuy* _One, const struct Plot* _Two) {
	return _One->Object.Id - PlotLeader(_Two)->Object.Id;
}

int IsInPlot(const struct Plot* _Plot, const struct BigGuy* _Guy) {
	struct LnkLst_Node* _Itr = NULL;

	for(int i = 0; i < 2; ++i) {
		_Itr = _Plot->Side[i].Front;
		while(_Itr != NULL) {
			if(_Itr->Data == _Guy)
				return i + 1;
			_Itr = _Itr->Next;	
		}
	}
	return 0;
}

int HasPlotAction(const struct Plot* _Plot, const struct BigGuy* _Guy) {
	for(const struct PlotAction* _Action = PLOT_CURRACTLIST(_Plot); _Action != NULL; _Action = _Action->Next) {
		if(_Action->Actor == _Guy) {
			return 1;
		}
	}
	return 0;
}

/**
 * \brief Iterates through all PlotActions linked in _Actions looking for any ActionList
 * that has been performed by _Guy. All BigGuy's are allowed to perform one action per PlotWarScoreMods
 * tick thus once we find the action we can assume that no more actions will be found unless the action IsInPlot
 * special such as PLOTACT_DOUBLEATTK that adds a second action to simulate a second attack.
 * \return The enumeration that represents the PlotAction. 
 */

static inline void PlotPerformAction(struct PlotAction* _Action, struct Plot* _Plot) {
	switch( _Action->Type) {
		case PLOTACT_LOWERSTAT:
			for(int i = 0, _OSide = ((_Action->ActorSide == PLOT_ATTACKERS) ? (PLOT_DEFENDERS) : (PLOT_ATTACKERS)); i < BGSKILL_SIZE; ++i) {
				_Plot->StatMods[_OSide][i] += -PLOTACT_STATMOD;
			}
			return;	
		case PLOTACT_STOPATTK:
			for(struct PlotAction* i = PLOT_CURRACTLIST(_Plot); i != NULL; i = i->Next) {
				if(i->ActorSide != _Action->ActorSide) {
					i->Flags &= ~PLOTFLAG_HIT;
					return;	
				}
			}
			return;
	}
}

int PlotWarScore(struct Plot* _Plot, const struct LinkedList* _GuyList, struct PlotAction** _Actions, int* _Threat) {
	int _Score = 0;

	for(const struct LnkLst_Node* _Itr = _GuyList->Front; _Itr != NULL; _Itr = _Itr->Next) {
		const struct BigGuy* _Guy = _Itr->Data;
		struct PlotAction* _Action = NULL;

		for(_Action = *_Actions; _Action != NULL; _Action = _Action->Next) {
			if(_Action->Actor != _Guy)
				continue;
			++(*_Threat);
			goto found_action;
		}
		PlotAddAction(_Plot, PLOTACT_ATTACK, _Guy, NULL);
		PLOT_CURRACTLIST(_Plot)->Flags |= BigGuySkillCheck(_Guy, BGSKILL_WIT, SKILLCHECK_DEFAULT);
		_Action = PLOT_CURRACTLIST(_Plot);
		found_action:
		PlotPerformAction(_Action, _Plot);
		_Score += ActionDamage(_Action);
		continue;
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
	if(PlotDefenderWon(_Plot)) {
		_Looser = PlotLeader(_Plot);
		goto warscore_end;
	}
	if(PlotAttackerWon(_Plot) != 0) {
		switch(_Plot->PlotType) {
			case PLOT_OVERTHROW:
				PushEvent(EVENT_NEWLEADER, PlotTarget(_Plot), PlotLeader(_Plot));
				break;
			case PLOT_PASSPOLICY:
				PushEvent(EVENT_NEWPOLICY, PlotLeader(_Plot)->Person->Family->HomeLoc->Government, _Plot->PlotData);
				break;
			case PLOT_CHANGEPOLICY:
				PushEvent(EVENT_CHANGEPOLICY, PlotLeader(_Plot)->Person->Family->HomeLoc->Government, _Plot->PlotData);
				break;
			case PLOT_SLANDER:
				PushEvent(EVENT_SLANDER, NULL, NULL);
				break;
		}
		_Looser = PlotTarget(_Plot);
		goto warscore_end;
	}
	PLOT_SWAPACTLIST(_Plot);
	for(struct PlotAction* _Action = PLOT_CURRACTLIST(_Plot); _Action != NULL; _Action = _Action->Next)
		DestroyPlotAction(_Action);
	PLOT_CURRACTLIST(_Plot) = NULL;
	return;
	warscore_end:
	PushEvent(EVENT_ENDPLOT, _Plot, _Looser);
}

const struct PlotAction* const  PlotPrevActList(const struct Plot* _Plot) {
	return PLOT_PREVACTLIST(_Plot);
}

const struct PlotAction* const  PlotCurrActList(const struct Plot* _Plot) {
	return PLOT_CURRACTLIST(_Plot);	
}

struct BigGuy* PlotLeader(const struct Plot* _Plot) {
	return (_Plot->Side[PLOT_ATTACKERS].Size > 0) ? (_Plot->Side[PLOT_ATTACKERS].Front->Data) : (NULL);
}

struct BigGuy* PlotTarget(const struct Plot* _Plot) {
	return (_Plot->Side[PLOT_DEFENDERS].Size > 0) ? (_Plot->Side[PLOT_DEFENDERS].Front->Data) : (NULL);
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

int PlotAddAction(struct Plot* _Plot, int _Type, const struct BigGuy* _Actor, const struct BigGuy* _Target) {
	int _ActorSide = 0;
	struct PlotAction* _Action = NULL;

	if(_Actor == NULL || (_Actor == _Target))
		return 0;
	if(_Target != NULL) {
		if((_ActorSide = PlotOnSide(_Plot, PLOT_ATTACKERS, _Actor)) == PlotOnSide(_Plot, PLOT_ATTACKERS, _Target))
			return 0;
	}
	if(HasPlotAction(_Plot, _Actor) == 1)
		return 0;
	_Action = CreatePlotAction(_Type, _Actor, _ActorSide, PLOT_CURRACTLIST(_Plot));
	PLOT_CURRACTLIST(_Plot) = _Action;
	switch(_Type) {
		case PLOTACT_DOUBLEATTK:
			PLOT_CURRACTLIST(_Plot) = CreatePlotAction(PLOTACT_DOUBLEATTK, _Actor, _ActorSide, PLOT_CURRACTLIST(_Plot));
	}
	return 1;
}	

int PlotGetThreat(const struct Plot* _Plot) {
	return Abs(_Plot->Threat[PLOT_ATTACKERS] - _Plot->Threat[PLOT_DEFENDERS]);
}

int PlotCanUseAction(const struct Plot* _Plot, const struct BigGuy* _Guy) {
	const struct PlotAction* _Action = PlotCurrActList(_Plot);

	while(_Action!= NULL) {
		if(_Action->Actor == _Guy)
			return 0;
		_Action= _Action->Next;
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
	const struct BigGuy* _Actor = _Action->Actor;

	switch(_Action->Type) {
		case PLOTACT_ATTACK:
			snprintf(*_Buffer, _Size, "%s(%i%%) did %i damage.",
				 _Action->Actor->Person->Name,
				 _Actor->Stats[g_PlotActionTypes[_Action->Type].SkillUsed],
				 ActionDamage(_Action));
			break;
		case PLOTACT_LOWERSTAT:
			snprintf(*_Buffer, _Size, "%s(%i%%) lowered all the oponents stats by %i",
				_Action->Actor->Person->Name,
				_Actor->Stats[g_PlotActionTypes[_Action->Type].SkillUsed],
				((_Action->Flags & PLOTFLAG_HIT) == PLOTFLAG_HIT) ? (PLOTACT_LOWERSTAT) : (0));
			break;
		case PLOTACT_STOPATTK:
			snprintf(*_Buffer, _Size, "%s(%i%%) prevented %s from performing the action: %s from taking place.",
				_Action->Actor->Person->Name,
				_Actor->Stats[g_PlotActionTypes[_Action->Type].SkillUsed],
				_Action->ActionStopped->Actor->Person->Name,
				g_PlotActionTypes[_Action->Type].Name);

			break;
		case PLOTACT_DOUBLEDMG:
			snprintf(_Extra, __FUNC_EXTRASZ, "dealt double damage totaling %i", ActionDamage(_Action));
			snprintf(*_Buffer, _Size, "%s(%i%%) %s.",
				 _Action->Actor->Person->Name,
				 _Actor->Stats[g_PlotActionTypes[_Action->Type].SkillUsed],
				 _Extra);
			break;
		default:
			break;
	}
#undef __FUNC_EXTRASZ
}

void PlotDescription(const struct Plot* _Plot, char** _Buffer, size_t _Size) {
	const struct PolicyOption* _PolOpt = NULL;

	switch(_Plot->Type) {
		case PLOT_PASSPOLICY:
			snprintf(*_Buffer, _Size, "%s is attempting to pass the policy %s.",
				 PlotLeader(_Plot)->Person->Name,
				 ((struct Policy*)_Plot->PlotData)->Name); 
			break;
		case PLOT_CHANGEPOLICY:
			_PolOpt = PolicyChange(_Plot->PlotData);
			snprintf(*_Buffer, _Size, "%s is conspiring to change the policy %s to include %s.",
				PlotLeader(_Plot)->Person->Name,
				 ((struct ActivePolicy*)_Plot->PlotData)->Policy->Name,
				 _PolOpt->Name);
	}
}
