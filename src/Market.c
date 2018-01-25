/**
 * File: Market.c
 * Author: David Brotz
 */

#include "Market.h"

#include "Date.h"

#include "Location.h"

/*
 * GoodBuy and GoodSell both need to check the buyer's market and then the seller's market before they
 * can sucessfully post a buy/sell request. Instead of two markets there should simply be one. This
 * would help in finding buyers/sellers faster and only have to traverse one array.
 */

static int MarGoodCmp(const struct MarGood* One, const struct MarGood* Two) {
	return One->Owner->Object.Id - Two->Owner->Object.Id;
}

static inline void ProductQuantity(struct Product* Req, struct MarGood* Good, int64_t Quantity) {
	//Assert((Req->Supply * (Req->Supply > 0)) - (Req->Demand * (Req->Demand > 0)) == Req->Quantity);
	Good->Quantity += Quantity;
	Req->Quantity += Quantity;
	//Req->Demand -= Quantity;
	//Req->Supply += Quantity;
	Req->Demand += (Quantity < 0) * Quantity;
	Req->Supply += (Quantity > 0) * Quantity;
}

int MarketSell(struct Product* Req, struct MarGood* Seller, int64_t Quantity) {
	int Bought = 0;

	Assert(Seller->Quantity >= 0);
	if(Seller->Quantity < Quantity) {
		Bought = Seller->Quantity;
		ProductQuantity(Req, Seller, -Bought);
	} else {
		ProductQuantity(Req, Seller, -Quantity);
		Bought = Quantity;
	}
	Assert(Bought >= 0);
	return Bought;
}

struct Product* CreateProduct(const struct GoodBase* Base, uint32_t Cost) {
	struct Product* Req = (struct Product*) malloc(sizeof(struct Product));

	Req->Base = Base;
	Req->Price = Cost;
	Req->Quantity = 0;
	Req->Demand = 0;
	Req->Supply = 0;
	CtorArray(&Req->Fillers, 4);
	return Req;
}

void DestroyProduct(struct Product* Req) {
	for(int i = 0; i < Req->Fillers.Size; ++i) {
		free(Req->Fillers.Table[i]);
	}
	free(Req);
}

void DestroyMarGood(struct Product* Req, struct MarGood* Good, int Idx) {
	ArrayRemove(&Req->Fillers, Idx);
	ProductQuantity(Req, Good, -Good->Quantity);
	free(Good);
}

int GoodBuy(struct Family* Family, const struct GoodBase* Base, int Quantity) {
	struct Settlement* Settlement = Family->HomeLoc;
	struct MarGood* Sell = NULL;
	struct Product* SellReq = NULL;
	int Sold = 0;
			
	Assert(Quantity >= 0);
	//Check the market if there is someone selling the good.
	for(int i = 0; i < Settlement->Market.Size; ++i) {
		SellReq = Settlement->Market.Table[i];

		if(GoodBaseCmp(SellReq->Base, Base) == 0) {
			if(SellReq->Quantity > 0) {
				for(int j = 0; j < SellReq->Fillers.Size; ++j) {
					struct MarGood* Sell = SellReq->Fillers.Table[j];
					
					if(Sell->Quantity <= 0) continue;
					Sold += MarketSell(SellReq, Sell, Quantity);
					Quantity -= Sold;
					if(Quantity <= 0) return Sold;
				}
			return Sold;
			} else {
				//Not enough goods to buy. Create a buy request.
				for(int j = 0; j < SellReq->Fillers.Size; ++j) {
					struct MarGood* Sell = SellReq->Fillers.Table[j];

					if(Sell->Owner == Family) {
						int Amount = Quantity - Sold;

						ProductQuantity(SellReq, Sell, -Amount);
						goto end;
					}
				}
				goto create_margood;
			}
		}
	}
	//MarketRemoveBuy(Family, &Settlement->BuyReqs, Base, Sold);
	SellReq = CreateProduct(Base, GoodGetValue(Base));
	ArrayInsert_S(&Family->HomeLoc->Market, SellReq);
create_margood:
	Sell = malloc(sizeof(struct MarGood));
	Sell->Owner = Family;
	Sell->Quantity = 0;
	ProductQuantity(SellReq, Sell, -(Quantity - Sold));
	ArrayInsert_S(&SellReq->Fillers, Sell);
end:
	//SellReq->Demand += Quantity;
	return Sold;
}

int MarketQuantity(struct Family* Family, const struct Array* Market, const struct GoodBase* Base) {
	struct MarGood* MarGood = MarketSearch(Family, Market, Base);

	return (MarGood == NULL) ? (0) : (MarGood->Quantity);
}

void MarketRemoveBuy(struct Family* Family, const struct Array* Market, struct GoodBase* Base, int Quantity) {
	if(!(Quantity > 0)) return;
	for(int i = 0; i < Market->Size; ++i) {
		struct Product* Product = Market->Table[i];

		if(Product->Base != Base) continue;
		for(int j = 0; j < Product->Fillers.Size; ++j) {
			struct MarGood* MarGood = Product->Fillers.Table[j];

			if(MarGood->Owner == Family) {
				MarGood->Quantity -= Quantity;
				if(MarGood->Quantity <= 0) {
					//ArrayRemove(&Product->Fillers, j);
				}	
			}
		}
	}
}

struct MarGood* MarketSearch(struct Family* Family, const struct Array* Market, const struct GoodBase* Base) {
	for(int i = 0; i < Market->Size; ++i) {
		struct Product* Product = Market->Table[i];

		if(Product->Base != Base) continue;
		for(int j = 0; j < Product->Fillers.Size; ++j) {
			struct MarGood* MarGood = Product->Fillers.Table[j];

			if(MarGood->Owner == Family) return MarGood;
		}
	}
	return NULL;
}

void MarketCalcPrice(struct Array* Market) {
	for(int i = 0; i < Market->Size; ++i) {
		struct Product* SellReq = Market->Table[i];
	//	int DmdDelta = SellReq->Demand / MONTHS;

		SellReq->Price = MarketPrice(SellReq);
	//	DmdDelta += (DmdDelta < 1); //If DmdDelta == 0 then DmdDelta = 1.
	//	SellReq->Demand -= DmdDelta * ((INT_MIN & (SellReq->Demand - 1)) == 0); //If SellReq->Demand - DmdDelta < 0 then SellReq->Demand = 0.
	}
}

int GoodPay(struct Family* Buyer, const struct Product* SellReq) {
	int Quantity = 0;

	GoodPayInKind(Buyer, SellReq->Price, SellReq->Base, &Quantity);
	//SellItem(Buyer, SellReq);
	return Quantity;
}

/*int MarketCheckBuyers(const struct Family* Family, const struct GoodBase* Base, int Quantity) {
	struct Settlement* Settlement = Family->HomeLoc;

	for(int i = 0; i < Settlement->Market.Size; ++i) {
		struct Product* Req = Settlement->Market.Table[i];

		if(GoodBaseCmp(Req->Base, Base) == 0) {

			for(int j = 0; j < Req->Fillers.Size; ++j) {
				struct MarGood* Sell = Req->Fillers.Table[j];

				
				if(Sell->Quantity < Quantity) {
					Quantity -= Sell->Quantity;
					Sell->Quantity = 0;
					ArrayRemove(&Req->Fillers, j--);
					if(Req->Quantity < Quantity) Req->Quantity = 0;
				} else {
					ProductQuantity(Req, Sell, -Quantity);
					return 0;
				}
			}
		return Quantity;
		}
	}
	return Quantity;
}*/

void GoodSell(const struct Family* Seller, const struct GoodBase* Base, uint32_t Quantity) {
	struct Product* SellReq = NULL;
	struct MarGood* Sell = NULL;
	struct MarGood* OwnerReq = NULL;

	//Quantity = MarketCheckBuyers(Seller, Base, Quantity);
	for(int i = 0; i < Seller->HomeLoc->Market.Size; ++i) {
		SellReq = Seller->HomeLoc->Market.Table[i];

		if(SellReq->Base == Base) {
			//Someone is trying to buy this good.
			for(int j = 0; j < SellReq->Fillers.Size; ++j) {
				Sell = SellReq->Fillers.Table[j];

				if(Sell->Owner == Seller) {
					OwnerReq = Sell;
					break;
				}
			}
			if(OwnerReq != NULL) {
				//After we've sold our stock to buyers, fill the market with the remaining amount.
				ProductQuantity(SellReq, OwnerReq, Quantity);
				return;
			}
			Sell = malloc(sizeof(struct MarGood));
			Sell->Owner = Seller;
			Sell->Quantity = Quantity;
			ProductQuantity(SellReq, Sell, Quantity);
			ArrayInsert_S(&SellReq->Fillers, Sell);
			return;
		}
	}
	SellReq = CreateProduct(Base, GoodGetValue(Base));
	SellReq->Quantity = Quantity;
	SellReq->Supply += Quantity;
	Sell = malloc(sizeof(struct MarGood));
	Sell->Owner = Seller;
	Sell->Quantity = Quantity;
	ArrayInsert_S(&SellReq->Fillers, Sell);
	ArrayInsert_S(&Seller->HomeLoc->Market, SellReq);
}
