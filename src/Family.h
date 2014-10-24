/*
 * Author: David Brotz
 * File: Family.h
 */

#ifndef __FAMILY_H
#define __FAMILY_H

#define CHILDREN_SIZE (8)

#define HUSBAND (0)
#define WIFE (1)
#define CHILDREN (2)

struct Person;
struct Array;
struct Field;
struct Constraint;
struct FamilyType;
typedef struct lua_State lua_State;

struct Family {
	int Id;
	int NumChildren;
	const char* Name;
	struct Person* People[CHILDREN_SIZE + 2];
	struct Field* Field;
	struct Array* Buildings;
	struct Array* Goods;
	struct Array* Animals;
};

void Family_Init(struct Array* _Array);
void Family_Quit();
struct Family* CreateFamily(const char* _Name, struct Person* _Husband, struct Person* _Wife, struct Person** _Children, int _ChildrenSize);
struct Family* CreateRandFamily(const char* _Name, int _Size, struct Constraint** _AgeGroups, struct Constraint** _BabyAvg, int _X, int _Y);
void DestroyFamily(struct Family* _Family);
int FamilySize(struct Family* _Family);
void Marry(struct Person* _Male, struct Person* _Female);
void FamilyAddGoods(struct Family* _Family, lua_State* _State, struct FamilyType** _FamilyTypes, int _X, int _Y);

#endif

