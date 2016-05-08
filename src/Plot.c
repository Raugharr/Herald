/**
 * Author: David Brotz
 * File: Plot.c
 */
#include "Plot.h"

#include "BigGuy.h"

#include <stdlib.h>
#include <assert.h>

struct Plot* CreatePlot(int _Type, struct BigGuy* _Owner, struct BigGuy* _Target) {
	struct Plot* _Plot = (struct Plot*) malloc(sizeof(struct Plot));

	_Plot->Type = _Type;
	ConstructLinkedList(&_Plot->Side[0]);
	ConstructLinkedList(&_Plot->SideAsk[0]);
	ConstructLinkedList(&_Plot->Side[1]);
	ConstructLinkedList(&_Plot->SideAsk[1]);
	LnkLstPushBack(&_Plot->Side[PLOT_ATTACKERS], _Owner);
	LnkLstPushBack(&_Plot->Side[PLOT_DEFENDERS], _Target);
	return _Plot;
}


int PlotCanAsk(const struct Plot* _Plot, struct BigGuy* _Guy) {
	int _Idx = 0;
	struct LnkLst_Node* _Itr = NULL;

	if(_Guy == PlotLeader(_Plot))
		_Idx = 0;
	else if(_Guy == PlotTarget(_Plot))
		_Idx = 1;
	else
		return 0;
	_Itr = _Plot->SideAsk[_Idx].Front->Next;
	while(_Itr != NULL) {
		if(((struct BigGuy*)_Itr->Data) == _Guy)
			return 0;
		_Itr = _Itr->Next;
	}
	return 1;
}

void PlotJoin(struct Plot* _Plot, int _Side, struct BigGuy* _Guy) {
	assert(PlotCanAsk(_Plot, _Guy) == 1);
	assert(_Side == PLOT_ATTACKERS || _Side == PLOT_DEFENDERS);
	LnkLstPushBack(&_Plot->Side[_Side], _Guy);
}

int PlotInsert(const struct Plot* _One, const struct Plot* _Two) {
	return PlotLeader(_One)->Id - PlotLeader(_Two)->Id;
}

int PlotSearch(const struct BigGuy* _One, const struct Plot* _Two) {
	return _One->Id - PlotLeader(_Two)->Id;
}

int IsInPlot(const struct Plot* _Plot, struct BigGuy* _Guy) {
	struct LnkLst_Node* _Itr = NULL;

	_Itr = _Plot->Side[0].Front;
	while(_Itr != NULL) {
		if(_Itr->Data == _Guy)
			return 1;
		_Itr = _Itr->Next;	
	}
	_Itr = _Plot->Side[1].Front;
	while(_Itr != NULL) {
		if(_Itr->Data == _Guy)
			return 2;
		_Itr = _Itr->Next;	
	}
	return 0;
}

struct BigGuy*  PlotLeader(const struct Plot* _Plot) {
	return _Plot->Side[PLOT_ATTACKERS].Front->Data;
}

struct BigGuy* PlotTarget(const struct Plot* _Plot) {
	return _Plot->Side[PLOT_DEFENDERS].Front->Data;
}
