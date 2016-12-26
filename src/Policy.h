/**
 * Author: David Brotz
 * File: Policy.h
 */

#ifndef __POLICY_H
#define __POLICY_H

#include "Family.h"

#define POLICY_SUBSZ (4)
#define POLICY_OPTIONS (5)
#define POLICY_MAXOPTIONS (POLICY_SUBSZ * POLICY_OPTIONS)
#define POLICYACT_IGNORE (-1)
#define POLICYCAT_UNUSED (-1)

#define ValidPolicyCategory(_Cat) ((_Cat) >= 0 && (_Cat) < POLICY_SUBSZ)

struct Government;

typedef void (*PolicyOptFunc)(struct Government*);
typedef int (*PolicyOptUtility)(struct Government*, int*, int*);//First int* is the min utility, second int* is the max utility. Return is the expected utility.

enum {
	POLCAT_MILITARY,
	POLCAT_ECONOMY,
	POLCAT_LAW,
};

enum {
	POLICYMOD_MINOR = 5,
	POLICYMOD_NORMAL = 10,
	POLICYMOD_GREAT = 15
};

enum {
	POLPREF_HATE = -3,
	POLPREF_DISLIKE,
	POLPREF_UNFAVORED,
	POLPREF_NETURAL,
	POLPREF_FAVORED,
	POLPREF_LIKE,
	POLPREF_LOVE
};

/**
 * \brief A single law for a policy that can be selected.
 * Conains the name of the policy, what governments can pass it, which castes like
 * the policy as well as what happens when its enacted.
 */
struct PolicyOption {
	const char* Name;
	const char* Desc;
	uint32_t GovsAllowed;
	int8_t CastePreference[CASTE_SIZE];
	PolicyOptFunc OnPass;
	PolicyOptFunc OnRemove;
	PolicyOptUtility Utility;
};

/**
 * \brief A policy is a grouping of laws that a government and enact. Each Policy
 * can have several different laws and different options for that law to have.
 */
struct Policy {
	const char* Name;
	const char* Description;
	struct {
		char* Name[POLICY_SUBSZ];
		int8_t Size[POLICY_SUBSZ];
		struct PolicyOption Options[POLICY_MAXOPTIONS];
	} Options;
	uint16_t Id;
	uint32_t GovsAllowed;
	uint8_t OptionsSz;
	uint8_t Category;
};

struct ActivePolicy {
	const struct Policy* Policy;
	int8_t OptionSel[POLICY_SUBSZ];
};

void CtorPolicy(struct Policy* _Policy, const char* _Name, const char* _Description, int _Category);
void DestroyPolicy(struct Policy* _Policy);

void PolicyAddOption(struct Policy* _Policy, int _Row,  const char* _Name, const char* _Desc, PolicyOptFunc _CallFunc, PolicyOptUtility _Utility);
void PolicyAddCategory(struct Policy* _Policy, const char* _Name);
/**
 * \brief Returns a pointer to the first element in _Row.
 * \return The index that contains the first policy of row _Row.
 */
const struct PolicyOption* PolicyRow(const struct Policy* _Policy, int _Row, int _Col);
void DestroyPolicyOption(struct PolicyOption* _Opt);
/**
 * \brief Returns a pointer to the policy option being changed by the active policy or
 * NULL if no policy is being changed.
 */
const struct PolicyOption* PolicyChange(const struct ActivePolicy* _Policy);
int LuaPolicyLoad(lua_State* State);

#endif
