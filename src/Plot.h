/**
 * Author: David Brotz
 * File: Plot.h
 */
#ifndef __PLOT_H
#define __PLOT_H

#include "Herald.h"

#include "BigGuy.h"
#include "Person.h"

#include "sys/LinkedList.h"

#include <stdbool.h>

#define PLOT_OVERTHROW_MAXSCORE (10)
#define IsPlotTypeValid(_Type) ((_Type) >= 0 && (_Type) < PLOT_SIZE)

struct BigGuy;
struct Policy;
struct ActivePolicy;

enum {
	PLOT_OVERTHROW,
	PLOT_CONRETINUE,
	PLOT_PASSPOLICY,
	PLOT_CHANGEPOLICY,
	PLOT_REMOVEPOLICY,
	PLOT_SLANDER,
	PLOT_SIZE
};

enum {
	PLOTACT_NONE,
	PLOTACT_ATTACK,
	PLOTACT_LOWERSTAT,
	PLOTACT_DOUBLEDMG,
	PLOTACT_DOUBLEATTK,
	PLOTACT_STOPATTK,
	PLOTACT_SIZE
};

enum {
	PLOTFLAG_HIT = 1,
};

enum {
	PLOT_ATTACKERS,
	PLOT_DEFENDERS,
	PLOT_SIDES
};

/**
 *\brief PlotAction describes an action that influences a plot.
 */
struct PlotAction {
	const int8_t Type;
	const int8_t ActorSide;
	const struct BigGuy* const Actor;
	const struct BigGuy* const Target;
	struct PlotAction* const Next;
	struct PlotAction* ActionStopped;
	int Flags;
};

struct Plot {
	struct Object Object;
	void* PlotData;
	/**
	 *One ActionList represents the current month's actions and can be added to,
	 * the other ActionList represents the previous month's actions and is intended
	 * to be used as a log and not be changed.
	 * Onces a month has ended we clear the previous month's ActionList and use it.
	 */
	 /**
	  * TODO: This should be an inplicit list of ActionPlots.
	  */
	struct PlotAction* ActionList[2]; 
	struct LinkedList Side[PLOT_SIDES];
	struct LinkedList SideAsk[PLOT_SIDES];
	int16_t SidePower[PLOT_SIDES]; //Assumption on how strong each side is.
	int16_t Threat[PLOT_SIDES]; //How much threat each side has accumulated.
	int16_t WarScore;
	int16_t MaxScore;
	uint8_t StatMods[PLOT_SIDES][BGSKILL_SIZE]; 
	uint8_t Type;
	uint8_t CurrActList; //Which ActionList is being used for the current month.
	bool HasStarted;
};

struct Plot* CreatePlot(int _Type, void* _Data, struct BigGuy* _Owner, struct BigGuy* _Target);
static inline struct Plot* CreatePassPolicyPlot(struct Policy* _Policy, struct BigGuy* _Owner, struct BigGuy* _Target) {
	return CreatePlot(PLOT_PASSPOLICY, _Policy, _Owner, _Target);
}

static inline struct Plot* CreateRemovePolicyPlot(struct Policy* _Policy, struct BigGuy* _Owner, struct BigGuy* _Target) {
	return CreatePlot(PLOT_REMOVEPOLICY, _Policy, _Owner, _Target);
}

static inline struct Plot* CreateChangePolicyPlot(struct ActivePolicy* _Policy, struct BigGuy* _Owner, struct BigGuy* _Target) {
	return CreatePlot(PLOT_CHANGEPOLICY, _Policy, _Owner, _Target);
}

static inline struct Plot* CreateConRetinuePlot(struct BigGuy* _Owner, struct BigGuy* _Target) {
	struct Retinue* _Retinue = IntSearch(&g_GameWorld.PersonRetinue, _Owner->Person->Object.Id);

	if(_Retinue == NULL)
		return NULL;
	return CreatePlot(PLOT_CONRETINUE, _Retinue, _Owner, _Target);
}

static inline int PlotPower(const struct Plot* _Plot, int _Side) {
	return _Plot->SidePower[_Side];
}
void DestroyPlot(struct Plot* _Plot);

void PlotJoin(struct Plot* _Plot, int _Side, struct BigGuy* _Guy);
int PlotInsert(const struct Plot* _One, const struct Plot* _Two);
int PlotSearch(const struct BigGuy* _One, const struct Plot* _Two);
void PlotThink(struct Object* _Obj);
const struct PlotAction* const PlotPrevActList(const struct Plot* _Plot);
const struct  PlotAction* const PlotCurrActList(const struct Plot* _Plot);
/**
 * \return 0 If the person is not in the plot, 1 if the person is on side 0, and 2 if the person is on side 1.
 */
int IsInPlot(const struct Plot* _Plot, const struct BigGuy* _Guy);
int HasPlotAction(const struct Plot* _Plot, const struct BigGuy* _Guy);
struct BigGuy* PlotLeader(const struct Plot* _Plot);
struct BigGuy* PlotTarget(const struct Plot* _Plot);
int PlotAddAction(struct Plot* _Plot, int _Type, const struct BigGuy* _Actor, const struct BigGuy* _Target);
int PlotGetThreat(const struct Plot* _Plot);
int PlotCanUseAction(const struct Plot* _Plot, const struct BigGuy* _Guy);
const char* PlotTypeStr(const struct Plot* _Plot);
void PlotSetTarget(struct Plot* _Plot, struct BigGuy* _Target);

void PlotActionEventStr(const struct PlotAction* _Action, char** _Buffer, size_t _Size);
void  PlotDescription(const struct Plot* _Plot, char** _Buffer, size_t _Size);
#endif
