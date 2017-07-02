Mission.Load {
	Name = "Man murdered.",
	--Description  = "[From.FirstName] [From.LastName] has been murdered most foul.",
	OnTrigger = function(Frame)
--		if Frame.Owner:SuccessMargin(Stat.Agility) > 0 then
--			Mission.FireEvent("Murder.3", Frame.Owner, Frame.From)
--		else
			--Mission.FireEvent("Murder.2", Frame.Owner, Frame.From)
--		end
			Mission.FireEvent("Murder.6", Frame.Owner, Frame.From)
	end,
	Action = Action.Murder,
	NoMenu = true,
	OnlyTriggered = false,
	MeanTime = 10,
	Id = "Murder.1"
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
--]]
Mission.Load {
	Name = "Failed murder",
	Description = "Failed to kill [From.FirstName].",
	OnTrigger = function(Frame)
--		if Random(0, 1) == 0 then
--			Settlement:BulitinPost(Frame.From, "Murder.4", "Murder.5", 30, 1)
--		else
			BigGuy.Crisis(Frame.Owner, Frame.From, Crisis.Murder)
--		end
	end,
	OnlyTriggered = false,
	Id = "Murder.2"
}

Mission.Load {
	Name = "Sucessful murder",
	Description = "You sucessfully killed [From.FirstName].",
	OnTrigger = function(Frame)
		local Family = Frame.From:GetFamily()
		local Settlement = Family:GetSettlement()
		for Person in ipairs(Family:GetFamily():GetPeople()) do
			--FIXME: Use macro and not 13 for the adult age.
			if Person:GetAge() >= 13 then
				Settlement:BulitinPost(Frame.From, "Murder.4", "Murder.5", 30, 1)
				break
			end	
		end
		Frame.Owner:Murder(Frame.From)
	end,
	Id = "Murder.3",
	OnlyTriggered = false,
}

Mission.Load {
	Name = "Found the murderer",
	Description = "You found him",
	Id = "Murder.4",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Culprit unfound",
	Description = "No one is interested in finding the murderer.",
	Id = "Murder.5",
	OnlyTriggered = false,
	MeanTime = 1
}

Mission.Load {
	Name = "Attempted murder",
	Description = "A [From.FirstName] [From.LastName] has tried to murder you!",
	Options = {
		{
			Text = "Escape to a nearby village.",
			Trigger = function(Frame)
				local SetList = Frame.QuerySettlement({Distance = 20, Target = Frame.Owner:GetSettlement()})
				local Setlmnt = SetList:Front()

				Mission.FireEvent("FamMove.1", Frame.Owner, Setlmnt:GetLeader(), {{"NewHome", Setlmnt}, {"SetList", SetList}})
				SetList:RemoveFront()
			end,
			AIUtility = function(Frame)
				return 1
			end
		},
		{
			Text = "Duel [From.Pronoun].",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.2", Frame.From, Frame.Owner)
			end,
			AIUtility = function(Frame)
				return 1
			end
		},
		{
			Text = "Let the council find a solution.",
			Trigger = function(Frame)
				BigGuy.Crisis(Frame.Owner, Frame.From, Crisis.Murder)
			end,
			AIUtility = function(Frame)
				return 1
			end
		}
	},
	OnlyTriggered = false,
	Id = "Murder.6",
}
