/*
 * File: BigGuy.h
 * Author: David Brotz
 */
#ifndef __BIGGUY_H
#define __BIGGUY_H

#include "Herald.h"
#include "WorldState.h"

#include "sys/LinkedList.h"

#include <inttypes.h>

struct Person;
struct Mission;
struct Trait;
struct Object;
struct Feud;
struct Agent;

#define BIGGUYSTAT_MIN (0)
#define BIGGUYSTAT_MAX (100)

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
	BGBYTE_FYRDRAISED,
	BGBYTE_FEUDCOUNT,
	BGBYTE_SIZE
};

enum {
	BGREL_HATE,
	BGREL_DISLIKE,
	BGREL_NEUTURAL,
	BGREL_LIKE,
	BGREL_LOVE
};

enum {
	OPINION_NONE,
	OPINION_TOKEN,
	OPINION_SMALL,
	OPINION_AVERAGE,
	OPINION_GREAT
};

enum {
	ACTTYPE_THEFT,
	ACTTYPE_RUMOR,
	ACTTYPE_TRAIT,
	ACTTYPE_RAISEFYRD,
	ACTTYPE_ATTACK,
	ACTTYPE_GIFT,
	ACTTYPE_WARLACK,
	ACTTYPE_SIZE
};
//These actions should be removed and only GOAP acions should be used instead.
enum {
	BGACT_NONE,
	BGACT_IMRPOVEREL,
	BGACT_SABREL,
	BGACT_GIFT,
	BGACT_STEALANIMAL,
	BGACT_DUEL,
	BGACT_SIZE
};

enum {
	BGMOT_RULE,
	BGMOT_SIZE
};

enum {
	CRISIS_WARDEATH,
	CRISIS_SIZE
};

extern const char* g_BGStateStr[BGBYTE_SIZE];
extern const char* g_BGMission[BGACT_SIZE];
extern const char* g_CrisisStateStr[CRISIS_SIZE];

struct BigGuyMission {
	int Type;
};

struct BigGuyStats {
	uint8_t Administration;
	uint8_t Intrigue;
	uint8_t Strategy;
	uint8_t Warfare;
	uint8_t Tactics;
	uint8_t Charisma;
	uint8_t Piety;
	uint8_t Intellegence;
};

struct BigGuyOpinion {
	int Action;
	int RelMod;
	struct BigGuyOpinion* Next;
};

struct BigGuyRelation {
	int Relation;
	int Modifier;
	struct BigGuy* Person;
	struct BigGuyOpinion* Opinions;
	struct BigGuyRelation* Next;
};

struct BigGuyAction {
	int Type;
	struct BigGuy* Target;
	int Modifier;
	void* Data;
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
	struct WorldState State;
	int TriggerMask; //Mask of all trigger types that have been fired recently.
	float Authority;
	float Prestige;
	struct BigGuyRelation* Relations;
	struct BigGuyStats Stats;
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

struct Crisis* CreateCrisis(int _Type, struct BigGuy* _Guy);
void DestroyCrisis(struct Crisis* _Crisis);

int CrisisSearch(const struct Crisis* _One, const struct Crisis* _Two);
int CrisisInsert(const int* _One, const struct Crisis* _Two);

struct BigGuyRelation* CreateBigGuyRelation(struct BigGuy* _Guy, struct BigGuy* _Actor);
struct BigGuyOpinion* CreateBigGuyOpinion(struct BigGuyRelation* _Relation, int _Action, int _Modifier);
struct BigGuy* CreateBigGuy(struct Person* _Person, struct BigGuyStats* _Stats);
void DestroyBigGuy(struct BigGuy* _BigGuy);

void BigGuyThink(struct BigGuy* _Guy);

int BigGuyIdInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyIdCmp(const int* _Two, const struct BigGuy* _BigGuy);
int BigGuyStateInsert(const struct BigGuy* _One, const struct BigGuy* _Two);
int BigGuyMissionCmp(const struct BigGuy* _BigGuy, const struct Mission* _Mission);
void BigGuySetState(struct BigGuy* _Guy, int _State, int _Value);

struct BigGuy* BigGuyLeaderType(struct Person* _Person);

void BigGuyAddRelation(struct BigGuy* _Guy, struct BigGuyRelation* _Relation, int _Action, int _Modifier);
void BigGuyChangeOpinion(struct BigGuy* _Guy, struct BigGuy* _Target, int _Action, int _Modifier);
/*
 * Recalculates the modifier variable of _Relation and then updates Relation if applicable.
 */
void BigGuyRelationUpdate(struct BigGuyRelation* _Relation);
struct BigGuyRelation* BigGuyGetRelation(const struct BigGuy* _Guy, const struct BigGuy* _Target);

/*
 * Randomly distributes points to the stats that are provided in the variable argument.
 * _StatCt pointers to integers should be supplied to the variable argument followed by
 * _StatCt floats whose cumulative sum is 1. The float numbers represent the percentage of _Points
 * a random integer will receive.
 */
void BGStatsRandom(int _Points, int _StatCt, ...);
/*
 * Creates a big guy with a stat emphasis on warfare.
 */
void BGStatsWarlord(struct BigGuyStats* _Stats, int _Points);

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

#endif
