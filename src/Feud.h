/*
 * File: Feud.h
 * Author: David Brotz
 */
#ifndef __FEUD_H
#define __FEUD_H

#include "Herald.h"

#include "sys/LinkedList.h"

struct Family;
struct BigGuy;

struct Feud {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	double Level;
	//Linked list of families on a side.
	struct BigGuy* Leader[2];
	struct LinkedList Side[2];
};

struct Feud* CreateFeud(struct BigGuy* _SideOne, struct BigGuy* _SideTwo);
void DestroyFeud(struct Feud* _Feud);

void FeudAddFamily(struct Feud* _Feud, struct Family* _Family, int _Side);
void FeudThink(struct Feud* _Feud);

#endif
