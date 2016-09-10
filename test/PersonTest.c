/**
 * Author: David Brotz
 * File: PersonTest.c
 */

START_TEST(CreatePersonInvalidTest) {
	struct Person* _Person = CreatePerson(NULL, -1, EMALE, -1, -1 ,-1, NULL);
	
	ck_assert_int_eq(_Person, NULL);	 
}
END_TEST

