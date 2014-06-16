/*
 * Author: David Brotz
 * File: Log.h
 */

#ifndef __LOG_H
#define __LOG_H

enum {
	EAPPLICATION = (1<<0),
	EPERSON = (1<<1),
	EFAMILY = (1<<2)
};

void SetFilter(int _Flags);
void Log(int _Category, const char* _Text, ...);

#endif

