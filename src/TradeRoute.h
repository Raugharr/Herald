/**
 * File: TradeRoute.h
 * Author: David Brotz
 */

#ifndef __TRADEROUTE_H
#define __TRADEROUTE_H

struct TradeRoute {
	struct Settlement* From;
	struct Settlement* To;
	//List of GoodBase that will be given to To or taken from To.
	struct Array Import;
	struct Array Export;
};

void CreateTroute(const struct Settlement* Settlement);

#endif
