/*
 * File: Skill.c
 * Author: David Brotz
 */

#include "Skill.h"

#include "Herald.h"

struct SkillType* CreateSkillType(const char* _Name) {
	struct SkillType* _Type = (struct SkillType*) malloc(sizeof(struct SkillType));

	_Type->Id = NextId();
	_Type->Name = calloc(strlen(_Name) + 1, sizeof(char));
	strcpy(_Type->Name, _Name);
}

void DestroySkillGroup(struct SkillGroup* _Group) {
	free(_Group->Name);
	free(_Group);
}

struct SkillGroup* CreateSkillGroup(const char* _Name) {
	struct SkillGroup* _Group = CreateSkillType(_Name);

	_Group->Children = NULL;
	return _Group;
}

void DestroySkillGroup(struct SkillGroup* _Group) {
	DestroySkillType(_Group);
}

struct Skill* CreateSkill(const char* _Name) {
	struct Skill* _Skill = CreateSkillType(_Name);

	_Skill->Xp = 0;
	return _Skill;
}

void DestroySkill(struct Skill* _Skill) {
	DestroySkillType(_Skill);
}

struct SkillGroup* SkillGroupLoad(lua_State* _State, int _Index) {
	struct SkillGroup* _Group = NULL;
	struct SkillType** _Children = NULL;
	const char* _Name = NULL;

	lua_pushstring(_State, "Name");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TSTRING)
		return luaL_error(_State, "Skill group name parameter is not of type string.");
	lua_pop(_State, 1);
	lua_pushstring(_State, "Skills");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TTABLE)
		return luaL_error(_State, "Skill group skill parameter is not of type table.");
	_Children = calloc(lua_rawlen(_State, -1), sizeof(struct SkillType*));
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		SkillLoad(_State, -1);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	_Group = CreateSkillGroup(_Name);
	return _Group;
}

struct Skill* SkillLoad(lua_State* _State, int _Index) {
	const char* _Name = NULL;

	lua_pushstring(_State, "Name");
	lua_rawget(_State);
	if(lua_type(_State, -1) != LUA_TSTRING)
		return luaL_error(_State, "Skill name parameter is not of type string.");
	_Name = lua_tostring(_State, -1);
	lua_pop(_State, 1);
	return CreateSkill(_Name);
}

