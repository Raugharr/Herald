/*
 * File: Goals.h
 * Author: David Brotz
 */

#ifndef __GOALS_H
#define __GOALS_H
struct GoapGoal;
struct GOAPPlanner;

void GoalChallangeLeader(struct GOAPPlanner* _Planner, struct GoapGoal* _Goal);
void GoalImproveRelations(struct GOAPPlanner* _Planner, struct GoapGoal* _Goal);
#endif
