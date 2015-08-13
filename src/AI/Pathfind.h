/*
 * File: Pathfind.h
 * Author: David Brotz
 */
#ifndef __PATHFIND_H
#define __PATHFIND_H

struct LinkedList;
struct PathData;

struct Path {
	int Direction;
	int Tiles;
	struct Path* Next;
};


void PathfindInit();
void PathfindQuit();

struct Path* CreatePath(int _Dir, int _Tiles);
void DestroyPath(struct Path* _Path);

int PathfindNext(struct PathData* _Path, void* _None);
void ListToPath(const struct LinkedList* _List, struct Path* _Path);
void Pathfind(int _StartX, int _StartY, int _EndX, int _EndY, struct Path* _Path, void* _Data, int (*_Heuristic)(const void*, const void*), void(*_Callback)(void*, struct Path*));

#endif
