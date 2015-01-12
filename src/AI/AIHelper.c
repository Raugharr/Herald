/*
 * File: AIHelper.c
 * Author: David Brotz
 */

#include "AIHelper.h"

#include "../sys/Queue.h"
#include "../sys/MemoryPool.h"
#include "../sys/LinkedList.h"

#include <stdlib.h>
#include <string.h>

#define PATH_QUEUESZ (128)
#define PATH_PATHPOOLSZ (1024)

struct Queue g_PathQueue = {NULL, 0, 0, 0};
struct MemoryPool* g_PathDataPool = NULL;
struct MemoryPool* g_PathPool = NULL;

struct PathData {
	int (*Heuristic)(const void*, const void*);
	void** (*Neighbors)(const void*);
	void* (*GetTile)(int, int);
	int NeighborCt;
	int StartX;
	int StartY;
	int EndX;
	int EndY;
	struct Path* Path;
};

struct PathNodeScore {
	int Score;
	int Direction;
	void* Node;
};

void PathfindInit() {
	g_PathQueue.Table = calloc(PATH_QUEUESZ, sizeof(struct PathData*));
	g_PathDataPool = CreateMemoryPool(sizeof(struct PathData), PATH_QUEUESZ);
	g_PathPool = CreateMemoryPool(sizeof(struct Path), PATH_PATHPOOLSZ);
}

struct PathNodeScore* CreatePathNodeScore(void* _Node, int _Score, int _Direction) {
	struct PathNodeScore* _NodeScore = (struct PathNodeScore*) malloc(sizeof(struct PathNodeScore));

	_NodeScore->Node = _Node;
	_NodeScore->Score = _Score;
	_NodeScore->Direction = _Direction;
	return _NodeScore;
}

void DestroyPathNodeScore(struct PathNodeScore* _Path) {
	free(_Path);
}

struct Path* CreatePath() {
	struct Path* _Path = MemPoolAlloc(g_PathPool);

	_Path->Direction = 0;
	_Path->Tiles = 0;
	_Path->Next = NULL;
	return _Path;
}

void DestroyPath(struct Path* _Path) {
	MemPoolFree(g_PathPool, _Path);
}

void PathfindQuit() {
	free(g_PathQueue.Table);
	DestroyMemoryPool(g_PathDataPool);
}

void PathfindNext() {
	struct PathData* _Path = QueuePop(&g_PathQueue);
	struct LinkedList _ClosedList = {0, NULL, NULL};
	struct PathNodeScore* _Tile = (struct PathNodeScore*) malloc(sizeof(struct PathNodeScore));
	void* _StartTile = _Path->GetTile(_Path->StartX, _Path->StartY);
	struct PathNodeScore* _HighTile = NULL;
	int _GScore = 0;
	int i = 0;
	int _NeighborCt = _Path->NeighborCt;
	struct PathNodeScore* _OpenList[_NeighborCt];
	void** _Neighbors = NULL;

	memset(_OpenList, 0, _NeighborCt);
	_Tile->Score = 0;
	_Tile->Node = _Path->GetTile(_Path->EndX, _Path->EndY);
	_Tile->Direction = EDIR_NONE;
	LnkLstPushBack(&_ClosedList, _Tile);
	_OpenList[0] = _Tile;
	while(_OpenList[0] != 0) {
		_Tile = _OpenList[0];
		/*
		 * Fill the open list with _Tile's neighbors.
		 */
		_Neighbors = _Path->Neighbors(_OpenList[0]);
		for(i = 0; i < _NeighborCt; ++i)
			_OpenList[i] = CreatePathNodeScore(_Neighbors[i], _GScore + _Path->Heuristic(_Neighbors[i], _StartTile), i);

		/*
		 * Find the tile with the best score.
		 */
		_HighTile = _OpenList[0];
		_OpenList[0] = NULL;
		for(i = 1; i < _NeighborCt; ++i) {
			if(_OpenList[i]->Score > _HighTile->Score) {
				free(_HighTile);
				_HighTile = _OpenList[i];
				_OpenList[i] = NULL;
			}
		}
		/*
		 * Free unused tiles, add highest tile to list.
		 */
		for(i = 0; i < _NeighborCt; ++i)
			free(_OpenList[i]);
		LnkLstPushBack(&_ClosedList, _HighTile);
		++_GScore;
		memset(_OpenList, 0, _NeighborCt);
	}
}

void ListToPath(struct Path* _Path, struct LinkedList* _List) {
	struct LnkLst_Node* _Itr = _List->Front->Next;
	int _LastDir = ((struct PathNodeScore*)_Itr->Data)->Direction;
	struct Path* _PathItr = NULL;

	_PathItr = CreatePath();
	_PathItr->Direction = _LastDir;
	_PathItr->Tiles = 1;
	_Path->Next = _PathItr;
	DestroyPathNodeScore(_Itr->Data);
	_Itr = _Itr->Next;
	while(_Itr != NULL) {
		if(((struct PathNodeScore*)_Itr->Data)->Direction == _PathItr->Direction)
			++_PathItr->Tiles;
		else {
			_PathItr->Next = CreatePath();
			_PathItr = _PathItr->Next;
			_PathItr->Direction = ((struct PathNodeScore*)_Itr->Data)->Direction;
			_PathItr->Tiles = 1;
		}
		_Itr = _Itr->Next;
		DestroyPathNodeScore(_Itr->Data);
	}
}

void Pathfind(int _StartX, int _StartY, int _EndX, int _EndY, struct Path* _Path) {
	struct PathData* _PData = MemPoolAlloc(g_PathDataPool);

	_PData->StartX = _StartX;
	_PData->StartY = _StartY;
	_PData->EndX = _EndX;
	_PData->EndY = _EndY;
	_PData->Path = _Path;
	QueuePush(&g_PathQueue, _PData);
}
