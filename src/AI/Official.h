/*
 * File: Official.h
 * Author: David Brotz
 */

struct GovernmentBlackboard {
	struct Government* Target;
	uint16_t Distance;
	uint16_t Strength;
};

struct OfficialBlackboard {
	struct Government* Target;
};

struct AgentOfficial {
	struct Agent* Agent;
	struct OfficialBlackboard;
};
