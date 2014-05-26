/*
 * Author: David Brotz
 * File: Family.h
 */

#ifndef __FAMILY_H
#define __FAMILY_H

#define CHILDREN_SIZE (8)

struct Person;

struct Family {
	const char* Name;
	struct Person* Husband;
	struct Person* Wife;
	struct Person** Children;
	int NumChildren;
};

struct Family* CreateFamily(const char* _Name, struct Person* _Husband, struct Person* _Wife, struct Person** _Children, int _ChildrenSize);
struct Family* CreateRandFamily(const char* _Name, int _Size);
void DestroyFamily(struct Family* _Family);
int Family_Size(struct Family* _Family);
void Marry(struct Person* _Male, struct Person* _Female);

#endif

