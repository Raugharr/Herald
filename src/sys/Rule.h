/*
 * File: Rule.h
 * Author: David Brotz
 */
#ifndef __RULE_H
#define __RULE_H

typedef struct lua_State lua_State;

#define RuleEval(_Rule) g_RuleFuncLookup[(_Rule)->Type]((_Rule))

enum {
	PRIM_FLOAT,
	PRIM_INTEGER,
	PRIM_BOOLEAN,
	PRIM_PTR,
	PRIM_BYTE,
	PRIM_STRING
};

enum {
	RULE_NONE,
	RULE_BOOLEAN,
	RULE_GREATERTHAN,
	RULE_LESSTHAN,
	RULE_LUACALL,
	RULE_PRIMITIVE
};

union UPrimitive {
	float Float;
	int Int;
	void* Ptr;
	char Byte;
	char* String;
};

struct Primitive {
	int Type;
	union UPrimitive Value;
};

struct Rule {
	int Type;
	void (*Destroy)(struct Rule*);
};

struct RuleComparator {
	int Type;
	void (*Destroy)(struct Rule*);
	struct Rule* Left;
	struct Rule* Right;
};

struct RuleBoolean {
	int Type;
	void(*Destroy)(struct Rule*);
	struct Rule* Boolean;
};

struct RuleLuaCall {
	int Type;
	void (*Destroy)(struct Rule*);
	lua_State* State;
	char* FuncName;
	struct Primitive** Arguments;
};

struct RulePrimitive {
	int Type;
	void (*Destroy)(struct Rule*);
	struct Primitive Value;
};

struct Primitive* CreatePrimitive();
void DestroyPrimitive(struct Primitive* _Primitive);

struct Rule* CreateRule(int _Type, void(*_Destroy)(struct Rule*));
void DestroyRule(struct Rule* _Rule);

struct RulePrimitive* CreateRulePrimitive(struct Primitive* _Primitive);
void DestroyRulePrimitive(struct RulePrimitive* _Rule);

struct RuleComparator* CreateRuleComparator(int _Type, struct Rule* _Left, struct Rule* _Right);
void DestroyRuleComparator(struct RuleComparator* _Rule);

struct RuleBoolean* CreateRuleBoolean(struct Rule* _Boolean);
void DestroyRuleBoolean(struct RuleBoolean* _Rule);

struct RuleLuaCall* CreateRuleLuaCall(lua_State* _State, const char* _FuncName, struct Primitive** _Arguments);
void DestroyRuleLuaCall(struct RuleLuaCall* _Rule);

void PrimitiveLuaPush(lua_State* _State, struct Primitive* _Primitive);
struct Primitive* LuaToPrimitive(lua_State* _State, int _Index);

int LuaRuleLuaCall(lua_State* _State);
int LuaRuleGreaterThan(lua_State* _State);
int LuaRuleLessThan(lua_State* _State);
int LuaRuleIsTrue(lua_State* _State);

struct Rule* LuaValueToRule(lua_State* _State, int _Index);

int RuleTrue(const struct Rule* _Rule);
int RuleFalse(const struct Rule* _Rule);
int RuleGreaterThan(const struct RuleComparator* _Rule);
int RuleLessThan(const struct RuleComparator* _Rule);
int RuleLuaCall(const struct RuleLuaCall* _Rule);

extern int(*g_RuleFuncLookup[])(const struct Rule*);

#endif /* __RULE_H */
