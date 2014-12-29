/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "World.h"
#include "Good.h"
#include "Occupation.h"
#include "Population.h"
#include "sys/Stack.h"
#include "sys/RBTree.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "sys/Array.h"
#include "sys/LuaHelper.h"
#include "sys/Constraint.h"
#include "sys/KDTree.h"
#include "sys/Event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define AGEDIST_SIZE (17)

struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_BuildMats;
struct HashTable g_Occupations;
struct HashTable g_Populations;
struct ATimer g_ATimer;
int g_ObjPosBal = 2;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeConstraints;

int g_Id = 0;
const char* g_ShortMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int IdISCallback(const int* _One, const int* _Two) {
	return *(_One) - *(_Two);
}

void HeraldInit() {
	g_Crops.TblSize = 0;
	g_Crops.Table = NULL;
	g_Crops.Size = 0;

	g_Goods.TblSize = 0;
	g_Goods.Table = NULL;
	g_Goods.Size = 0;

	g_BuildMats.TblSize = 0;
	g_BuildMats.Table = NULL;
	g_BuildMats.Size = 0;

	g_Occupations.TblSize = 0;
	g_Occupations.Table = NULL;
	g_Occupations.Size = 0;

	g_Populations.TblSize = 0;
	g_Populations.Table = NULL;
	g_Populations.Size = 0;

	g_ATimer.Tree = CreateRBTree((int(*)(const void*, const void*))ATimerICallback, (int(*)(const void*, const void*))ATimerSCallback);
	g_ATimer.ATypes = calloc(ATT_SIZE, sizeof(struct ATType*));
	ATimerAddType(&g_ATimer, CreateATType(ATT_PREGANCY, (int(*)(void*))PregancyUpdate, (void(*)(void*))DestroyPregancy));
	ATimerAddType(&g_ATimer, CreateATType(ATT_CONSTRUCTION, (int(*)(void*))ConstructUpdate, (void(*)(void*))DestroyConstruct));

	g_FamilySize = CreateConstrntBnds(5, 1, 5, 15, 40, 75, 100);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	EventInit();
}

void HeraldDestroy() {
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeConstraints);
	ATTimerRmAll(&g_ATimer);
	EventQuit();
}

struct InputReq* CreateInputReq() {
	struct InputReq* _Mat = (struct InputReq*) malloc(sizeof(struct InputReq));

	_Mat->Req = NULL;
	_Mat->Quantity = 0;
	return _Mat;
}

void DestroyInputReq(struct InputReq* _Mat) {
	free(_Mat);
}

int InputReqQtyCmp(const void* _One, const void* _Two) {
	return (int) ((struct InputReq*)_One)->Quantity - ((struct InputReq*)_Two)->Quantity;
}

struct Array* FileLoad(const char* _File, char _Delimiter) {
	int _Pos = 0;
	int _Size = 1;
	char* _Name = NULL;
	char _Char = 0;
	char _Buffer[256];
	FILE* _FilePtr = fopen(_File, "r");
	struct Array* _Array = NULL;

	if(_FilePtr == NULL)
		return NULL;

	while((_Char = fgetc(_FilePtr)) != EOF) {
		if(_Char == _Delimiter)
			++_Size;
	}
	rewind(_FilePtr);
	_Array = CreateArray(_Size);
	while((_Char = fgetc(_FilePtr)) != EOF) {
			if(_Char == _Delimiter && _Pos > 0) {
				_Buffer[_Pos] = 0;
				_Name = (char*) malloc(sizeof(char) * _Pos + 1);
				_Name[0] = 0;
				strncat(_Name, _Buffer, _Pos);
				ArrayInsert(_Array, _Name);
				_Pos = 0;
			} else {
				if(_Pos >= 256)
					return _Array;
				_Buffer[_Pos++] = _Char;
			}
		}
	return _Array;
}

struct Array* ListToArray(const struct LinkedList* _List) {
	struct Array* _Array = NULL;
	struct LnkLst_Node* _Itr = _List->Front;

	if(_List->Size < 1)
		return NULL;
	_Array = CreateArray(_List->Size);
	while(_Itr != NULL) {
		ArrayInsert(_Array, _Itr->Data);
		_Itr = _Itr->Next;
	}
	return _Array;
}

void* PowerSet_Aux(void* _Tbl, int _Size, int _ArraySize, struct StackNode* _Stack) {
	struct StackNode _Node;
	void** _Return = NULL;
	int i;

	if(_Size == 0) {
		if(_Stack == NULL) {
			_Return = malloc(sizeof(void*));
			*((int**)_Return) = 0;
			goto end;
		}
		_Return = calloc(_ArraySize + 1, sizeof(void*));
		for(i = _ArraySize - 1; i >= 0; --i) {
			_Return[i] = _Stack->Data;
			_Stack = _Stack->Prev;
		}
		_Return[_ArraySize] = NULL;
	} else {
		int _Len = 0;
		void** _Left = NULL;
		void** _Right = NULL;

		_Node.Prev = _Stack;
		_Node.Data = (void*)*(int*)_Tbl;
		_Left = PowerSet_Aux(_Tbl + sizeof(void*), _Size - 1, _ArraySize, _Stack);
		_Right = PowerSet_Aux(_Tbl + sizeof(void*), _Size - 1, _ArraySize + 1, &_Node);

		if(_Size == 1) {
			_Return = calloc(3, sizeof(void*));
			_Return[0] = _Left;
			_Return[1] = _Right;
			_Return[2] = NULL;
			goto end;
		}
		_Len = ArrayLen(_Left);
		_Return = calloc(_Len * 2 + 1, sizeof(void*));
		for(i = 0; i < _Len; ++i) {
			_Return[i] = _Left[i];
		}
		for(i = 0; i < _Len; ++i)
			_Return[i + _Len] = _Right[i];
		_Return[_Len * 2] = NULL;
	}
	end:
	return _Return;
}

void CreateObject(struct Object* _Obj, int _X, int _Y, int (*_Think)(struct Object*)) {
	struct Array* _Table = NULL;
	int _Pos[2];

	_Obj->Id = NextId();
	_Obj->X = _X;
	_Obj->Y = _Y;
	_Obj->Think = _Think;

	_Pos[0] = _X;
	_Pos[1] = _Y;
	if((_Table = KDSearch(&g_ObjPos, _Pos)) == NULL) {
		_Table = CreateArray(128);
		KDInsert(&g_ObjPos, _Table, _X, _Y);
	}
	ArrayInsertSort_S(_Table, _Obj, (int(*)(const void*, const void*)) IdISCallback);
	if(g_ObjPos.Size >= g_ObjPosBal) {
		g_ObjPosBal *= 2;
		KDBalance(&g_ObjPos);
	}
}

int ObjNoThink(struct Object* _Obj) {
	return 1;
}

int NextId() {return g_Id++;}

int ObjCmp(const void* _One, const void* _Two) {
	return *((int*)_One) - *((int*)_Two);
}

DATE MonthToInt(const char* _Month) {
	int i;

	for(i = 0; i < MONTHS; ++i)
		if(strcmp(_Month, g_ShortMonths[i]) == 0)
			return i;
	return -1;
}

DATE DaysBetween(int _DateOne, int _DateTwo) {
	if(_DateTwo < _DateOne)
		return 0;
	return DateToDays(_DateTwo) - DateToDays(_DateOne);
}

DATE DateToDays(int _Date) {
	int _Total = 0;
	int _Years = YEAR(_Date);
	int _Months = MONTH(_Date);
	int _Result = 0;

	_Total = _Years * YEAR_DAYS;
	_Result = _Months / 2;
	_Total += (_Result + 1) * 31;
	_Total += _Result * 30;
	if(_Months >= 1) {
		if(_Years % 4 == 0)
			_Total += 28;
		else
			_Total += 29;
	}
	if(_Months >= 8)
		++_Total;
	return _Total;
}

DATE DaysToDate(int _Days) {
	int _Years = 0;
	int _Months = 0;

	while(_Days >= YEAR_DAYS) {
		_Days -= YEAR_DAYS;
		++_Years;
	}

	while(_Days >= MONTH_DAYS) {
		_Days -= MONTH_DAYS;
		++_Months;
	}
	return (_Years << 9) | (_Months << 5) | (_Days);
}

void NextDay(int* _Date) {
	int _Day = DAY(*_Date);
	int _Month = MONTH(*_Date);
	int _Year = YEAR(*_Date);

	if((_Month & 1) == 0 || _Month == 7) {
		if(_Day == 31)
			goto new_month;
	} else if(_Month == 1) {
		if(_Day == 28 || ((_Year % 4) == 0 && _Day == 29))
			goto new_month;
	} else if(_Day == 30)
		goto new_month;
	++_Day;
	if(_Month >= 12) {
		++_Year;
		_Month = 0;
	}
	end:
	*_Date = TO_DATE(_Year, _Month, _Day);
	return;
	new_month:
	_Day = 0;
	++_Month;
	goto end;
}

void* DownscaleImage(void* _Image, int _Width, int _Height, int _ScaleArea) {
	int _NewWidth = (_Width / _ScaleArea);
	int _NewSize = _NewWidth * (_Height / _ScaleArea);
	int x = 0;
	int y = 0;
	int i = 0;
	int _Ct = 0;
	int _Avg = 0;
	int _AvgCt = _ScaleArea * _ScaleArea;
	void* _NewImg = calloc(sizeof(int), _NewSize);

	for(i = 0; i < _NewSize; ++i) {
		for(x = 0; x < _ScaleArea; ++x)
			for(y = 0; y < _ScaleArea; ++y)
				_Avg += ((int*)_Image)[y * _ScaleArea + (_Ct * _ScaleArea + x)];
		++_Ct;
		if(_Ct > _NewWidth)
			_Ct = 0;
		((int*)_NewImg)[i] = _Avg / _AvgCt;
		_Avg = 0;
	}
	return _NewImg;
}
