/*
 * File: BigGuy.h
 * Author: David Brotz
 */
#ifndef __BIGGUY_H
#define __BIGGUY_H

#include "Herald.h"
#include "WorldState.h"
#include "BigGuyRelation.h"

#include "sys/LinkedList.h"

#include <inttypes.h>

struct Person;
struct Mission;
struct Trait;
struct Object;
struct Feud;
struct Agent;

#define BIGGUYSTAT_MIN (1)
#define BIGGUYSTAT_MAX (100)

#define BG_MINGENSTATS (BGSKILL_SIZE * 60)
#define BG_MAXGENSTATS (BGSKILL_SIZE * 80)

#define BIGGUY_RELMAX (100)
#define BIGGUY_HATEMIN (-76)
#define BIGGUY_DISLIKEMIN (-26)
#define BIGGUY_LIKEMIN (25)
#define BIGGUY_LOVEMIN (75)
#define BIGGUY_TRAITREL (10)

#define BIGGUY_PERSONALITIES (4)


enum {
	BGBYTE_PASSREFORM = 0,
	BGBYTE_ISLEADER,
	BGBYTE_IMPROVINGRELATION,
	BGBYTE_AUTHORITY,
	BGBYTE_PRESTIGE,
	BGBYTE_FEUDCOUNT,
	BGBYTE_FYRDRAISED,
	BGBYTE_SUFFREINDS, //Has sufficient friends.
	BGBYTE_SUFENEMIES, //True if the number of enemies is acceptable.
	BGBYTE_ISMARRIED,
	BGBYTE_SIZE
};

//These actions should be removed and only GOAP acions should be used instead.
enum {
	BGACT_NONE,
	BGACT_IMRPOVEREL,
	BGACT_SABREL,
	BGACT_GIFT,
	BGACT_STEALCATTLE,
	BGACT_DUEL,
	BGACT_MURDER,
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
	BGSKILL_ADMINISTRATION,
	BGSKILL_INTRIGUE,
	BGSKILL_STRATEGY,
	BGSKILL_WARFARE,
	BGSKILL_TACTICS,
	BGSKILL_CHARISMA,
	BGSKILL_PIETY,
	BGSKILL_INTELLEGENCE,
	BGSKILL_SIZE
};

extern const char* g_BGStateStr[BGBYTE_SIZE];
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

/**
 * Note: Owner and ActionType are declared first to allow an incomplete BigGuyActionHist to 
 * search for a BigGuyActionHist.
 */
struct BigGuyActionHist {
	struct BigGuy* Owner;
	int ActionType;
	DATE DayDone;
};

struct BigGuy {
	int Id;
	int Type;
	ObjectThink Think;
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	struct Person* Person;
	struct Agent* Agent;
	int IsDirty;
	int Popularity; //number of non-big guys adults who like this person. To calculate how popular this person is to everyone call BigGuyPopularity.
	struct WorldState State;
	int TriggerMask; //Mask of all trigger types that have been fired recently.
	float Authority;
	float Prestige;
	struct BigGuyRelation* Relations;
	uint8_t Stats[BGSKILL_SIZE];
	struct BigGuyAction Action;
	struct LinkedList Feuds;
	int Personality;
	void(*ActionFunc)(struct BigGuy*, const struct BigGuyAction*);
	struct Trait** Traits;
	int Motivation;
};

struct Crisis {
	struct WorldState State;
	int TriggerMask;
	struct BigGuy* Guy;
};

struct BigGuyActionHist* CreateBGActionHist(struct BigGuy* _Owner, int _Action);
void DestroyBGActionHist(struct BigGuyActionHist* _Hist);
int BigGuyActionHistIS(const struct BigGuyActionHist* _One, const struct BigGuyActionHist* _Two);

struct Crisis* CreateCrisis(int _Type, struct BigGuy* _Guy);
void DestroyCrisis(struct Crisis* _Crisis);

int CrisisSearch(const struct Crisis* _One, const struct Crisis* _Two);
int CrisisInsert(const int* _One, const struct Crisis* _Two);

struct BigGuy* CreateBigGuy(struct Person* _Person, uint8_t _Stats[BGSKILL_SIZE], int _Motivation);
void DestroyBigGuy(struct BigGuy* _BigGuy);

void BigGuyThink(struct BigGuy* _Guy);

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyIdCmp(const int* _Two, const struct BigGuy* _BigGuy);
int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission);
void BigGuySetState(struct BigGuy* _Guy, int _State, int _Value);

struct BigGuy* BigGuyLeaderType(struct Person* _Person);


/*
 * Randomly distributes points to the stats that are provided in the variable argument.
 * _StatCt pointers to integers should be supplied to the variable argument followed by
 * _StatCt floats whose cumulative sum is 1. The float numbers represent the percentage of _Points
 * a random integer will receive.
 */
void BGStatsRandom(int _Points, int _StatCt, ...);
int BGRandPopularity(const struct BigGuy* _Guy);
/*
 * Creates a big guy with a stat emphasis on warfare.
 */
void BGStatsWarlord(uint8_t _Stats[BGSKILL_SIZE], int _Points);

void BGSetAuthority(struct BigGuy* _Guy, float _Authority);
void BGSetPrestige(struct BigGuy* _Guy, float _Prestige);

void BigGuySetAction(struct BigGuy* _Guy, int _Action, struct BigGuy* _Target, void* _Data);
void BigGuyAddFeud(struct BigGuy* _Guy, struct Feud* _Feud);
/*
 * Return 1 if _Target's personality is one that _Guy would prefer to have as an acquaintance.
 * Return 0 if _Target's personality is not compatable.
 */
int BigGuyLikeTrait(const struct BigGuy* _Guy, const struct BigGuy* _Target);
double BigGuyOpinionMod(const struct BigGuy* _Guy, const struct BigGuy* _Target);

/**
 * Compares  the  _Stat of _One and _Two and returns how sucessful the winner was.
 * If _One has a lower score than _Two a negative value will be returned.
 * If they are equal 0 will be returned.
 * If _Two is greater than _One in the check a positive value will be returned.
 * The value returned will be the diference of the two rolls divided by 10.
 */
int BigGuyOpposedCheck(const struct BigGuy* _One, const struct BigGuy* _Two, int _Skill); 
/**
 * Returns a positive integer on sucess and a negative number on failure.
 */
int BigGuySkillCheck(const struct BigGuy* _Guy, int _Skill);
int BigGuyPopularity(const struct BigGuy* _Guy);
#endif
