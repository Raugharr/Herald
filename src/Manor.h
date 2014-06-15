/*
 * File: Manor.h
 * Author: David Brotz
 */

#ifndef __MANOR_H
#define __MANOR_H

#include "Population.h"
#include "sys/LinkedList.h"
#include "sys/HashTable.h"

#define ManorPop(__Manor) (__Manor.People.Size)

struct Building;

struct Manor {
	char* Name;
	int Acres;
	int FreeAcres;
	struct LinkedList People;
	struct HashTable Goods;//Key is name of Good, pair is struct Good*
	struct LinkedList Crops;//Contains struct Field*.
	struct LinkedList Animals;//Key is name of Population, pair is struct Population*
	/* @brief The HashTable's key is the name of the Good that the Buildings in the
	 * LinkedList<Building*> can produce.
	 */
	struct HashTable Production; //Key is output good of Building*, pair is struct LinkedList* of struct Building*
};

struct Manor* CreateManor(const char* _Name, int _Population);
void DestroyManor(struct Manor* _Manor);
int AddBulding(struct Manor* _Manor, const struct Building* _Building);
int ManorUpdate(struct Manor* _Manor);

#endif
