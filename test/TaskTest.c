/**
 * Author: David Brotz
 * File: TaskTest.c
 */

#include "TaskTest.h"

#include "../src/sys/TaskPool.h"

#include <check.h>

#include <sys/time.h>

double GetTime() {
	struct timeval _Time;

	gettimeofday(&_Time, NULL);
	return _Time.tv_sec + _Time.tv_usec * 1e-6;
}

struct TestTask {
	int TasksRun;
	int ParentLast;
	int Children;
};

int TestFunc(struct Task* _Task, struct TestTask* _Test) {
	++_Test->TasksRun;
	return 1;
}

int TestParent(struct Task* _Task, struct TestTask* _Test) {
	if(_Test->TasksRun == _Test->Children)
		_Test->ParentLast = 1;
	else 
		_Test->ParentLast = 0;
	return 1;
}

START_TEST(AddTaskTest) {
	struct TestTask _Test = {0, 0, 1};

	InitTaskPool();
	TaskPoolExecute(TaskPoolAdd(TASK_NOPARENT, (TaskFunc) TestFunc, &_Test, sizeof(&_Test)));
	
	RunTasks();
	ck_assert_int_eq(_Test.TasksRun, _Test.Children);	
	QuitTaskPool();
}
END_TEST

START_TEST(AddMultTaskTest) {
	struct TestTask _Test = {0, 0, 1000000};

	InitTaskPool();
	for(int i = 0; i < _Test.Children; ++i)
		TaskPoolExecute(TaskPoolAdd(TASK_NOPARENT, (TaskFunc) TestFunc, &_Test, sizeof(&_Test)));
	RunTasks();
	ck_assert_int_eq(_Test.TasksRun, _Test.Children);
	QuitTaskPool();
}
END_TEST

START_TEST(AddParentTaskTest) {
	struct TestTask _Test = {0, 0, 2};
	int  _Parent = 0;

	InitTaskPool();
	_Parent = TaskPoolAdd(TASK_NOPARENT, (TaskFunc) TestParent, &_Test, sizeof(&_Test));
	for(int i = 0; i < _Test.Children; ++i)
		TaskPoolExecute(TaskPoolAdd(_Parent, (TaskFunc) TestFunc, &_Test, sizeof(&_Test)));
	TaskPoolExecute(_Parent);
	RunTasks();
	ck_assert_int_eq(_Test.TasksRun, _Test.Children);
	ck_assert_int_eq(_Test.ParentLast, 1);
	QuitTaskPool();
}
END_TEST

START_TEST(AddMultParentTest) {
	struct TestTask _TestOne = {0, 0, 4};
	struct TestTask _TestTwo = {0, 0, 2};
	int _POne = 0;
	int _PTwo = 0;

	InitTaskPool();
	_POne = TaskPoolAdd(TASK_NOPARENT, (TaskFunc) TestParent, &_TestOne, sizeof(&_TestOne));
	_PTwo = TaskPoolAdd(TASK_NOPARENT, (TaskFunc) TestParent, &_TestTwo, sizeof(&_TestTwo));
	for(int i = 0; i < 2; ++i)
		TaskPoolExecute(TaskPoolAdd(_POne, (TaskFunc) TestFunc, &_TestOne, sizeof(&_TestOne)));
	for(int i = 0; i < 2; ++i)
		TaskPoolExecute(TaskPoolAdd(_PTwo, (TaskFunc) TestFunc, &_TestTwo, sizeof(&_TestTwo)));
	for(int i = 0; i < 2; ++i)
		TaskPoolExecute(TaskPoolAdd(_POne, (TaskFunc) TestFunc, &_TestOne, sizeof(&_TestOne)));
	TaskPoolExecute(_POne);
	TaskPoolExecute(_PTwo);
	RunTasks();
	ck_assert_int_eq(_TestOne.TasksRun, 4);
	ck_assert_int_eq(_TestOne.ParentLast, 1);
	ck_assert_int_eq(_TestTwo.TasksRun, 2);
	ck_assert_int_eq(_TestTwo.ParentLast, 1);
	QuitTaskPool();
}
END_TEST

Suite* TaskSuite(void) {
	Suite* _Suite = suite_create("Task");
	TCase* _Case = tcase_create("AddTask");
	TCase* _ParentCase = tcase_create("Parent");

	tcase_add_test(_Case, AddTaskTest);
	tcase_add_test(_Case, AddMultTaskTest);
	tcase_add_test(_ParentCase, AddParentTaskTest);
	tcase_add_test(_ParentCase, AddMultParentTest);
	suite_add_tcase(_Suite, _Case);
	suite_add_tcase(_Suite, _ParentCase);
	return _Suite;
}
