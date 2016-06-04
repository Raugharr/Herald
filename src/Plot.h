/**
 * Author: David Brotz
 * File: Plot.h
 */
#ifndef __PLOT_H
#define __PLOT_H

#include "Herald.h"

#include "sys/LinkedList.h"

#define PLOT_OVERTHROW_MAXSCORE (10)
#define IsPlotTypeValid(_Type) ((_Type) >= 0 && (_Type) < PLOT_SIZE)

struct BigGuy;

enum {
	PLOT_OVERTHROW,
	PLOT_PASSPOLICY,
	PLOT_REMOVEPOLICY,
	PLOT_SIZE
};

enum {
	PLOTACT_ATTACK,
	PLOTACT_PREVENT,
	PLOTACT_DOUBLEDMG,
	PLOTACT_REDUCETHREAT,
	PLOTACT_SIZE
};

enum {
	PLOT_ATTACKERS,
	PLOT_DEFENDERS
};

/**
 *\brief PlotAction describes an action that influences a plot.
 */
struct PlotAction {
	int Type;
	int ActorSide;
	int DmgDelt;
	struct BigGuy* Actor;
	struct BigGuy* Target;
};

struct Plot {
	int Id;
	int Type;
	ObjectThink Think;
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	int PlotType;
	struct LinkedList Side[2];
	struct LinkedList SideAsk[2];
	int Threat[2]; //How much threat each side has accumulated.
	/**
	 *One ActionList represents the current month's actions and can be added to,
	 * the other ActionList represents the previous month's actions and is intended
	 * to be used as a log and not be changed.
	 * Onces a month has ended we clear the previous month's ActionList and use it.
	 */
	struct LinkedList ActionList[2]; 
	int CurrActList; //Which ActionList is being used for the current month.
	int WarScore;
	int MaxScore;
};

struct Plot* CreatePlot(int _Type, struct BigGuy* _Owner, struct BigGuy* _Target);
void DestroyPlot(struct Plot* _Plot);

void PlotJoin(struct Plot* _Plot, int _Side, struct BigGuy* _Guy);
int PlotInsert(const struct Plot* _One, const struct Plot* _Two);
int PlotSearch(const struct BigGuy* _One, const struct Plot* _Two);
void PlotThink(struct Plot* _Plot);
const struct LinkedList* PlotPrevActList(const struct Plot* _Plot);
const struct LinkedList* PlotCurrActList(const struct Plot* _Plot);
/**
 * \return 0 If the person is not in the plot, 1 if the person is on side 0, and 2 if the person is on side 1.
 */
int IsInPlot(const struct Plot* _Plot, struct BigGuy* _Guy);
struct BigGuy* PlotLeader(const struct Plot* _Plot);
struct BigGuy* PlotTarget(const struct Plot* _Plot);
void PlotAddAction(struct Plot* _Plot, int _Type, struct BigGuy* _Actor, struct BigGuy* _Target);
int PlotGetThreat(const struct Plot* _Plot);
int PlotCanUseAction(const struct Plot* _Plot, const struct BigGuy* _Guy);
const char* PlotTypeStr(const struct Plot* _Plot);
void PlotSetTarget(struct Plot* _Plot, struct BigGuy* _Target);

void PlotActionEventStr(const struct PlotAction* _Action, char** _Buffer, size_t _Size);
#endif
