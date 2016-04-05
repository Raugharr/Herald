#include "GoapTest.h"

#include <check.h>

#include "../src/AI/Setup.h"

enum {
	ATOM_ONE,
	ATOM_TWO,
	ATOM_THREE,
	ATOM_SIZE
};

const char* g_TestAtoms[] = {
	"AtomOne",
	"AtomTwo",
	"AtomThree"
};

//int AgentActionTestCost(const struct Agent* _Agent) {
//	return 1;
//}

//int AgentActionTestAction(struct BigGuy* _Guy) {
//	return 1;
//`}

//void AgentActionTest(struct GOAPPlanner* _GoPlan, struct AgentUtility* _AgentPlan) {
//	GoapSetActionCost(_GoPlan, "AtomOne", AgentActionTestCost);
//}


START_TEST(GoapTest) {
	struct GOAPPlanner _Goap;	

	GoapClear(&_Goap);
	for(int i = 0; i < ATOM_SIZE; ++i) {
		GoapAddAtom(&_Goap, g_TestAtoms[i]);
	}
	//GoapSetActionCost(&_Goap, "ActionOne", AgentActionTestCost);
	//ck_assert_int_eq(_Goap.ActionCt, 1);
	//ck_assert_ptr_eq(_Goap.ActionCosts[_Goap.ActionCt], AgentActionTestCost);
}
END_TEST

START_TEST(GoapAddAtomTest) {
	struct GOAPPlanner _Goap;	

	GoapClear(&_Goap);
	for(int i = 0; i < ATOM_SIZE; ++i) {
		GoapAddAtom(&_Goap, g_TestAtoms[i]);
	}
	ck_assert_ptr_eq(_Goap.AtomNames[0], g_TestAtoms[0]);
	ck_assert_ptr_eq(_Goap.AtomNames[1], g_TestAtoms[1]);
	ck_assert_ptr_eq(_Goap.AtomNames[2], g_TestAtoms[2]);
	for(int i = 3; i < GOAP_ATOMS; ++i)
		ck_assert_ptr_eq(_Goap.AtomNames[i], NULL);
}
END_TEST

START_TEST(GoapClearTest) {
	struct GOAPPlanner _Goap;

	GoapClear(&_Goap);
	ck_assert_int_eq(_Goap.ActionCt, 0);
	ck_assert_int_eq(_Goap.GoalCt, 0);
	ck_assert_int_eq(_Goap.AtomCt, 0);
	for(int i = 0; i < GOAP_ATOMS; ++i)
		ck_assert_ptr_eq(_Goap.AtomNames[i], NULL);
}
END_TEST

START_TEST(GoapActionClearTest) {
	struct GoapAction _Action;

	GoapActionClear(&_Action);
	ck_assert_ptr_eq(_Action.Name, NULL);
	ck_assert_ptr_eq(_Action.Action, NULL);
	ck_assert_ptr_eq(_Action.Utility, NULL);
	ck_assert_int_eq(_Action.UtilityFunction, 0);
	ck_assert_ptr_eq(_Action.IsComplete, NULL);

}
END_TEST

Suite* GoapSuite(void) {
	Suite* _Suite = suite_create("AI");
	TCase* _Case = tcase_create("GOAP");

	tcase_add_test(_Case, GoapClearTest);
	tcase_add_test(_Case, GoapTest);
	tcase_add_test(_Case, GoapAddAtomTest);
	tcase_add_test(_Case, GoapActionClearTest);
	suite_add_tcase(_Suite, _Case);
	return _Suite;
}
