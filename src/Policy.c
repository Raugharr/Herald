/**
 * Author: David Brotz
 * File: Policy.h
 */

#include "Policy.h"

#include <stdlib.h>
#include <string.h>

struct Policy* CreatePolicy(const char* _Name, const char* _Description, int _Category) {
	struct Policy* _Policy = malloc(sizeof(struct Policy));

	_Policy->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Policy->Description = calloc(strlen(_Description) + 1, sizeof(char));
	_Policy->Category = _Category;
	_Policy->Options = NULL;
	_Policy->OptionsSz = 0;
	strcpy((char*)_Policy->Name, _Name);
	strcpy((char*)_Policy->Description, _Description);
	return _Policy;
}

void DestroyPolicy(struct Policy* _Policy) {
	free((char*) _Policy->Name);
	free((char*) _Policy->Description);
	free(_Policy);
}
