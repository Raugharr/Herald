/*
 * File: BigGuy.h
 * Author: David Brotz
 */
#ifndef __BIGGUY_H
#define __BIGGUY_H

#include "WorldState.h"

#include <inttypes.h>

struct Person;
struct Mission;
struct Object;

#define BIGGUYSTAT_MIN (0)
#define BIGGUYSTAT_MAX (100)

#define BIGGUY_RELMAX (100)
#define BIGGUY_HATEMIN (-76)
#define BIGGUY_DISLIKEMIN (-26)
#define BIGGUY_LIKEMIN (25)
#define BIGGUY_LOVEMIN (75)

/*enum {
	BGSTATE_PASSREFORM = (1 << 0),
	BGSTATE_ISLEADER = (1 << 1),
	BGSTATE_IMPROVINGRELATION = (1 << 2),
	BGSTATE_AUTHORITY = (1 << 3),
	BGSTATE_PRESTIGE = (1 << 4),
	BGSTATE_SIZE = 5
};*/

enum {
	BGBYTE_PASSREFORM = 0,
	BGBYTE_ISLEADER,
	BGBYTE_IMPROVINGRELATION,
	BGBYTE_AUTHORITY,
	BGBYTE_PRESTIGE,
	BGBYTE_FYRDRAISED,
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
	BGOPIN_IMPROVREL
};

enum {
	BGACT_NONE,
	BGACT_IMRPOVEREL,
	BGACT_SIZE
};

extern const char* g_BGStateStr[BGBYTE_SIZE];
extern const char* g_BGMission[BGACT_SIZE];

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
	const struct BigGuy* Person;
	struct BigGuyOpinion* Opinions;
	struct BigGuyRelation* Next;
};

struct BigGuyAction {
	void(*ActionFunc)(struct BigGuy*, void*);
	void* Data;
};

struct BigGuy {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	struct Person* Person;
	int IsDirty;
	struct WorldState State;
	float Authority;
	float Prestige;
	struct BigGuyRelation* Relations;
	struct BigGuyStats Stats;
	struct BigGuyAction Action;
};

struct BigGuyRelation* CreateBigGuyRelation(struct BigGuy* _Guy, const struct BigGuy* _Actor, int _Action, int _Modifier);
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
void BigGuyRelationUpdate(struct BigGuyRelation* _Relation);
struct BigGuyRelation* BigGuyGetRelation(const struct BigGuy* _Guy, const struct BigGuy* _Target);

void BGStatsRandom(int _Points, int _StatCt, ...);
void BGStatsWarlord(struct BigGuyStats* _Stats, int _Points);

void BGSetAuthority(struct BigGuy* _Guy, float _Authority);
void BGSetPrestige(struct BigGuy* _Guy, float _Prestige);

void BigGuySetAction(struct BigGuy* _Guy, int _Action, void* _Data);

#endif
