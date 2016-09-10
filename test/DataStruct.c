/* 
 * Author: David Brotz
 * File: DataStruct.h
 */

#include "DataStruct.h"

#include <check.h>

#include "../src/sys/StackAllocator.h"
#include "../src/sys/LinkedList.h"
#include "../src/sys/HashTable.h"

#include <stdlib.h>

START_TEST(StackAllocInitTest) {
	struct LifoAllocator _Allocator;

	InitLifoAlloc(&_Allocator, 512);
	ck_assert_ptr_ne(_Allocator.ArenaBot, 0);
	ck_assert_ptr_eq(_Allocator.ArenaTop, _Allocator.ArenaBot);
	ck_assert_int_eq(_Allocator.ArenaSize, 512);
}
END_TEST

START_TEST(StackAllocInsertTest) {
	struct LifoAllocator _Allocator;
	int* _TestInt = NULL;

	InitLifoAlloc(&_Allocator, 512);
	_TestInt = LifoAlloc(&_Allocator, sizeof(int));
	*_TestInt = 0x10;
	ck_assert_int_eq(*((int*)_Allocator.ArenaBot), *_TestInt);
	ck_assert_ptr_eq(((int*)_Allocator.ArenaBot), _TestInt);
	_TestInt = LifoAlloc(&_Allocator, sizeof(int));
	*_TestInt = 0x20;
	ck_assert_ptr_eq((int*)_Allocator.ArenaTop - (sizeof(int) / sizeof(int)), _TestInt);

}
END_TEST

START_TEST(StackAllocRemoveTest) {
	struct LifoAllocator _Allocator;

	InitLifoAlloc(&_Allocator, 512);
	LifoAlloc(&_Allocator, 12);
	LifoFree(&_Allocator, 12);
	ck_assert_int_eq(_Allocator.ArenaTop, _Allocator.ArenaBot);
}
END_TEST

START_TEST(LinkedListPushBackTest) {
	struct LinkedList _List =  {0};
	int _Tbl[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

	LnkLstPushBack(&_List, &_Tbl[0]);
	ck_assert_ptr_eq(_List.Front->Data, &_Tbl[0]);
	ck_assert_ptr_eq(_List.Back->Data, &_Tbl[0]);
	LnkLstPushBack(&_List, &_Tbl[1]);
	ck_assert_ptr_eq(_List.Front->Data, &_Tbl[0]);
	ck_assert_ptr_eq(_List.Back->Data, &_Tbl[1]);
	LnkLstPushBack(&_List, &_Tbl[2]);
	ck_assert_ptr_eq(_List.Front->Data, &_Tbl[0]);
	ck_assert_ptr_eq(_List.Back->Data, &_Tbl[2]);
}
END_TEST

START_TEST(LinkedListPushFrontTest) {
	struct LinkedList _List = {0};
	int _Tbl[] = {0, 1, 2};

	LnkLstPushFront(&_List, &_Tbl[0]);
	ck_assert_ptr_eq(_List.Front->Data, &_Tbl[0]);
	ck_assert_ptr_eq(_List.Back->Data, &_Tbl[0]);
	LnkLstPushFront(&_List, &_Tbl[1]);
	ck_assert_ptr_eq(_List.Front->Data, &_Tbl[1]);
	ck_assert_ptr_eq(_List.Back->Data, &_Tbl[0]);
}
END_TEST

START_TEST(LinkedListPopFrontTest) {
	struct LinkedList _List = {0};
	int _Tbl[] = {0, 1, 2};

	LnkLstPushFront(&_List, &_Tbl[0]);
	ck_assert_ptr_eq(LnkLstPopFront(&_List), &_Tbl[0]);
	ck_assert_int_eq(_List.Size, 0);
	ck_assert_ptr_eq(_List.Front, NULL);
	ck_assert_ptr_eq(_List.Back, NULL);
	LnkLstPushBack(&_List, &_Tbl[1]);
	LnkLstPushBack(&_List, &_Tbl[2]);
	ck_assert_ptr_eq(LnkLstPopFront(&_List), &_Tbl[1]);
	ck_assert_ptr_eq(LnkLstPopFront(&_List), &_Tbl[2]);
	ck_assert_int_eq(_List.Size, 0);
	ck_assert_ptr_eq(_List.Front, NULL);
	ck_assert_ptr_eq(_List.Back, NULL);
}
END_TEST

START_TEST(LinkedListPopBackTest) {
	struct LinkedList _List = {0};
	int _Tbl[] = {0, 1, 2};

	LnkLstPushBack(&_List, &_Tbl[0]);
	ck_assert_ptr_eq(LnkLstPopBack(&_List), &_Tbl[0]);
	ck_assert_int_eq(_List.Size, 0);
	ck_assert_ptr_eq(_List.Front, NULL);
	ck_assert_ptr_eq(_List.Back, NULL);
	LnkLstPushBack(&_List, &_Tbl[1]);
	LnkLstPushBack(&_List, &_Tbl[2]);
	ck_assert_ptr_eq(LnkLstPopBack(&_List), &_Tbl[2]);
	ck_assert_ptr_eq(LnkLstPopBack(&_List), &_Tbl[1]);
	ck_assert_int_eq(_List.Size, 0);
	ck_assert_ptr_eq(_List.Front, NULL);
	ck_assert_ptr_eq(_List.Back, NULL);
}
END_TEST

START_TEST(HashTableInsertTest) {
	struct HashTable _Table = {NULL, 0, 0};
	int _Data = 5;

	_Table.Table = calloc(sizeof(struct HashNode*), 20);
	//Should not add a element add not crash the test.
	HashInsert(&_Table, "Hello", &_Data);
	_Table.TblSize = 20;
	HashInsert(&_Table, "Hello", &_Data);
	ck_assert_ptr_eq(HashSearch(&_Table, "Hello"), &_Data);
	HashInsert(&_Table, NULL, &_Data);
	ck_assert_int_eq(_Table.Size, 1);
}
END_TEST

START_TEST(HashTableDeleteTest) {
	struct HashTable _Table = {NULL, 0, 0};
	int _Data = 5;

	_Table.Table = calloc(sizeof(struct HashNode*), 20);
	_Table.TblSize = 20;
	_Table.Size = 0;

	ck_assert_int_eq(HashDelete(&_Table, "Foobar"), 0);
	HashInsert(&_Table, "Foobar", &_Data);
	ck_assert_int_eq(HashDelete(&_Table, "Foobar"), 1);
	ck_assert_int_eq(HashDelete(&_Table, NULL), 0);
	ck_assert_int_eq(_Table.Size, 0);
}
END_TEST

Suite* DataStructureSuite(void) {
	Suite* _Suite = suite_create("Data Structures");
	TCase* _LinkedList = tcase_create("LinkedList");
	TCase* _HashTable = tcase_create("HashTable");
	TCase* _LifoAlloc = tcase_create("LifoAllocator");

	tcase_add_test(_LinkedList, LinkedListPushBackTest);
	tcase_add_test(_LinkedList, LinkedListPushFrontTest);
	tcase_add_test(_LinkedList, LinkedListPopFrontTest);
	tcase_add_test(_LinkedList, LinkedListPopBackTest);

	tcase_add_test(_HashTable, HashTableInsertTest);
	tcase_add_test(_HashTable, HashTableDeleteTest);

	tcase_add_test(_LifoAlloc, StackAllocInitTest);
	tcase_add_test(_LifoAlloc, StackAllocInsertTest);
	tcase_add_test(_LifoAlloc, StackAllocRemoveTest);

	suite_add_tcase(_Suite, _LinkedList);
	suite_add_tcase(_Suite, _HashTable);
	suite_add_tcase(_Suite, _LifoAlloc);
	return _Suite;
}
