/**
 * File: RedBlackTest.c
 * Author: David Brotz
 */

#include "../src/sys/RBTree.h"

#include <check.h>

int ISCallback(int* _One, int* _Two) {
	return (*_One) - (*_Two);
}

START_TEST(RBInsertTest) {
	struct RBTree _Tree = {NULL, 0, (RBCallback) ISCallback, (RBCallback) ISCallback};
	int _One = 1;

	RBInsert(&_Tree, &_One);
	ck_assert_int_eq(_Tree.Size, 1);
	ck_assert_ptr_ne(_Tree.Table, NULL);
}
END_TEST


START_TEST(RBDeleteTest) {
	struct RBTree _Tree = {NULL, 0, (RBCallback) ISCallback, (RBCallback) ISCallback};
	int _One = 1;

	RBInsert(&_Tree, &_One);
	RBDelete(&_Tree, &_One);
	ck_assert_int_eq(_Tree.Size, 0);
	ck_assert_ptr_eq(_Tree.Table, NULL);
}
END_TEST


START_TEST(RBDeleteManyTest) {
	struct RBTree _Tree = {NULL, 0, (RBCallback) ISCallback, (RBCallback) ISCallback};
	int _Table[100];

	for(int i = 0; i < 100; ++i) {
		_Table[i] = i;
		RBInsert(&_Tree, &_Table[i]);
	}
	ck_assert_int_eq(_Tree.Size, 100);
	RBDelete(&_Tree, &_Table[49]);
	ck_assert_int_eq(_Tree.Size, 99);
	ck_assert_ptr_eq(RBSearchNode(&_Tree, &_Table[49]), NULL);
	RBInsert(&_Tree, &_Table[49]);
	for(int i = 99; i >= 0; --i) {
		RBDelete(&_Tree, &_Table[i]);
		ck_assert_int_eq(_Tree.Size, i);
	}
}
END_TEST

START_TEST(RBRangeTest) {
	struct RBTree _Tree = {NULL, 0, (RBCallback) ISCallback, (RBCallback) ISCallback};
	int _Table[100];
	int _RangeResult[100];
	int _Min = 51;
	int _Max = 60;

	for(int i = 0; i < 100; ++i) {
		_Table[i] = i;
		RBInsert(&_Tree, &_Table[i]);
	}
	ck_assert_int_eq(RBRange(&_Tree, &_Min, &_Max, (void**)&_RangeResult, 100), _Max - _Min + 1); 
}
END_TEST

Suite* RedBlackSuite(void) {
	Suite* _Suite = suite_create("RB Tree");
	TCase* _Insert = tcase_create("Insert");
	TCase* _Delete = tcase_create("Delete");
	TCase* _Range = tcase_create("Range");

	tcase_add_test(_Insert, RBInsertTest);
	tcase_add_test(_Delete, RBDeleteTest);
	tcase_add_test(_Delete, RBDeleteManyTest);
	tcase_add_test(_Range, RBRangeTest);

	suite_add_tcase(_Suite, _Insert);
	suite_add_tcase(_Suite, _Delete);
	suite_add_tcase(_Suite, _Range);
	return _Suite;
}
