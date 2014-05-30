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
#define MAX_NUTRITION (2000)
#define IsMarried(__Person) (__Person->Family->Wife != NULL)

struct Person {
	const char* Name;
	int Age; //In months.
	int Gender;
	int Nutrition;
	struct Family* Family;
	struct Family* Parent;
};

struct Pregancy {
	int TTP;//Time to pregancy
	struct Person* Mother;
};

void Person_Init();
void Person_Quit();
struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition);
struct Person* CreateChild(struct Family* _Family);
struct Pregancy* CreatePregancy(struct Person* _Person); 
void Person_Update(struct Person* _Person, int _NutVal);
void Birth();
#endif

