/**
 * Author: David Brotz
 * File: Plot.h
 */
#ifndef __PLOT_H
#define __PLOT_H

#include "sys/LinkedList.h"

struct BigGuy;

enum {
	PLOT_OVERTHROW,
	PLOT_SIZE
};

enum {
	PLOT_ATTACKERS,
	PLOT_DEFENDERS
};

struct Plot {
	int Type;
	struct LinkedList Side[2];
	struct LinkedList SideAsk[2];
};

struct Plot* CreatePlot(int _Type, struct BigGuy* _Owner, struct BigGuy* _Target);
void PlotJoin(struct Plot* _Plot, int _Side, struct BigGuy* _Guy);
int PlotInsert(const struct Plot* _One, const struct Plot* _Two);
int PlotSearch(const struct BigGuy* _One, const struct Plot* _Two);
/**
 * \return 0 If the person is not in the plot, 1 if the person is on side 0, and 2 if the person is on side 1.
 */
int IsInPlot(const struct Plot* _Plot, struct BigGuy* _Guy);
struct BigGuy* PlotLeader(const struct Plot* _Plot);
struct BigGuy* PlotTarget(const struct Plot* _Plot);
#endif
