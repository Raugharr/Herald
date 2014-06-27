/*
 * File: Setup.h
 * Author: David Brotz
 */

#ifndef __SETUP_H
#define __SETUP_H

struct Behavior;

#define AI_MAKEGOOD "MakeGood"
#define AI_MAKEBUILD "MakeBuild"
#define AI_REAP "Reap"
#define AI_PLOW "Plow"
#define AI_HOUSE "House"
#define AI_SHELTER "Shelter"

extern struct Behavior* g_AIMan;
extern struct Behavior* g_AIWoman;
extern struct Behavior* g_AIChild;

void AIInit();
void AIQuit();

#endif
