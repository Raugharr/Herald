/*
 * File: Rule.h
 * Author: David Brotz
 */
#ifndef __RULE_H
#define __RULE_H

#include <stdint.h>

struct Rule;

typedef struct lua_State lua_State;
typedef int(*RuleFunc)(const struct Rule*, lua_State*); 
typedef void(*RuleDestroy)(struct Rule*);

#define RuleEval(_Rule) LuaRuleEval((_Rule), g_LuaState)
#define LuaRuleEval(_Rule, _State) g_RuleFuncLookup[(_Rule)->Type]((_Rule), _State)

/**
 * TODO: There are several rule types that are very common such as RuleBoolean(0) and RuleBoolean(1)
 * used to represent true and false respectivly. It should be looked into to determine if caching common
 * rules would be able to save a significant amount of memory.
 * Simple rules like greaterthan, lessthan, and ifthenelse should be all put into one function that chooses
 * the correct rule through a switch to remove function overhead.
 */

enum {
	PRIM_FLOAT,
	PRIM_INTEGER,
	PRIM_BOOLEAN,
	PRIM_BYTE,
	PRIM_STRING,
	PRIM_PTR
};

enum {
	RULE_NONE,
	RULE_BOOLEAN,
	RULE_GREATERTHAN,
	RULE_LESSTHAN,
	RULE_LUACALL,
	RULE_PRIMITIVE,
	RULE_IFTHENELSE,
	RULE_EVENT,
	RULE_BLOCK,
	RULE_LUAOBJ,
	RULE_NEGATE,
	RULE_COND
};

union UPrimitive {
	float Float;
	int32_t Int;
	void* Ptr;
	char Byte;
	char* String;
};

struct Primitive {
	uint16_t Type;
	uint16_t Class;//Class of pointer should use LOBJ_* as values.
	union UPrimitive Value;
};

struct Rule {
	int Type;
	void (*Destroy)(struct Rule*);
};

struct RuleDecorator {
	int Type;
	void (*Destroy)(struct Rule*);
	struct RuleLuaCall* Rule;
};

struct RuleComparator {
	int Type;
	void (*Destroy)(struct Rule*);
	struct Rule* Left;
	struct Rule* Right;
};

struct RuleIfThenElse {
	int Type;
	void(*Destroy)(struct Rule*);
	struct RuleComparator* Comparator;
	struct Rule* OnTrue;
	struct Rule* OnFalse;
};

struct RuleBoolean {
	int Type;
	void(*Destroy)(struct Rule*);
	int Boolean;
};

struct RuleLuaCall {
	int Type;
	void (*Destroy)(struct Rule*);
	int TblRef;
	lua_State* State;
};

struct RulePrimitive {
	int Type;
	void (*Destroy)(struct Rule*);
	struct Primitive Value;
};

struct RuleEvent {
	int Type;
	void (*Destroy)(struct Rule*);
	int (*Compare)(const void*, const void*);
	int Event;
};

struct RuleBlock {
	int Type;
	void (*Destroy)(struct Rule*);
	struct Rule** RuleList;
	int ListSz;
};

struct RuleCond {
	int Type;
	void (*Destroy)(struct Rule*);
	struct RuleBoolean** Conditions;
	struct Rule** Actions;
	int ListSz;
};

struct RuleLuaObj {
	int Type;
	void (*Destroy)(struct Rule*);
	void* Object;
	const char* Class;
};

struct Primitive* CreatePrimitive();
void DestroyPrimitive(struct Primitive* _Primitive);

void PrimitiveSetInt(struct Primitive* _Primitive, int _Int);
void PrimitiveSetFloat(struct Primitive* _Primitive, float _Float);
void PrimitiveSetPtr(struct Primitive* _Primitive, void* _Ptr, uint16_t Class);
void PrimitiveSetString(struct Primitive* _Primitive, const char* _Str);

int PrimitiveToBoolean(struct Primitive* _Primitive);
void PrimitiveToStr(const struct Primitive* Prim, char* restrict  Buffer, uint32_t BufSz);
void PrimitivePrint(const struct Primitive* _Primitive);

struct Rule* CreateRule(int _Type, void(*_Destroy)(struct Rule*));
void DestroyRule(struct Rule* _Rule);

int RuleCmp(const void* _One, const void* _Two);

struct RulePrimitive* CreateRulePrimitive(struct Primitive* _Primitive);
void DestroyRulePrimitive(struct RulePrimitive* _Rule);

struct RuleDecorator* CreateRuleDecorator(int _Type, struct RuleLuaCall* _RuleDec);
void DestroyRuleDecorator(struct RuleDecorator* _Rule);

struct RuleComparator* CreateRuleComparator(int _Type, struct Rule* _Left, struct Rule* _Right);
void DestroyRuleComparator(struct RuleComparator* _Rule);

struct RuleIfThenElse* CreateRuleIfThenElse(struct RuleComparator* _Comparator, struct Rule* _OnTrue, struct Rule* _OnFalse);
void DestroyRuleIfThenElse(struct RuleIfThenElse* _Rule);

struct RuleBoolean* CreateRuleBoolean(int _Boolean);
void DestroyRuleBoolean(struct RuleBoolean* _Rule);

struct RuleLuaCall* CreateRuleLuaCall(lua_State* _State, int _TableRef);
void DestroyRuleLuaCall(struct RuleLuaCall* _Rule);

struct RuleEvent* CreateRuleEvent(int _Event);
void DestroyRuleEvent(struct RuleEvent* _Rule);

struct RuleBlock* CreateRuleBlock(int _Size);
void DestroyRuleBlock(struct RuleBlock* _Rule);

struct RuleCond* CreateRuleCond(int _Conditions);
void DestroyRuleCond(struct RuleCond* _Rule);

struct RuleLuaObj* CreateRuleLuaObj(void* _Object, const char* _Class);
void DestroyRuleLuaObj(struct RuleLuaObj* _Obj);

int RuleTrue(const struct Rule* _Rule, lua_State* _State);
int RuleFalse(const struct Rule* _Rule, lua_State* _State);
int RuleGreaterThan(const struct RuleComparator* _Rule, lua_State* _State);
int RuleLessThan(const struct RuleComparator* _Rule, lua_State* _State);

int RuleLuaCall(const struct RuleLuaCall* _Rule, lua_State* _State);
int RulePrimitive(const struct RulePrimitive* _Primitive, lua_State* _State);
int RuleIfThenElse(const struct RuleIfThenElse* _Rule, lua_State* _State);
int RuleBoolean(const struct RuleBoolean* _Rule, lua_State* _State);
int RuleBlock(const struct RuleBlock* _Block, lua_State* _State);
int RuleLuaObject(const struct RuleLuaObj* _Obj, lua_State* _State);
int RuleNegate(const struct RuleDecorator* _Rule, lua_State* _State);

int RuleCond(const struct RuleCond* _Obj, lua_State* _State);

int RuleEventCompare(const struct Rule* _One, const struct Rule* _Two);

extern RuleFunc g_RuleFuncLookup[];

#endif /* __RULE_H */
