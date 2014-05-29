/*
 * File: Manor.h
 * Author: David Brotz
 */

#ifndef __MANOR_H
#define __MANOR_H

#include "Population.h"
#include "sys/LinkedList.h"
#include "sys/HashTable.h"

struct Building;

struct Manor {
	char* Name;
	int Population;
	int Acres;
	int FreeAcres;
	int Treasury;
	int Income; //Last year's profits.
	struct Population PopCenter;
	struct LinkedList Families;
	/*
	 * Note: m_Goods, m_Crops, m_Animals, and m_Production might no longer be needed as these should be handled by each household.
	 * Note: m_Production might be needed to keep a priority list so when goods are produced prerequiste goods are made first.
	 */
	struct HashTable Goods;//Key is name of Good, pair is struct Good*
	struct LinkedList Crops;//Key is name of crop, pair is struct Crop*
	struct LinkedList Animals;//Key is name of Population, pair is struct Population*
	/* @brief The HashTable's key is the name of the Good that the Buildings in the
	 * LinkedList<Building*> can produce.
	 */
	struct HashTable Production; //Key is output good of Building*, pair is struct LinkedList* of struct Building*
};

struct Manor* CreateManor(const char* _Name, int _Population);
void DestroyManor(struct Manor* _Manor);
int AddBuilding(struct Manor* _Manor, const struct Building* _Building);

#endif
