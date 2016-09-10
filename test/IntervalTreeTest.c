/**
 * Author: David Brotz
 * File: IntervalTreeTest.c
 */

#include "IntervalTreeTest.h"

#include "../src/sys/IntervalTree.h"
#include "../src/sys/LinkedList.h"

#include <check.h>

#include <stdlib.h>

struct TestStruct {
	int Min;
	int Max;
};

int MinFunc(const struct TestStruct* _Test) {
	return _Test->Min;
}

int MaxFunc(const struct TestStruct* _Test) {
	return _Test->Max;
}

START_TEST(ITreeConstructTest) {
	struct TestStruct _One = {0, 3};
	struct TestStruct _Two = {3, 6};
	struct TestStruct _Three = {6, 9};
	struct TestStruct _Four = {9, 12};
	struct TestStruct _Five = {12, 15};
	struct TestStruct* _Values[] = {
		&_One,
		&_Two,
		&_Three,
		&_Four,
		&_Five	
	};
	const struct IntervalTree* _Tree = ConstructITree((void**) _Values, 5, (ITreeGetVal) MinFunc, (ITreeGetVal) MaxFunc);
	DestroyITree(_Tree);
}
END_TEST

START_TEST(ITreeQuerySimpleTest) {
	struct TestStruct _One = {0, 3};
	struct TestStruct _Two = {3, 6};
	struct TestStruct _Three = {6, 9};
	struct TestStruct _Four = {9, 12};
	struct TestStruct _Five = {12, 15};
	struct TestStruct* _Values[] = {
		&_One,
		&_Two,
		&_Three,
		&_Four,
		&_Five	
	};
	const struct IntervalTree* _Tree = ConstructITree((void**) _Values, 5, (ITreeGetVal) MinFunc, (ITreeGetVal) MaxFunc);
	struct LinkedList _List;

	ConstructLinkedList(&_List);
	ITreeQuery(_Tree, 2, &_List);
	ck_assert_int_eq(1, _List.Size);
	DestroyITree(_Tree);
}
END_TEST

START_TEST(ITreeQueryComplexTest) {
	struct TestStruct _One = {0, 4};
	struct TestStruct _Two = {3, 6};
	struct TestStruct* _Values[] = {
		&_One,
		&_Two
	};
	const struct IntervalTree* _Tree = ConstructITree((void**) _Values, 2, (ITreeGetVal) MinFunc, (ITreeGetVal) MaxFunc);
	struct LinkedList _List;

	ConstructLinkedList(&_List);
	ITreeQuery(_Tree, 3, &_List);
	ck_assert_int_eq(2, _List.Size);
	LnkLstClear(&_List);
	ITreeQuery(_Tree, 6, &_List);
	ck_assert_int_eq(1, _List.Size);
	DestroyITree(_Tree);
}
END_TEST

START_TEST(ITreeQueryComplexMultTest) {
	struct TestStruct _One = {0, 8};
	struct TestStruct _Two = {3, 5};
	struct TestStruct _Three = {4, 11};
	struct TestStruct* _Values[] = {
		&_One,
		&_Two,
		&_Three
	};
	const struct IntervalTree* _Tree = ConstructITree((void**) _Values, 3, (ITreeGetVal) MinFunc, (ITreeGetVal) MaxFunc);
	struct LinkedList _List;

	ConstructLinkedList(&_List);
	ITreeQuery(_Tree, 3, &_List);
	ck_assert_int_eq(2, _List.Size);
	LnkLstClear(&_List);
	ITreeQuery(_Tree, 4, &_List);
	ck_assert_int_eq(3, _List.Size);
	LnkLstClear(&_List);
	ITreeQuery(_Tree, 11, &_List);
	ck_assert_int_eq(1, _List.Size);
	DestroyITree(_Tree);
}
END_TEST

Suite* IntervalTreeSuite(void) {
	Suite* _Suite = suite_create("Interval Tree");
	TCase* _Construct = tcase_create("Construction");

	tcase_add_test(_Construct, ITreeConstructTest);
	tcase_add_test(_Construct, ITreeQuerySimpleTest);
	tcase_add_test(_Construct, ITreeQueryComplexTest);
	tcase_add_test(_Construct, ITreeQueryComplexMultTest);

	suite_add_tcase(_Suite, _Construct);
	return _Suite;
}
