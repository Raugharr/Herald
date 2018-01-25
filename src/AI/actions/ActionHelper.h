/*
 * File: ActionHelper.h
 * Author: David Brotz
 */
#ifndef __ACTIONHELPER_H
#define __ACTIONHELPER_H

struct Policy;
struct ActivePolicy;
struct Agent;

/**
 * \brief Returns the policy that _Actor likes the least.
 */
struct Policy* HatedPolicy(const struct Agent* _Actor);
int JoinPolicyPlot(struct Agent* _Agent, struct Plot* _Plot, struct ActivePolicy* _Policy);

#endif
