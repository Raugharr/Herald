/*
 * File: AIHelper.h
 * Author: David Brotz
 */
#ifndef __AIHELPER_H
#define __AIHELPER_H

struct LinkedList;

extern struct Queue g_PathQueue;

enum {
	EDIR_NORTH,
	EDIR_EAST,
	EDIR_SOUTH,
	EDIR_WEST,
	EDIR_NONE
};

struct Path {
	int Direction;
	int Tiles;
	struct Path* Next;
};

void PathfindInit();
void PathfindQuit();

struct Path* CreatePath();
void DestroyPath(struct Path* _Path);

void PathfindNext();
void ListToPath(struct Path* _Path, struct LinkedList* _List);
void Pathfind(int _StartX, int _StartY, int _EndX, int _EndY, struct Path* _Path);

#endif
