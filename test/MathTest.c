/*
 * File: MathTest.c
 * Author: David Brotz
 */

#include "MathTest.h"

#include "../src/sys/Math.h"

#include <check.h>
#include <stdio.h>

#define RAND_CT (1000000)

START_TEST(MathRandTest) {
	int _Table[100] = {0};
	
	for(int i = 0; i < RAND_CT; ++i)
		_Table[Random(0, 9999999) / 100000]++;
	for(int i = 0; i < RAND_CT / 10000; ++i) {
		ck_assert_int_le((RAND_CT / 100 * .96), _Table[i]);
	}
}
END_TEST

Suite* MathSuite(void) {
	Suite* _Suite = suite_create("Math");
	TCase* _Case = tcase_create("Rand");

	tcase_add_test(_Case, MathRandTest);
	suite_add_tcase(_Suite, _Case);
	return _Suite;
}
