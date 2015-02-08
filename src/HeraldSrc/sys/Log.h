/*
 * Author: David Brotz
 * File: Log.h
 */

#ifndef __LOG_H
#define __LOG_H

#include "stdarg.h"

#define LOG_MAXSIZE (512)
#define LOG_LUAMSG "Function %s contains error (%s) on line %i"

typedef struct lua_State lua_State;

struct LogFile {
	int Level;
	int File;
	int Size;
	int Indents;
	char Buffer[LOG_MAXSIZE];
};

extern struct LogFile g_Log;

enum {
	ELOG_INFO = 1,
	ELOG_DEBUG = 2,
	ELOG_WARNING = 4,
	ELOG_ERROR = 8,
	ELOG_ALL = 15
};

int LogSetFile(const char* _File);
void LogCloseFile();

void Log(int _Category, const char* _Text, ...);
int LogLua(lua_State* _State);
#endif

