#include "GoapTest.h"

#include <check.h>

START_TEST(ArmyTestRaid) {

}
END_TEST

Suite* ArmySuite(void) {
	Suite* Suite = suite_create("Army");
	TCase* Raid = tcase_create("Raid");

	tcase_add_test(Raid, ArmyTestRaid);
	suite_add_tcase(Suite, Raid);
	return _Suite;
}

