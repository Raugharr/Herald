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

struct Family {
	int Id;
	int NumChildren;
	const char* Name;
	struct Person** People;
	struct Field* Field;
	struct Array* Buildings;
	struct Array* Goods;
};

void Family_Init(struct Array* _Array);
void Family_Quit();
struct Family* CreateFamily(const char* _Name, struct Person* _Husband, struct Person* _Wife, struct Person** _Children, int _ChildrenSize);
struct Family* CreateRandFamily(const char* _Name, int _Size);
void DestroyFamily(struct Family* _Family);
int FamilySize(struct Family* _Family);
void Marry(struct Person* _Male, struct Person* _Female);

#endif

