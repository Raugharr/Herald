/*
 * File: GameObject.h
 * Author: David Brotz
 */
#ifndef __GAMEOBJECT_H
#define __GAMEOBJECT_H

enum {
	GAMEOBJ_CANCOLLIDE = (1 << 0)
};

struct GameObject {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	struct Sprite* Sprite;
	int TilePos;
	int Flags;
};

#endif
