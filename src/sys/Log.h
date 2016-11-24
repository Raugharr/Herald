/*
 * Author: David Brotz
 * File: Log.h
 */

#ifndef __LOG_H
#define __LOG_H

#include "stdarg.h"

#define LOG_MAXSIZE (512)
#define LOG_LUAMSG "Function %s contains error (%s) on line %i"

#ifdef DEBUG
	#define Assert(_Bool)	\
		if((_Bool) == 0) {	\
			Log(ELOG_DEBUG, (#_Bool));	\
		}

	#define AssertIntEq(_Left, _Right)				\
		if(_Left != _Right) {						\
			Log(ELOG_DEBUG, (#_Left##!=##_Right));	\
		}
	#define AssertIntLt(_Left, _Right)				\
		if(_Left < _Right) {						\
			Log(ELOG_DEBUG, (#_Left##<##_Right));	\
		}

#else
#define Assert(_Bool)
#define AssertIntEq(_Left, _Right)
#define AssertIntLt(_Left, _Right)
#endif

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
	ELOG_INFO = (1 << 0),
	ELOG_DEBUG = (1 << 1),
	ELOG_WARNING = (1 << 2),
	ELOG_ERROR = (1 << 3),
	ELOG_ALL = (1 << 4) - 1
};

int LogSetFile(const char* _File);
void LogCloseFile();

void Log(int _Category, const char* _Text, ...);
int LogLua(lua_State* _State);
#endif

