/*
 * File: CoroTest.c
 * Author: David Brotz
 */
#include "CoroTest.h"

#include <check.h>

#include "../src/sys/Coroutine.h"
#include "../src/sys/LuaCore.h"

void CoroFunc(int One, int Two, int Times) {
	int Result = 0;

	for(int i = 0; i < Times; ++i) {
		Result += One * Two;
	}
			
}

void CoroFuncYield(int One, int Two, int Times) {
	int Result = 0;

	for(int i = 0; i < Times; ++i) {
		Result += One * Two;
		CoYield();
	}
			
}

void CoroMain() {
	int Id = CoSpawn(CoroFunc, 3, 7, 15, 10);

	CoResume(Id);
	ck_assert_int_eq(CoStatus(Id), CO_DEAD);
	CoResume(Id);
	ck_assert_int_eq(CoStatus(Id), CO_DEAD);
}

START_TEST(CoroBaseTest) {
	InitLuaCore();
	CoSchedule(CoroMain);
}
END_TEST

Suite* CoroSuite(void) {
	Suite* Suite = suite_create("Coroutine");
	TCase* Case = tcase_create("Main");

	tcase_add_test(Case, CoroBaseTest);
	suite_add_tcase(Suite, Case);
	return Suite;
}
