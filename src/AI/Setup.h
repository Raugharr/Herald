/*
 * File: Setup.h
 * Author: David Brotz
 */

#ifndef __SETUP_H
#define __SETUP_H

struct Behavior;

#define MAKEGOOD "MakeGood"

extern struct Behavior* g_AIMan;
extern struct Behavior* g_AIWoman;
extern struct Behavior* g_AIChild;

void AIInit();
void AIQuit();

#endif
