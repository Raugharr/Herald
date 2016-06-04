/**
 * Author: David Brotz
 * File: Policy.h
 */

#ifndef __POLICY_H
#define __POLICY_H

enum {
	POLCAT_MILITARY,
	POLCAT_ECONOMY,
	POLCAT_LAW
};

struct PolicyOption {
	const char* Name;
};

struct Policy {
	const char* Name;
	const char* Description;
	int Category;
	struct PolicyOption** Options;
	int OptionsSz;
};

struct Policy* CreatePolicy(const char* _Name, const char* _Description, int _Category);
void DestroyPolicy(struct Policy* _Policy);

#endif
