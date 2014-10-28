/*
 * File: Log.c
 * Author: David Brotz
 */

#include "Log.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <lua/lua.h>
#ifndef _WIN32
	#include <unistd.h>
#endif

#define LOG_MAXSIZE (512)
#define LOG_LUAMSG "Function %s contains error (%s) on line %i"

int g_LogLevel = 0;
int g_LogFile = -1;
int g_LogSize = 0;
char g_LogBuffer[LOG_MAXSIZE];

void SetFilter(int _Level) {
	g_LogLevel = _Level;
}

int LogSetFile(const char* _File) {
	if(g_LogFile >= 0)
		close(g_LogFile);
#ifdef _WIN32
	if((g_LogFile = open(_File, _O_WRONLY, _O_CREAT, _O_TRUNC)) < 0)
#else
	if((g_LogFile = open(_File, O_WRONLY, O_CREAT, O_TRUNC)) < 0)
#endif
		return 0;
	return 1;
}

void LogCloseFile() {
	close(g_LogFile);
}

void Log(int _Category, const char* _Text, ...) {
	va_list _List;
	char _Buffer[LOG_MAXSIZE];
	int _Size;
	int i = 0;
	
	if(g_LogLevel > _Category)
		return;
	va_start(_List, _Text);
	_Size = vsnprintf(_Buffer, LOG_MAXSIZE - 2, _Text, _List) + 1;
	strcat(_Buffer, "\n");
	while(_Size > 0) {
		if(g_LogSize >= LOG_MAXSIZE) {
			if(write(g_LogFile, g_LogBuffer, g_LogSize) != g_LogSize)
				return;
			g_LogSize = 0;
		} else
			g_LogBuffer[g_LogSize++] = _Buffer[i++];
		--_Size;
	}
	if(write(g_LogFile, g_LogBuffer, g_LogSize) != g_LogSize)
		return;
	g_LogSize = 0;
}

void LogLua(lua_State* _State, int _Category, const char* _Text, ...) {
	lua_Debug _Debug;
	int _Size = 0;
	va_list _List;

	if(g_LogLevel > _Category)
		return;

	if(lua_getstack(_State, 0, &_Debug) == 0)
		return;
	lua_getinfo(_State, "nlS", &_Debug);
	//Adding 6 for _Debug.currentline and one for the null terminator
	_Size = strlen(LOG_LUAMSG) + strlen(_Debug.name) + 7;
	char _Str[_Size];
	char _Ftext[LOG_MAXSIZE];
	va_start(_List, _Text);
	vsnprintf(_Ftext, LOG_MAXSIZE, _Text, _List);
	sprintf(_Str, LOG_LUAMSG, _Debug.name, _Ftext, _Debug.currentline);
	Log(_Category, _Str);
}
