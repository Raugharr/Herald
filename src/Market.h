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

//To be used with Product.
struct MarGood {
	const struct Family* Owner;
	int64_t Quantity;
};

struct Product {
	const struct GoodBase* Base;
	struct Array Fillers;//List of MarGood.
	int64_t Quantity; //How much of this base is in the fillers array.
	int32_t Supply;
	int32_t Demand; //How much is expected to be needed for the next month.
	uint32_t Price;
};

void DestroyMarGood(struct Product* Req, struct MarGood* Good, int Idx);

/**
 * Checks if Seller already has a Product of type Base at their settlement if
 * they do add Quantity to the Product. Otherwise a new Product is created
 * at the settlement.
 */
void GoodSell(const struct Family* Seller, const struct GoodBase* Base, uint32_t Quantity);
/**
 * Checks for a Product that contains Base. If such a Product exists then attempt
 * to buy Quantity amount. If the Product does not have enough items to statisfy Quantity
 * GoodBuy will continue iterating through all Products to find another valid Product.
 * If Quantity amount of goods are not able to be bought then a BuyRequest will be created for
 * the remaining amount.
 */
int GoodBuy(struct Family* Family, const struct GoodBase* Base, int Quantity);
int MarketQuantity(struct Family* Family, const struct Array* Market, const struct GoodBase* Base);
struct MarGood* MarketSearch(struct Family* Family, const struct Array* MarketQuantity, const struct GoodBase* Base);
void MarketCalcPrice(struct Array* Market);
int GoodPay(struct Family* Buyer, const struct Product* SellReq);

static inline int ProductSupply(const struct Product* Good) {return Good->Supply * (Good->Supply > 0);}
static inline int ProductDemand(const struct Product* Good) {return Good->Demand * (Good->Demand > 0);}
static inline int MarketPrice(struct Product* Req) {
	if(Req->Demand <= 0) return Req->Base->Price;
	double Ratio = (ProductDemand(Req) / ((double)ProductSupply(Req) + 1));

	Ratio = sqrt(Ratio);
	Ratio = fabs(Ratio);
	return Req->Base->Price + Req->Base->Price * Ratio;
	//return Req->Base->Price * abs(sqrt((Demand / (double)Req->Quantity)));
}

static inline struct Product* MarketFindProduct(const struct Array* Market, const struct GoodBase* Base) {
	for(int i = 0; i < Market->Size; ++i) {
		struct Product* Product = Market->Table[i];

		if(Product->Base == Base) return Product;
	}
	return NULL;
}

#endif

