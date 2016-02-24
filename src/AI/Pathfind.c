/*
 * File: Pathfind.c
 * Author: David Brotz
 */

#include "Pathfind.h"

#include "../sys/MemoryPool.h"
#include "../sys/LinkedList.h"
#include "../sys/TaskPool.h"
#include "../World.h"
#include "../video/MapRenderer.h"
#include "../video/Tile.h"
#include "../sys/BinaryHeap.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <SDL2/SDL.h>

#define PATH_PATHPOOLSZ (1024)
#define PATH_PATHSCORESZ (2048)
#define PATHFIND_OPENSIZE (256)
#define PATHTABLE_SIZE (4)

struct PathStack {
	void* Stack[PATHTABLE_SIZE];
	int Top;
	SDL_mutex* Lock; //Ensures that when PathStackNext is called it cannot be called by another thread.
	SDL_sem* Sem; //Ensures that only PATHTABLE_SIZE number of pointers from Stack are given out.
};

static struct MemoryPool* g_PathDataPool = NULL;
static struct MemoryPool* g_PathPool = NULL;
static struct MemoryPool* g_PathScorePool = NULL;
static struct PathStack g_PathStack;

struct PathData {
	int (*Heuristic)(const void*, const void*);
	void (*Callback)(void*, struct Path*); //void* is the PathData's Data element and the Path* is the completed path.
	SDL_Point Start;
	SDL_Point End;
	struct Path* Path;
	void* Data;
};

struct PathNodeScore {
	int g;
	int h;
	int f;
	int Direction;
	const struct Tile* Node;
	const struct PathNodeScore* Parent;
};

void PathfindInit() {
	g_PathDataPool = CreateMemoryPool(sizeof(struct PathData), PATH_PATHPOOLSZ);
	g_PathPool = CreateMemoryPool(sizeof(struct Path), PATH_PATHPOOLSZ);
	g_PathScorePool = CreateMemoryPool(sizeof(struct PathNodeScore), PATH_PATHSCORESZ);
	for(int i = 0; i < PATHTABLE_SIZE; ++i)
		g_PathStack.Stack[i] = calloc(PATHFIND_OPENSIZE, sizeof(struct PathNodeScore*));
	g_PathStack.Lock = SDL_CreateMutex();
	g_PathStack.Top = 0;
	g_PathStack.Sem = SDL_CreateSemaphore(PATHTABLE_SIZE);
}

void PathfindQuit() {
	DestroyMemoryPool(g_PathDataPool);
	DestroyMemoryPool(g_PathPool);
	DestroyMemoryPool(g_PathScorePool);
	for(int i = 0; i < PATHTABLE_SIZE; ++i)
		free(g_PathStack.Stack[i]);
	SDL_DestroyMutex(g_PathStack.Lock);
	SDL_DestroySemaphore(g_PathStack.Sem);
}

int PathNodeScoreCmp(const void* _One, const void* _Two) {
	return ((struct PathNodeScore*)_Two)->f - ((struct PathNodeScore*)_One)->f;
}

struct PathNodeScore* CreatePathNodeScore(const struct Tile* _Node, int _g, int _h, int _Direction, struct PathNodeScore* _Parent) {
	struct PathNodeScore* _NodeScore = (struct PathNodeScore*) MemPoolAlloc(g_PathScorePool);

	_NodeScore->Node = _Node;
	_NodeScore->g = _g;
	_NodeScore->h = _h;
	_NodeScore->f = _g + _h;
	_NodeScore->Direction = _Direction;
	_NodeScore->Parent = _Parent;
	return _NodeScore;
}

void DestroyPathNodeScore(struct PathNodeScore* _Path) {
	MemPoolFree(g_PathScorePool, _Path);
}

struct Path* CreatePath(int _Dir, int _Tiles) {
	struct Path* _Path = MemPoolAlloc(g_PathPool);

	_Path->Direction = _Dir;
	_Path->Tiles = _Tiles;
	_Path->Next = NULL;
	return _Path;
}

void DestroyPath(struct Path* _Path) {
	MemPoolFree(g_PathPool, _Path);
}

void ListToPath(const struct LinkedList* _List, struct Path* _Path) {
	struct LnkLst_Node* _Itr = _List->Front->Next;
	const struct PathNodeScore* _Node = NULL;
	int _Ct = 0;
	int _Dir = TILE_SIZE;

	_Path->Direction = TILE_SIZE;
	_Path->Tiles = 1;
	if(_Itr != NULL) {
		_Node = ((struct PathNodeScore*)_Itr->Data);
		_Dir = _Node->Direction;
	} else
		return;
	do {
		if(_Dir == _Node->Direction) {
			++_Ct;
		} else {
			_Path->Next = CreatePath(_Dir, _Ct);
			_Path = _Path->Next;
			_Ct = 1;
			_Dir = _Node->Direction;
		}
		_Itr = _Itr->Next;
		if(_Itr == NULL)
			break;
		_Node = ((struct PathNodeScore*)_Itr->Data);
	} while(1);
	if(_Ct > 0)
		_Path->Next = CreatePath(_Dir, _Ct);
}

void* PathStackNext() {
	void* _Ptr = NULL;

	SDL_SemWait(g_PathStack.Sem);
	SDL_LockMutex(g_PathStack.Lock);
	_Ptr = g_PathStack.Stack[g_PathStack.Top];
	++g_PathStack.Top;
	if(g_PathStack.Top >= PATHTABLE_SIZE)
		g_PathStack.Top = 0;
	SDL_UnlockMutex(g_PathStack.Lock);
	return _Ptr;
}

void PathStackFree(void* _Ptr) {
	SDL_SemPost(g_PathStack.Sem);
	SDL_LockMutex(g_PathStack.Lock);
	int _Size = PATHTABLE_SIZE - SDL_SemValue(g_PathStack.Sem);
	int _Bottom = _Size + g_PathStack.Top;
	void* _Temp = NULL;

	if(_Bottom >= PATHTABLE_SIZE)
		_Bottom -= PATHTABLE_SIZE;
	for(int i = 0; i < PATHTABLE_SIZE; ++i)
		if(g_PathStack.Stack[i] == _Ptr) {
			_Temp = g_PathStack.Stack[i];
			g_PathStack.Stack[i] = g_PathStack.Stack[_Bottom];
			g_PathStack.Stack[_Bottom] = _Temp;
			break;
		}
	SDL_UnlockMutex(g_PathStack.Lock);
}

/*
 * Pathfind takes ownership of _Path and then releases ownership of _Path when it calls _Path's callback.
 */
int PathfindNext(struct PathData* _Path, void* _None) {
	const struct Tile* _StartTile = MapGetTile(g_GameWorld.MapRenderer, &_Path->Start);
	const struct Tile* _Goal = MapGetTile(g_GameWorld.MapRenderer, &_Path->End);
	struct Tile* _Neighbors[TILE_SIZE];
	/*
	 * TODO: Remove LinkedList from PathfindNext to remove malloc calls from PathfindNext.
	 */
	struct LinkedList _ClosedList = {0, NULL, NULL};
	struct LnkLst_Node* _Itr = NULL;
	struct BinaryHeap _OpenList = {NULL, PATHFIND_OPENSIZE, 0, PathNodeScoreCmp};
	struct PathNodeScore* _Current = NULL;

	_OpenList.Table = PathStackNext();
	BinaryHeapInsert(&_OpenList, CreatePathNodeScore(_StartTile, 0, _Path->Heuristic(_StartTile, _Goal), TILE_SIZE, NULL));
	while(_OpenList.Size > 0 && _OpenList.Size <= _OpenList.TblSz) {
		_Current = BinaryHeapTop(&_OpenList);
		LnkLstPushBack(&_ClosedList, BinaryHeapPop(&_OpenList));
		if(_Current->Node->TilePos.x == _Goal->TilePos.x && _Current->Node->TilePos.y == _Goal->TilePos.y)
			break;
		/*
		 * Fill the open list with _Tile's neighbors.
		 */
		TileGetAdjTiles(g_GameWorld.MapRenderer, _Current->Node, _Neighbors);
		for(int i = 0; i < TILE_SIZE; ++i) {
			if(_Neighbors[i] != NULL) {
				const struct LnkLst_Node* CloseItr = _ClosedList.Front;
				while(CloseItr != NULL) {
					const struct PathNodeScore* _Node = (struct PathNodeScore*)CloseItr->Data;
					if(_Node->Node == _Neighbors[i]) {
						goto loop_end;
					}
					CloseItr = CloseItr->Next;
				}
				/*
				 * Check if neighbors are already in open list.
				 */
				for(int j = 0; j < _OpenList.Size; ++j)  {
					struct PathNodeScore* _OpenTemp = (struct PathNodeScore*)_OpenList.Table[j];
					if(_OpenTemp->Node == _Neighbors[i]) {
						int _gCost = _Current->g + _Path->Heuristic(_OpenTemp->Node, _Neighbors[i]);
						if(_gCost < _OpenTemp->g) {
							_OpenTemp->g = _gCost;
							BinaryHeapIncrease(&_OpenList, j);
						}
						goto loop_end;
					}
				}
					BinaryHeapInsert(&_OpenList, CreatePathNodeScore(_Neighbors[i], _Current->g + 1, _Path->Heuristic(_Neighbors[i], _Goal), i, _Current));
					loop_end:
					continue;
			}
		}
	}
	while(_OpenList.Size > 0) {
		DestroyPathNodeScore((struct PathNodeScore*)BinaryHeapPop(&_OpenList));
	}
	PathStackFree(_OpenList.Table);
	_Itr = _ClosedList.Front;
	//ListToPath(&_ClosedList, _Path->Path);
	const struct PathNodeScore* _Temp = _Current;
	struct LinkedList _TempList = {0, NULL, NULL};
	while(_Temp != NULL) {
		LnkLstPushFront(&_TempList, (struct PathNodeScore*)_Temp);
		_Temp = _Temp->Parent;
	}
	ListToPath(&_TempList, _Path->Path);
	while(_Itr != NULL) {
		DestroyPathNodeScore((struct PathNodeScore*)_Itr->Data);
		_Itr = _Itr->Next;
	}
	LnkLstClear(&_ClosedList);
	_Path->Callback(_Path->Data, _Path->Path);
	MemPoolFree(g_PathDataPool, _Path);
	return 0;
}

void Pathfind(int _StartX, int _StartY, int _EndX, int _EndY, struct Path* _Path, void* _Data, int (*_Heuristic)(const void*, const void*), void(*_Callback)(void*, struct Path*)) {
	struct PathData* _PData = MemPoolAlloc(g_PathDataPool);

	_PData->Start.x = _StartX;
	_PData->Start.y = _StartY;
	_PData->End.x = _EndX;
	_PData->End.y = _EndY;
	_PData->Path = _Path;
	_PData->Heuristic = _Heuristic;
	_PData->Callback = _Callback;
	_PData->Data = _Data;
	TaskPoolAdd(g_TaskPool, g_TaskPool->Time, (int(*)(void*, void*))PathfindNext, _PData, NULL);
}
