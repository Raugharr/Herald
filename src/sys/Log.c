/*
 * File: Log.c
 * Author: David Brotz
 */

#include "Log.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <lua/lua.h>
#include <SDL2/SDL.h>
#ifndef _WIN32
	#include <unistd.h>
#endif

const char* g_LogCatStr[] = {
	"Info",
	"Debug",
	"Warning",
	"Error"
};

struct LogFile g_Log = {0, -1, 0, 0};

int LogSetFile(const char* _File) {
	if(g_Log.File >= 0)
		close(g_Log.File);
#ifdef _WIN32
	if((g_Log.File = open(_File, _O_WRONLY, _O_CREAT, _O_TRUNC)) < 0)
#else
	if((g_Log.File = open(_File, O_WRONLY | O_CREAT | O_TRUNC)) < 0)
#endif
		return 0;
	return 1;
}

void LogCloseFile() {
	close(g_Log.File);
}

void LogFlush() {
	write(g_Log.File, g_Log.Buffer, g_Log.Size);
	g_Log.Size = 0;
}

void LogAppendStr(const char* _Text, int _Size) {
	int i = 0;

	while(_Size > 0) {
		if(g_Log.Size >= LOG_MAXSIZE) {
			LogFlush();
		} else
			g_Log.Buffer[g_Log.Size++] = _Text[i++];
		--_Size;
	}
	LogFlush();
}

void Log(int _Category, const char* _Text, ...) {
	va_list _List;
	char _Buffer[LOG_MAXSIZE + g_Log.Indents];
	int _Size;
	int i = 0;
	int _ExtraSz = 0;
	
	if((g_Log.Level & _Category) != _Category)
		return;
	for(i = 0; i < g_Log.Indents; ++i)
		_Buffer[i] = '\t';
	va_start(_List, _Text);
	_ExtraSz = snprintf(&_Buffer[g_Log.Indents], LOG_MAXSIZE - 2, "[%s] %f: ", g_LogCatStr[ffs(_Category) - 1], ((double)SDL_GetTicks()) / 1000);
	_Size = i;
	_Size += vsnprintf(&_Buffer[g_Log.Indents + _ExtraSz], LOG_MAXSIZE + g_Log.Indents - 2, _Text, _List) + 1;
	strcat(_Buffer, "\n");
	LogAppendStr(_Buffer, _Size + _ExtraSz);
	g_Log.Size = 0;
}

int LogLua(lua_State* _State) {
	lua_Debug _Debug;
	int _Size = 0;
	//va_list _List;

	if((g_Log.Level & ELOG_ERROR) != ELOG_ERROR)
		return 0;

	if(lua_getstack(_State, 0, &_Debug) == 0) {
		Log(ELOG_ERROR, lua_tostring(_State, -1));
		return 0;
	}
	lua_getinfo(_State, "nlS", &_Debug);
	//Adding 6 for _Debug.currentline and one for the null terminator
	_Size = strlen(LOG_LUAMSG) + strlen(_Debug.name) + 7;
	char _Str[_Size];
	//char _Ftext[LOG_MAXSIZE];
	//va_start(_List, _Text);
	//vsnprintf(_Ftext, LOG_MAXSIZE, _Text, _List);
	sprintf(_Str, LOG_LUAMSG, _Debug.name, lua_tostring(_State, -1), _Debug.currentline);
	Log(ELOG_ERROR, _Str);
	return 0;
}
