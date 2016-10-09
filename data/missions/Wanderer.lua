Mission.Load {
	Name = "Wanderer aproches village",
	Description = "NoDesc",
	Options = {
		{
			Text = "Drive him away.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Invite him as a guest.",
			Trigger = function(Frame)
				--Indended to randomly fire one of many events that generate different wanderers.			
				Mission.FireEvent("WANDR.2", Frame.Owner)	
			end,
			AIUtility = function(Frame) end
		},
	},
	Trigger = function(Frame)
		return Frame.Owner:GetSettlement():GetLeader() == Frame.Owner
	end,
	MeanTime = {Base = 365 * 1.5},
	Id = "WANDR.1"
}

Mission.Load {
	Name = "Wandering blacksmith",
	Description = "I am [Random.FirstName] son of [Random.FirstName] the greatest blacksmith of these lands. I have come here to sell my arms to you but only if you prove yourself worthy.",
	Options = {
		{
			Text = "Arm wrestle him.",
			Trigger = function(Frame)
				if Frame.Owner:SkillCheck(Stat.Strength, 100) >= 0 then
					Mission.FireEvent("WANDR.3", Frame.Owner)
				else 
					Mission.FireEvent("WANDR.7", Frame.Owner)
				end
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Answer a riddle.",
			Trigger = function(Frame)
				Mission.FireEvent("WANDR.4", Frame.Owner)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy without showing your worth.",
			Trigger = function(Frame)
				Mission.FireEvent("WANDR.8", Frame.Owner)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy nothing.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "WANDR.2"
}

Mission.Load {
	Name = "Wrestling the blacksmith.",
	Description = "NoDesc.",
	Options = {
	},
	OnlyTriggered = false,
	Id = "WANDR.3"
}

Mission.Load {
	Name = "Answering blacksmith's riddle.",
	Description = "Nodesc.",
	Options = {
		{
			Text = "Answer correctly.",
			Trigger = function(Frame)
				Mission.FireEvent("WANDR.5", Frame.Owner)
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				return Frame.Score >= 4
			end
		},
		{
			Text = "Possible answer.",
			Trigger = function(Frame)
				if Frame.Answer == 1 then
					Mission.FireEvent("WANDR.5", Frame.Owner)
				else
					Mission.FireEvent("WANDR.6", Frame.Owner)
				end
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				return Frame.Score >= 4
			end
		},
		{
			Text = "Possible answer.",
			Trigger = function(Frame)
				if Frame.Answer == 2 then
					Mission.FireEvent("WANDR.5", Frame.Owner)
				else
					Mission.FireEvent("WANDR.6", Frame.Owner)
				end
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				return Frame.Score >= 2
			end
		},
		{
			Text = "Possible answer.",
			Trigger = function(Frame)
				if Frame.Answer == 3 then
					Mission.FireEvent("WANDR.5", Frame.Owner)
				else
					Mission.FireEvent("WANDR.6", Frame.Owner)
				end
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				return Frame.Score >= 1
			end
		},
		{
			Text = "Answer wrongly.",
			Trigger = function(Frame)
				Mission.FireEvent("WANDR.6", Frame.Owner)
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				return Frame.Score <= 0
			end
		}
	},
	OnTrigger = function(Frame)
		Frame.Score = Frame.Owner:SkillCheck(Stat.Inteligence, 80)
		Frame.Answer = Random(1, 3)
	end,
	OnlyTriggered = false,
	Id = "WANDR.4"
}

Mission.Load {
	Name = "Answered riddle correctly.",
	Description = "Nodesc.",
	Options = {
		{
			Text = "Buy seax",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(2)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Seax", 2))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy spear",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(2)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Spear", 1))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy sword",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(3)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Sword", 1))
			end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "WANDR.5"
}

Mission.Load {
	Name = "Answered riddle incorrectly.",
	Description = "How disapointing, you are not as great of a chief as I thought you were.",
	Options = {
		{
			Text = "Buy seax",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(3)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Seax", 2))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy spear",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(3)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Spear", 1))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy sword",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(5)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Sword", 1))
			end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "WANDR.6"
}

Mission.Load {
	Name = "Blacksmith won arm wrestle",
	Description = "NoDesc.",
	Options = {
		{
			Text = "Buy seax",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(3)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Seax", 2))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy spear",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(3)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Spear", 1))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy sword",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(5)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Sword", 1))
			end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "WANDR.7"
}

Mission.Load {
	Name = "Didn't show blacksmith worth",
	Description = "NoDesc.",
	Options = {
		{
			Text = "Buy seax",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(2)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Seax", 2))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy spear",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(3)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Spear", 1))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Buy sword",
			Trigger = function(Frame) 
				Frame.Owner:TakeWealth(4)
				Frame.Owner:GetFamily():GiveGood(CreateGood("Sword", 1))
			end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "WANDR.8"
}
