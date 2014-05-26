/*
 * File: Building.c
 * Author: David Brotz
 */

#include "Building.h"

#include <stdlib.h>
#include <string.h>

struct Building* CreateBuilding(const char* _Name, struct Good* _Output, int _Tax, int _Throughput, int _SquareFeet) {
	struct Building* _Building = (struct Building*) malloc(sizeof(struct Building));

	_Building->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Building->OutputGood = _Output;
	_Building->Price = 0;
	_Building->Tax = _Tax;
	_Building->Throughput = _Throughput;
	_Building->Size = _SquareFeet;
	strcpy(_Building->Name, _Name);
	return _Building;
}

struct Building* CopyBuilding(const struct Building* _Building, struct Good* _Good) {
	struct Building* _NewBuilding = (struct Building*) malloc(sizeof(struct Building));

	_NewBuilding->Name = (char*) malloc(sizeof(char) * strlen(_Building->Name) + 1);
	_NewBuilding->OutputGood = _Building->OutputGood;
	_NewBuilding->Price = _Building->Price;
	_NewBuilding->Tax = _Building->Tax;
	_NewBuilding->Throughput = _Building->Throughput;
	_NewBuilding->Size = _Building->Size;
	strcpy(_NewBuilding->Name, _Building->Name);
	return _NewBuilding;
}

void DestroyBuilding(struct Building* _Building) {
	free(_Building->Name);
	free(_Building);
}


