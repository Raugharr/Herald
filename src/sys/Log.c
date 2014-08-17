/*
 * File: Log.c
 * Author: David Brotz
 */

#include "Log.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>

#define LOG_MAXSIZE (512)

int g_Level = 0;
int g_LogFile = -1;
int g_LogSize = 0;
char g_LogBuffer[LOG_MAXSIZE];

void SetFilter(int _Level) {
	g_Level = _Level;
}

int LogSetFile(const char* _File) {
	if(g_LogFile >= 0)
		close(g_LogFile);
	if((g_LogFile = open(_File,  _O_WRONLY | _O_CREAT | _O_TRUNC)) < 0)
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
	
	va_start(_List, _Text);
	_Size = vsnprintf(_Buffer, LOG_MAXSIZE - 2, _Text, _List) + 2;
	strcat(_Buffer, "\n");
	
	while(_Size > 0) {
		if(g_LogSize >= LOG_MAXSIZE) {
			if(write(g_LogFile, g_LogBuffer, g_LogSize) != g_LogSize)
				return;
			g_LogSize = 0;
		} else
			g_LogBuffer[g_LogSize] = _Buffer[i];
		++g_LogSize;
		++i;
		--_Size;
	}
	if(write(g_LogFile, g_LogBuffer, g_LogSize) != g_LogSize)
		return;
	g_LogSize = 0;
}
