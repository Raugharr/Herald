/*
 * File: SettlementTest.c
 * Author: David Brotz
 */

#include "SettlementTest.h"

#include <check.h>

START_TEST(ArmyTestRaid) {
	struct Settlement* Settlement = CreateSettlement(0, 0, 0);

}
END_TEST

Suite* SettlementTest(void) {
	Suite* Suite = suite_create("Settlement");
	TCase* Raid = tcase_create("Raid");

	tcase_add_test(Raid, ArmyTestRaid);
	suite_add_tcase(Suite, Raid);
	return _Suite;
}
