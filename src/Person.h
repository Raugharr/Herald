/*
 * Author: David Brotz
 * File: Person.h
 */

#ifndef __PERSON_H
#define __PERSON_H

#include "Herald.h"

#include "World.h"

#define EMALE (1)
#define EFEMALE (2)
#define AVKID (3)
#define MAX_NUTRITION (2000)
#define IsMarried(__Person) (__Person->Family->Wife != NULL)
#define PersonMature(__Person) (TO_YEARS(__Person->Age) > 13)
#define PersonDead(__Person) (__Person->Nutrition == 0)

struct Person {
	int Id;
	int Gender;
	int Nutrition;
	DATE Age;
	const char* Name;
	struct Family* Family;
	struct Family* Parent;
	struct Occupation* Occupation;
};

struct Pregancy {
	int TTP;//Time to pregancy
	struct Person* Mother;
};

void PersonInit();
void PersonQuit();
struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition);
void DestroyPerson(struct Person* _Person);
struct Person* CreateChild(struct Family* _Family);
struct Pregancy* CreatePregancy(struct Person* _Person); 
void DestroyPregancy(struct Pregancy* _Pregancy);
int PersonUpdate(struct Person* _Person);
void PersonWork(struct Person* _Person);
void PersonDeath(struct Person* _Person);
int PregancyUpdate();
int PersonWorkMult(struct Person* _Person);
#endif

