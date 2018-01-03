/**
 * File: Market.c
 * Author: David Brotz
 */

#include "Market.h"

#include "Location.h"

/*
 * GoodBuy and GoodSell both need to check the buyer's market and then the seller's market before they
 * can sucessfully post a buy/sell request. Instead of two markets there should simply be one. This
 * would help in finding buyers/sellers faster and only have to traverse one array.
 */

static inline void MarReqQuantity(struct MarReq* Req, struct MarGood* Good, int64_t Quantity) {
	Good->Quantity += Quantity;
	Req->Quantity += Quantity;
}

int MarketSell(struct MarReq* Req, struct MarGood* Seller, int64_t Quantity) {
	int Bought = 0;
	struct Good* Good = NULL;

	if(Seller->Quantity < Quantity) {
		Quantity -= Seller->Quantity;
		Bought = Seller->Quantity;
		MarReqQuantity(Req, Seller, -Seller->Quantity);
		return Quantity;
	} else {
		MarReqQuantity(Req, Seller, -Quantity);
		Bought = Quantity;
		Quantity = 0;
	}
	Good = CheckGoodTbl(&Seller->Owner->Goods, Req->Base->Name, &g_Goods);
	Good->Quantity += Bought;
	return Quantity;
}

struct MarReq* CreateMarReq(const struct GoodBase* Base, uint32_t Cost) {
	struct MarReq* Req = (struct MarReq*) malloc(sizeof(struct MarReq));

	Req->Base = Base;
	Req->Price = Cost;
	Req->Quantity = 0;
	Req->Demand = 0;
	CtorArray(&Req->Fillers, 4);
	return Req;
}

void DestroyMarReq(struct MarReq* Req) {
	for(int i = 0; i < Req->Fillers.Size; ++i) {
		free(Req->Fillers.Table[i]);
	}
	free(Req);
}

void DestroyMarGood(struct MarReq* Req, struct MarGood* Good, int Idx) {
	ArrayRemove(&Req->Fillers, Idx);
	MarReqQuantity(Req, Good, -Good->Quantity);
	free(Good);
}

int GoodBuy(struct Family* Family, const struct GoodBase* Base, int Quantity) {
	struct Settlement* Settlement = Family->HomeLoc;
	struct MarGood* Sell = NULL;
	struct MarReq* SellReq = NULL;
	int Sold = 0;
			
	//Check the market if there is someone selling the good.
	for(int i = 0; i < Settlement->Market.Size; ++i) {
		SellReq = Settlement->Market.Table[i];

		if(GoodBaseCmp(SellReq->Base, Base) == 0) {
			if(SellReq->Quantity > 0) {
				for(int j = 0; j < SellReq->Fillers.Size; ++j) {
					struct MarGood* Sell = SellReq->Fillers.Table[j];
					
					Quantity = MarketSell(SellReq, Sell, Quantity);
				}
			} else {
				//Not enough goods to buy. Create a buy request.
				for(int i = 0; i < Settlement->Market.Size; ++i) {
					struct MarReq* Req = Settlement->Market.Table[i];

					if(GoodBaseCmp(Req->Base, Base) == 0) {

						for(int j = 0; j < Req->Fillers.Size; ++j) {
							struct MarGood* Sell = Req->Fillers.Table[j];

							if(Sell->Owner == Family) {
								int Amount = Quantity - Sold;

								MarReqQuantity(Req, Sell, -Amount);
								//ArrayRemove(&Req->Fillers, j--);
								goto end;
							}
						}
					SellReq = Req;
					goto create_margood;
					}
				}
			}
		}
	}
	//MarketRemoveBuy(Family, &Settlement->BuyReqs, Base, Sold);
	SellReq = CreateMarReq(Base, GoodGetValue(Base));
	SellReq->Quantity = 0;
	ArrayInsert_S(&Family->HomeLoc->Market, SellReq);
create_margood:
	Sell = malloc(sizeof(struct MarGood));
	Sell->Owner = Family;
	MarReqQuantity(SellReq, Sell, -(Quantity - Sold));
	ArrayInsert_S(&SellReq->Fillers, Sell);
end:
	SellReq->Demand += Sold;
	return Sold;
}

int MarketQuantity(struct Family* Family, const struct Array* Market, const struct GoodBase* Base) {
	struct MarGood* MarGood = MarketSearch(Family, Market, Base);

	return (MarGood == NULL) ? (0) : (MarGood->Quantity);
}

void MarketRemoveBuy(struct Family* Family, const struct Array* Market, struct GoodBase* Base, int Quantity) {
	if(!(Quantity > 0)) return;
	for(int i = 0; i < Market->Size; ++i) {
		struct MarReq* MarReq = Market->Table[i];

		if(MarReq->Base != Base) continue;
		for(int j = 0; j < MarReq->Fillers.Size; ++j) {
			struct MarGood* MarGood = MarReq->Fillers.Table[j];

			if(MarGood->Owner == Family) {
				MarGood->Quantity -= Quantity;
				if(MarGood->Quantity <= 0) {
					//ArrayRemove(&MarReq->Fillers, j);
				}	
			}
		}
	}
}

struct MarGood* MarketSearch(struct Family* Family, const struct Array* Market, const struct GoodBase* Base) {
	for(int i = 0; i < Market->Size; ++i) {
		struct MarReq* MarReq = Market->Table[i];

		if(MarReq->Base != Base) continue;
		for(int j = 0; j < MarReq->Fillers.Size; ++j) {
			struct MarGood* MarGood = MarReq->Fillers.Table[j];

			if(MarGood->Owner == Family) return MarGood;
		}
	}
	return NULL;
}

void MarketCalcPrice(struct Array* Market) {
	for(int i = 0; i < Market->Size; ++i) {
		struct MarReq* SellReq = Market->Table[i];

		SellReq->Price = MarketPrice(SellReq);
	}
}

int GoodPay(struct Family* Buyer, const struct MarReq* SellReq) {
	int Quantity = 0;

	GoodPayInKind(Buyer, SellReq->Price, SellReq->Base, &Quantity);
	//SellItem(Buyer, SellReq);
	return Quantity;
}

int MarketCheckBuyers(const struct Family* Family, const struct GoodBase* Base, int Quantity) {
	struct Settlement* Settlement = Family->HomeLoc;

	for(int i = 0; i < Settlement->BuyReqs.Size; ++i) {
		struct MarReq* Req = Settlement->BuyReqs.Table[i];

		if(GoodBaseCmp(Req->Base, Base) == 0) {

			for(int j = 0; j < Req->Fillers.Size; ++j) {
				struct MarGood* Sell = Req->Fillers.Table[j];

				if(Sell->Quantity < Quantity) {
					Quantity -= Sell->Quantity;
					Sell->Quantity = 0;
					ArrayRemove(&Req->Fillers, j--);
					if(Req->Quantity < Quantity) Req->Quantity = 0;
				} else {
					MarReqQuantity(Req, Sell, -Quantity);
					return 0;
				}
			}
		return Quantity;
		}
	}
	return Quantity;
}

void GoodSell(const struct Family* Seller, const struct GoodBase* Base, uint32_t Quantity) {
	struct MarGood* Sell = NULL;
	struct MarReq* SellReq = NULL;

	//Quantity = MarketCheckBuyers(Seller, Base, Quantity);
	for(int i = 0; i < Seller->HomeLoc->Market.Size; ++i) {
		SellReq = Seller->HomeLoc->Market.Table[i];

		if(SellReq->Base == Base) {
			//Someone is trying to buy this good.
			if(SellReq->Quantity > 0) {
				for(int j = 0; j < SellReq->Fillers.Size; ++j) {
					Sell = SellReq->Fillers.Table[j];

					Quantity = MarketSell(SellReq, Sell, Quantity);
					if(Quantity == 0) return;
				}
			}
			//After we've sold our stock to buyers, fill the market with the remaining amount.
			for(int j = 0; j < SellReq->Fillers.Size; ++j) {
				Sell = SellReq->Fillers.Table[j];
				
				if(Sell->Owner == Seller) {
					MarReqQuantity(SellReq, Sell, Quantity);
					return;
				}
			}
			SellReq->Quantity += Quantity;
			Sell = malloc(sizeof(struct MarGood));
			Sell->Owner = Seller;
			Sell->Quantity = Quantity;
			ArrayInsert_S(&SellReq->Fillers, Sell);
			return;
		}
	}
	SellReq = CreateMarReq(Base, GoodGetValue(Base));
	SellReq->Quantity = Quantity;
	Sell = malloc(sizeof(struct MarGood));
	Sell->Owner = Seller;
	Sell->Quantity = Quantity;
	ArrayInsert_S(&SellReq->Fillers, Sell);
	ArrayInsert_S(&Seller->HomeLoc->Market, SellReq);
}
