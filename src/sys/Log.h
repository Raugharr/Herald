/*
 * Author: David Brotz
 * File: Log.h
 */

#ifndef __LOG_H
#define __LOG_H

enum {
	ELOG_INFO = 0,
	ELOG_DEBUG,
	ELOG_WARNING,
	ELOG_ERROR
};

void SetFilter(int _Level);
int LogSetFile(const char* _File);
void LogCloseFile();
void Log(int _Category, const char* _Text, ...);

#endif

