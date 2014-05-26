/*
 * Author: David Brotz
 * File: Person.h
 */

#ifndef __PERSON_H
#define __PERSON_H

#include "Herald.h"

#define EMALE (1)
#define EFEMALE (2)
#define AVKID (3)

//extern struct Array g_BoyNames;
//extern struct Array g_GirlNames;

struct Person {
	const char* Name;
	int Age; //In months.
	int Gender;
	int Nutrition;
	struct Family* Family;
	struct Family* Parent;
	struct Job* Ocupation;
};

struct Pregancy {
	int TTP;//Time to pregancy
	struct Person* Mother;
};

#define IsMarried(__Person) (__Person->Family->Wife != NULL)

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition);
struct Person* CreateChild(struct Family* _Family);
struct Pregancy* CreatePregancy(struct Person* _Person); 
void BirthCheck(struct Person* _Person);
void Birth();
void DeathCheck();
void Person_DeleteNames();
#endif

