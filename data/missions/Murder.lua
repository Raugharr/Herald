Mission.Load {
	Name = "Man murdered.",
	Description  = "[0.FirstName] [0.LastName] has been murdered most foul.",
	OnTrigger = function(Frame)
		Frame.SetVar(0, RandomPerson({Adult = true, Count = 1, Male = true}))
	end,
	MeanTime = 365 * 20
}
--[[Mission.Load {
	Name = "Murder Plot",
	Description = "Plotting to kill %s.",
	TextFormat = {BigGuy.GetName(Mission.Sender())},
	Id = "MURDR.1",
	OnlyTriggered = false,
	--Instead of the below trigger randomly pick a mission that is related to the skill of the assassin.
	--The higher the skill of the assassin the easier the mission is. Each mission would have a different
	--description for flavor.
	OnTrigger = Rule.IfThenElse(
		Rule.GreaterThan(
			BigGuy.GetWit(Mission.Sender()),
			BigGuy.GetWit(Mission.Owner())),
		BigGuy.Kill(Mission.Owner()),
		Mission.FireEvent("MURDR.2", Mission.Sender(), Mission.Owner())),
	NoMenu = true,
	MeanTime = 1
}

Mission.Load {
	Name = "Failed murder",
	Description = "Failed to kill %s.",
	TextFormat = {BigGuy.GetName(Mission.Sender())},
	Id = "MURDR.2",
	OnlyTriggered = false,
	OnTrigger = Settlement.BulitinPost("MURDR.4", "MURDR.5", 30, 1),
	MeanTime = 1
}

Mission.Load {
	Name = "Sucessful murder",
	Description = "You killed %s.",
	TextFormat = {BigGuy.GetName(Mission.Sender())},
	Id = "MURDR.3",
	OnlyTriggered = false,
	MeanTime = 1
}

Mission.Load {
	Name = "Found the murderer",
	Description = "You found him",
	Id = "MURDR.4",
	OnlyTriggered = false,
	OnTrigger = Rule.True(),
	MeanTime = 1
}

Mission.Load {
	Name = "Culprit unfound",
	Description = "No one is interested in finding the murderer.",
	Id = "MURDR.5",
	OnlyTriggered = false,
	MeanTime = 1
}
--]]
