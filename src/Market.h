/**
 * File: Market.h
 * Author: David Brotz
 */

#ifndef __MARKET_H
#define __MARKET_H

#include "sys/Array.h"

#include "Good.h"

#include <math.h>
#include <inttypes.h>

struct Family;

//To be used with MarReq.
struct MarGood {
	struct Family* Owner;
	int64_t Quantity;
};

struct MarReq {
	const struct GoodBase* Base;
	struct Array Fillers;//List of MarGood.
	int64_t Quantity; //How much of this base is in the fillers array.
	uint32_t Demand; //How much is expected to be needed for the next month.
	uint32_t Price;
};

void DestroyMarGood(struct MarReq* Req, struct MarGood* Good, int Idx);

/**
 * Checks if Seller already has a MarReq of type Base at their settlement if
 * they do add Quantity to the MarReq. Otherwise a new MarReq is created
 * at the settlement.
 */
void GoodSell(const struct Family* Seller, const struct GoodBase* Base, uint32_t Quantity);
/**
 * Checks for a MarReq that contains Base. If such a MarReq exists then attempt
 * to buy Quantity amount. If the MarReq does not have enough items to statisfy Quantity
 * GoodBuy will continue iterating through all MarReqs to find another valid MarReq.
 * If Quantity amount of goods are not able to be bought then a BuyRequest will be created for
 * the remaining amount.
 */
int GoodBuy(struct Family* Family, const struct GoodBase* Base, int Quantity);
int MarketQuantity(struct Family* Family, const struct Array* Market, const struct GoodBase* Base);
struct MarGood* MarketSearch(struct Family* Family, const struct Array* MarketQuantity, const struct GoodBase* Base);
static inline int MarketPrice(struct MarReq* Req) {
	if(Req->Quantity <= 0) return Req->Base->Price;
	double Ratio = (Req->Demand / (double)Req->Quantity);

	Ratio = sqrt(Ratio);
	Ratio = fabs(Ratio);
	return Req->Base->Price + Req->Base->Price * Ratio;
	//return Req->Base->Price * abs(sqrt((Demand / (double)Req->Quantity)));
}
void MarketCalcPrice(struct Array* Market);
int GoodPay(struct Family* Buyer, const struct MarReq* SellReq);

#endif

