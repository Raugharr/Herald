/*
 * Author: David Brotz
 * File: main.c
 */

#include "GoapTest.h"
#include "DataStruct.h"
#include "RedBlackTest.h"
#include "IntervalTreeTest.h"
#include "MathTest.h"
#include "CoroTest.h"
#include "GameHarness.h"

#include "../src/sys/LuaCore.h"
#include "../src/Date.h"

#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <stdlib.h>
#include <check.h>


START_TEST(DateAddIntBaseTest) {
	DATE _Date = 0;

	_Date = DateAddInt(_Date, DAYS_EVEN);
	ck_assert_int_eq(DAY(DAYS_EVEN), DAYS_EVEN);
	ck_assert_int_eq(DateToDays(_Date), DAYS_EVEN);
	_Date = DateAddInt(_Date, 1);
	ck_assert_int_eq(DAY(_Date), 1);
	ck_assert_int_eq(DateToDays(_Date), DAYS_EVEN + 1);
	ck_assert_int_eq(MONTH(_Date), 1);
}
END_TEST

START_TEST(DateAddIntFebTest) {
	DATE _Date = DateAddInt(0, DAYS_EVEN + DAYS_LEAP);

	ck_assert_int_eq(DAY(_Date), 0);
	ck_assert_int_eq(MONTH(_Date), 2);
	ck_assert_int_eq(DateToDays(_Date), DAYS_EVEN + DAYS_LEAP);
}
END_TEST

START_TEST(LuaCopyTableTest) {
	lua_State* _State = luaL_newstate();
	
	lua_newtable(_State);
	lua_pushstring(_State, "Width");
	lua_pushinteger(_State, 500);
	lua_rawset(_State, 1);

	lua_pushstring(_State, "Height");
	lua_pushinteger(_State, 100);
	lua_rawset(_State, 1);

	lua_newtable(_State);
	lua_pushinteger(_State, 1);
	lua_pushinteger(_State, 10);
	lua_rawset(_State, 2);

	lua_pushstring(_State, "foo");
	lua_pushstring(_State, "bar");
	lua_rawset(_State, 2);

	ck_assert_int_eq(lua_gettop(_State), 2);
	LuaCopyTable(_State, 1);
	
	lua_rawgeti(_State, -1, 1);
	ck_assert_int_eq(lua_tointeger(_State, -1), 10);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Width");
	lua_rawget(_State, -2);
	ck_assert_int_eq(lua_tointeger(_State, -1), 500);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Height");
	lua_rawget(_State, -2);
	ck_assert_int_eq(lua_tointeger(_State, -1), 100);
	lua_pop(_State, 1);

	lua_close(_State);
}
END_TEST

Suite* CoreSuite(void) {
	Suite* _Suite = suite_create("System");
	TCase* _Lua = tcase_create("Lua");
	TCase* _Date = tcase_create("Date");

	tcase_add_test(_Lua, LuaCopyTableTest);
	suite_add_tcase(_Suite, _Lua);

	tcase_add_test(_Date, DateAddIntBaseTest);
	tcase_add_test(_Date, DateAddIntFebTest);
	suite_add_tcase(_Suite, _Date);
	return _Suite;
}

Suite* LuaSuite(void) {
	Suite* _Suite = suite_create("Lua");
	TCase* _Case = tcase_create("Core");

	tcase_add_test(_Case, LuaCopyTableTest);
	suite_add_tcase(_Suite, _Case);
	return _Suite;
}

int main(void) {
	Suite* _Suite = CoreSuite();
	SRunner* _Runner = NULL;	
	int _FailedCt = 0;

	_Runner = srunner_create(_Suite);
	srunner_set_fork_status(_Runner, CK_NOFORK);
	srunner_add_suite(_Runner, DataStructureSuite());
	srunner_add_suite(_Runner, GoapSuite());
	srunner_add_suite(_Runner, RedBlackSuite());
	srunner_add_suite(_Runner, IntervalTreeSuite());
	srunner_add_suite(_Runner, MathSuite());
	srunner_add_suite(_Runner, CoroSuite());
	srunner_run_all(_Runner, CK_NORMAL);
	_FailedCt = srunner_ntests_failed(_Runner);
	srunner_free(_Runner);
	GameHarness(0, NULL);
	return (_FailedCt == 0) ? (EXIT_SUCCESS) : (EXIT_FAILURE);
}
