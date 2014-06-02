/*
 * File: Population.c
 * Author: David Brotz
 */

#include "Population.h"

#include <stdlib.h>

struct Population* CreatePopulation(const char* _Name, int _AdultFood, int _ChildFood, int _AdultAge) {
	struct Population* _Population = (struct Population*) malloc(sizeof(struct Population));

	_Population->Name = _Name;
	_Population->AdultFood = _AdultFood;
	_Population->ChildFood = _ChildFood;
	_Population->AdultAge = _AdultAge;
	return _Population;
}

struct Population* CopyPopulation(const struct Population* _Population, int _Quantity) {
	struct Population* _NewPopulation = (struct Population*) malloc(sizeof(struct Population));

	_NewPopulation->Name = _Population->Name;
	_NewPopulation->AdultFood = _Population->AdultFood;
	_NewPopulation->ChildFood = _Population->AdultFood;
	_NewPopulation->AdultAge = _Population->AdultFood;
	_NewPopulation->Quantity = _Population->AdultFood;
	return _NewPopulation;
}

void DestroyPopulation(struct Population* _Population) {
	free(_Population);
}
