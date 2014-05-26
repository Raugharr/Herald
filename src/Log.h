/*
 * Author: David Brotz
 * File: Log.h
 */

#ifndef __LOG_H
#define __LOG_H

enum {
	EPERSON = (1<<1),
	EFAMILY = (1<<2)
};

void SetFilter(int _Flags);
void Log(const char* _Text, int _Category);

#endif

