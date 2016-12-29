/**
 * Author: David Brotz
 * File: Faction.h
 */

#ifndef __FACTION_H
#define __FACTION_H

#include "Family.h"

#include "sys/Array.h"

struct Person;

#define FACTION_PASSPOWER (20)
#define FACTION_MAXPOWER(Faction, Ideology) (24 * (Faction)->PowerGain[Ideology]) 
#define CanPassFactGoal(Faction) ((Faction)->Power > FACTION_PASSPOWER)
#define FactionCasteIdx(Ideology, Caste) (((Ideology) * CASTE_SIZE) + (Caste))
#define FactionIsActive(Faction, Ideology) (((Faction)->ActiveMask & (1 << (Ideology))) == (1 << (Ideology)))

enum EFactionGoal {
	FACTION_GNONE = 0,
	FACTION_GLTAXES, //Lower taxes. 
	FACTION_GRTAXES, //Raise taxes. 
	FACTION_GLCASTE, //Decrease power of a certain caste.
	FACTION_GRCASTE, //Increase power of a certain caste.
	FACTION_GLPOLICY,
	FACTION_GRPOLICY,
	//FACTION_GPOWER, //Weaken the other faction.
	//FACTION_GOVERTHROW, //Overthrow the ruler of the government.
	//FACTION_GWAR, //Demands to expand territory/raid.
	//FACTION_GCIVIC, //Demands to create more civic structures.
	//FACTION_GDEFENSE, //Demands of defenses to be made.
	FACTION_GSIZE
};

extern const char* g_FactionGoalNames[FACTION_GSIZE];
extern const char* g_FactionNames[FACTION_GSIZE];

enum EFactionIdeology {
	FACTION_IDNOBLE,
	FACTION_IDPEASANT,
	FACTION_IDRELIGION,
	FACTION_IDMERCHANT,
	FACTION_IDSIZE,
	FACTION_IDNONE
};

struct FactionOpCode {
	uint8_t Type;
	union {
		struct {
			uint8_t Caste; //Which caste this will effect.
		} Caste;

		struct {
			uint8_t TaxType; //What tax to raise/lower.
		} Taxes;

		struct {
			struct Policy* Policy;
			uint8_t Row;
		} Policy;
	};
};

struct Faction {
	struct Settlement* Settlement;
	struct BigGuy* Leader[FACTION_IDSIZE];
	struct Array Bosses[FACTION_IDSIZE];
	struct Array Mob[FACTION_IDSIZE];
	int Coro;
	DATE LastGoal[FACTION_IDSIZE];
	uint16_t Power[FACTION_IDSIZE];
	uint16_t PowerGain[FACTION_IDSIZE];
//	uint16_t CasteOpinion[FACTION_IDSIZE * CASTE_SIZE];
//	uint16_t CasteDelta[FACTION_IDSIZE * CASTE_SIZE];//Modifier of CasteOpinion that is done once per month. Modifier is halved every month.
	uint16_t CasteCount[FACTION_IDSIZE * CASTE_SIZE]; //Number of people in this faction for each caste.
	uint16_t Goal; //Goal trying to be passed.
	int16_t FactionBet[FACTION_IDSIZE];
	uint8_t CastePower[CASTE_SIZE]; //How much power each caste gives.
	uint8_t FactionWeight[FACTION_IDSIZE * CASTE_SIZE]; //How likely a member of a caste will join a certain faction.
	uint8_t ActiveMask;
	int16_t PolicyInfluence[FACTION_IDSIZE]; //Which policy this faction is supporting.
	bool OpposePolicy[FACTION_IDSIZE];
	bool DidOppose[FACTION_IDSIZE]; //Which faction opposed the last goal.
};

struct Faction* CtorFaction(struct Faction* Faction, struct Settlement* Settlement);
void DestroyFaction(struct Faction* Faction);

void FactionAddBoss(struct Faction* Faction, uint8_t Ideology, struct BigGuy* Boss);

void FactionAddPerson(struct Faction* Faction, uint8_t Ideology, struct Person* Person);
void FactionRemovePerson(struct Faction* Faction, uint8_t Ideology, struct Person* Person);
bool FactionBet(struct Faction* Faction, uint8_t Ideology, uint16_t Bet);
static inline void FactionPickSide(struct Faction* Faction, uint8_t Ideology, bool Against) {
	Faction->DidOppose[Ideology] = Against;
}
void FactionSetGoal(struct Faction* Faction, uint8_t Ideology, uint8_t Goal);
//True if it is Goal is valid.
bool FactionValGoal(struct Faction* Faction, uint8_t Ideology, uint8_t Goal);
void FactionPassGoal(struct Faction* Faction, uint8_t Ideology, struct FactionOpCode* Goal);
uint32_t FactionInfluenceCost(struct Faction* Faction, uint8_t Ideology);
#endif
