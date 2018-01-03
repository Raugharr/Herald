/**
 * Author: David Brotz
 * IntervalTree.h
 */

#ifndef __INTERVALTREE_H
#define __INTERVALTREE_H

#include <limits.h>

struct LinkedList;

typedef int (*ITreeGetVal)(const void*);

//NOTE: We can save 4 bytes by excluding the Split field from all nodes that are a leaf.
struct ITreeNode {
	void** Table;
	int TableSz;
	int Split;
};

struct IntervalTree {
	struct ITreeNode* Table;
	int TableSz;
	ITreeGetVal ValFunc;
};

const struct IntervalTree* ConstructITree(void** _Values, int _ValueSz, ITreeGetVal _MinFunc, ITreeGetVal _MaxFunc);
void DestroyITree(const struct IntervalTree* _Tree);

void ITreeQuery_Aux(const struct IntervalTree* _Tree, int _NodeIdx, int _Val, struct LinkedList* _List, int _Min, int _Max);
static inline void ITreeQuery(const struct IntervalTree* _Tree, int _Val, struct LinkedList* _List) {
	if(_Tree->TableSz <= 0)
		return;
	ITreeQuery_Aux(_Tree, 0, _Val, _List, INT_MIN, INT_MAX);
}

#endif


