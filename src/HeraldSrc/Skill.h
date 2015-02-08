/*
 * File: Skill.h
 * Author: David Brotz
 */
#ifndef __SKILL_H
#define __SKILL_H

typedef struct lua_State lua_State;

struct SkillType {
	int Id;
	char* Name;
};

struct SkillGroup {
	int Id;
	char* Name;
	struct SkillType** Children;
};

struct Skill {
	int Id;
	char* Name;
	int Xp;
};

struct SkillType* CreateSkillType(const char* _Name);
void DestroySkillGroup(struct SkillGroup* _Group);

struct SkillGroup* CreateSkillGroup(const char* _Name);
struct Skill* CreateSkill(const char* _Name);

struct SkillGroup* SkillGroupLoad(lua_State* _State, int _Index);
struct Skill* SkillLoad(lua_State* _State, int _Index);

#endif
