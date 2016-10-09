/*
 * Author: David Brotz
 * File: BigGuy.h
 */
#ifndef __BIGGUY_H
#define __BIGGUY_H

#include "Herald.h"
#include "WorldState.h"
#include "World.h"
#include "BigGuyRelation.h"
#include "Retinue.h"

#include "sys/LinkedList.h"

#include <inttypes.h>

struct Plot;
struct Person;
struct Mission;
struct Trait;
struct Object;
struct Agent;

#define STAT_MIN (1)
#define STAT_MAX (100)

#define BG_MINGENSTATS (BGSKILL_SIZE * 40)
#define BG_MAXGENSTATS (BGSKILL_SIZE * 60)
#define SKILLCHECK_DEFAULT (100)

#define BigGuyHasPlot(_Guy) (BigGuyGetPlot(_Guy) != NULL)

#define BIGGUY_PERSONALITIES (4)


//These actions should be removed and only GOAP acions should be used instead.
enum {
	BGACT_NONE,
	BGACT_BEFRIEND,
	BGACT_SABREL,
	BGACT_STEAL,
	BGACT_DUEL,
	BGACT_MURDER,
	BGACT_GAINPOP,
	BGACT_SABPOP,
	BGACT_GAINGLORY,
	BGACT_SABGLORY,
	BGACT_PLOTOVERTHROW,
	BGACT_SIZE
};

enum {
	BGMOT_RULE,
	BGMOT_WEALTH,
	BGMOT_SIZE
};

enum {
	CRISIS_WARDEATH,
	CRISIS_SIZE
};

enum {
	BGSKILL_COMBAT,
	BGSKILL_STRENGTH,
	BGSKILL_TOUGHNESS,
	BGSKILL_AGILITY,
	BGSKILL_WIT,
	BGSKILL_CHARISMA,
	BGSKILL_INTELLIGENCE,
	BGSKILL_SIZE
};

extern const char* g_BGMission[BGACT_SIZE];
extern const char* g_CrisisStateStr[CRISIS_SIZE];
extern int g_BGActCooldown[BGACT_SIZE];

struct BigGuyMission {
	int Type;
};

struct BigGuyAction {
	int Type;
	struct BigGuy* Target;
	int Modifier;
	void* Data;
};

struct BigGuy {
	struct Object Object;
	struct Person* Person;
	struct Agent* Agent;
	struct BigGuyRelation* Relations;
	//void(*ActionFunc)(struct BigGuy*, const struct BigGuyAction*);
	int Personality;
	int Motivation;
	int TriggerMask; //Mask of all trigger types that have been fired recently.
	float Popularity; 
	float Glory;
	struct LinkedList PlotsAgainst;
	struct Trait** Traits;
	int16_t PopularityDelta;
	uint8_t TraitCt; 
	uint8_t IsDirty;
	uint8_t Stats[BGSKILL_SIZE]; //Array of all stats.
	uint8_t Action;
	struct BigGuy* ActionTarget;
};

struct BigGuyActionHist* CreateBGActionHist(struct BigGuy* _Owner, int _Action);
void DestroyBGActionHist(struct BigGuyActionHist* _Hist);
int BigGuyActionHistIS(const struct BigGuyActionHist* _One, const struct BigGuyActionHist* _Two);

struct BigGuy* CreateBigGuy(struct Person* _Person, uint8_t (*_Stats)[BGSKILL_SIZE], int _Motivation);
void DestroyBigGuy(struct BigGuy* _BigGuy);

void BigGuyThink(struct BigGuy* _Guy);

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyIdCmp(const int* _Two, const struct BigGuy* _BigGuy);
int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission);
void BigGuySetState(struct BigGuy* _Guy, int _State, int _Value);

struct BigGuy* BigGuyLeaderType(struct Person* _Person);


/**
 * Randomly distributes points to the stats that are provided in the variable argument.
 * _StatCt pointers to integers should be supplied to the variable argument followed by
 * _StatCt floats whose cumulative sum is 1. The float numbers represent the percentage of _Points
 * a random integer will receive.
 */
void BGStatsRandom(int _Points, int _StatCt, ...);
int BGRandRes(const struct BigGuy* _Guy, int _Stat);
/*
 * Creates a big guy with a stat emphasis on warfare.
 */
void BGStatsWarlord(uint8_t (*_Stats)[BGSKILL_SIZE], int _Points);

struct Trait** BGRandTraits();
int HasTrait(const struct BigGuy* _BigGuy, const struct Trait* _Trait);

void BigGuySetAction(struct BigGuy* _Guy, int _Action, struct BigGuy* _Target, void* _Data);
struct Settlement* BigGuyHome(struct BigGuy* _Guy);
/**
 * Return 1 if _Target's personality is one that _Guy would prefer to have as an acquaintance.
 * Return 0 if _Target's personality is not compatable.
 */
int BigGuyLikeTrait(const struct BigGuy* _Guy, const struct BigGuy* _Target);
/**
 * \return A number that exists in [0, 2] that should be used to modify all opinion
 * values that _Guy has of _Target.
 */
double BigGuyOpinionMod(const struct BigGuy* _Guy, const struct BigGuy* _Target);

/**
 * \brief
 * Compares  the  _Stat of _One and _Two and returns how sucessful the winner was.
 * If _One has a lower score than _Two a negative value will be returned.
 * If they are equal 0 will be returned.
 * If _Two is greater than _One in the check a positive value will be returned.
 * The value returned will be the diference of the two rolls divided by 10.
 */
int BigGuyOpposedCheck(const struct BigGuy* _One, const struct BigGuy* _Two, int _Skill); 
/**
 * \return A positive integer on sucess and zero on failure.
 */
int BigGuySkillCheck(const struct BigGuy* _Guy, int _Skill, int _PassReq);
/**
 * Similar to BigGuySkillCheck, returns how by how many multiples of 10 _Guy passes or
 * fails the skill check by.
 */
int BigGuySuccessMargin(const struct BigGuy* _Guy, int _Skill, int _PassReq);
/**
 * \return How many people in _Guy's settlement that currently like him.
 */
int BigGuyPopularity(const struct BigGuy* _Guy);
void BigGuyPlotTarget(struct BigGuy* _Guy, struct Plot* _Plot);
int BigGuyPlotPower(const struct BigGuy* _Guy);
/*
 * Returns the retinue _Leader controls or NULL if _Leader does not control a retinue.
 */ 
struct Retinue* BigGuyRetinue(const struct BigGuy* _Leader);

inline static struct Plot* BigGuyGetPlot(struct BigGuy* _Guy) {
	return RBSearch(&g_GameWorld.PlotList, _Guy);
}
#endif
