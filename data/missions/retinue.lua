--[[
	This mission will randomly pick two people in a retinue to fight. However it would be better to instead
	have the people decide if they want to fight in case the person picked is a player. We could then also use
	traits to increase how often a person will fight with others in their retinue.
--]]
Mission.Load {
	Name = "Fighting in the retinue.",
	Description = "Nodesc",
	Options = {
		{
			Text = "Demand the the warlord gives you justice.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) return end
		},
		{
			Text = "Duel [From.FirstName].",
			Trigger = function(Frame)
				if Frame.Owner:SkillCheck(Stat.Charisma, 100) < 0 then

				end
			end,
			AIUtility = function(Frame) return end
		},
		{
			Text = "Settle things diplomatically.",
			Trigger = function(Frame) 
				if Frame.Owner:SkillCheck(Stat.Charisma, 100) >= 0 then
					Mission.FireEvent("RETIN.8", Frame.Var("Target"), Frame.Owner)
				else
					Mission.FireEvent("RETIN.9", Frame.Var("Target"), Frame.Owner)
					Mission.FireEvent("RETIN.9", Frame.Owner, Frame.Var("Target"))
				end
			end,
			AIUtility = function(Fame) return end
		},
		{
			Text = "Demand compensation.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		}

	},
	OnlyTriggered = false,
	Id = "RETIN.1"
}

Mission.Load {
	Name = "Spoils demanded.",
	Description = "You see that [VarOne.FirstName] is taking an equal share of the loot yet has only done half as much fighting as you.",
	Options = {
		{
			Text = "Let it go",
			Trigger = function(Frame)
				
			end,
			AIUtility = function(Frame) end,
			Condition =  function(Frame)
				return Frame.Owner:HasTrait("Kind") == true
			end
		},
		{	
			Text = "Demand the warlord give you half of his share.",
			Trigger = function(Frame) 
				if Mission.Owner:SkillCheck(Stat.Charisma, 100) then
				end
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Steal half of [VarOne.Pronoun] share.",
			Trigger = function(Frame)
				if Mission.Owner:SkillCheck(Stat.Agility, 100) >= 0 then
					Mission.Owner:TakeWealth(Mission.Event:Spoils() / 2)
				end
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				return Frame.Owner:HasTrait("Honest") == false
			end
		},
		{
			Text = "Steal all of [VarOne.Pronoun] spoils.",
			Trigger = function(Frame)
				if Mission.Owner:SkillCheck(Stat.Agility, 100) >= 0 then
				--	Mission.Owner:TakeWealth(Mission.Event:Spoils())
				end
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				return Frame.Owner:HasTrait("Greedy")
			end
				
		}
	},
	--Weight = 1,
	Event = Event.OnBattleEnd,
	Id = "RETIN.2"	
}

Mission.Load {
	Name = "Warrior refuses to return arms.",
	Description = "One of your warriors who is to leave your retinue has declared that the weapons loaned to him should belong to him and is now refusing to return them to you.",
	Options = {
		{
			Text = "Take them back by force.",
			Trigger = function(Frame) 
				--Frame.Owner:SetRelation
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Declare [VarOne.Pronoun] an outlaw.",
			Trigger = function(Frame)

			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Fine [VarOne.FirstName] a large sum.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		}
	},
	Event = Event.OnQuitRetinue,
	Id = "RETIN.3"
}

Mission.Load {
	Name = "Needy Thegn",
	Description = "[VarOne.FirstName] wants to join your retinue but will only do so if you give him more arms than you give new warriors.",
	Options = {
		{
			Text = "Try to reason with [VarOne.Pronoun].",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Give [VarOne.Pronoun] the same amount as everyone else.",
			Trigger = function(Frame)
				if Random(1, 4) ~= 4 then
					Frame.Var(1):LeaveRetinue()
				end
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "A greedy man such as [VarOne.FirstName] is not wanted in my retinue.",
			Trigger = function(Frame)
				Frame.Var(1):LeaveRetinue()
			end,
			AIUtility = function(Frame) end
		}
	},
	Event = Event.OnJoinRetinue,
	Id = "RETIN.4"
}

Mission.Load {
	Name = "Thegn wants thralls.",
	Description = "[From.FirstName] will not join your retinue saying, a thegn as strong as me deserves to have some thralls to tend to my lands.",
	Options = {
		{
			Text = "Give him one family.",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "I have no thralls.",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame) end
		}
	},
	Event = Event.OnJoinRetinue,
	Id = "RETIN.5"
}

Mission.Load {
	Name = "Weak warlord",
	Description = "One one your warriors, [VarOne.FirstName]  approaches you and declares you are not fit to lead us. As son of [VarOne.Father.FirstName] i would be more likely to bring us glory than you.",
	Options = {
		{
			Text = "Step down.",
			Trigger = function(Frame)
				local Retinue = Frame.Owner:GetPerson():Retinue()

				Frame.Owner:ChangeGlory(-15)
				Retinue:ReplaceLeader(Frame.Var(1))
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Duel [VarOne.FirstName].",
			Trigger = function(Frame)

			end,
			AIUtility = function(Frame) end
		}
	},
	Trigger = function(Frame)
		local Retinue = Frame.Owner:GetPerson():Retinue()

		if Null(Retinue) then
			return false
		end
		return Retinue:Leader() == Frame.Owner and Frame.Owner:Glory() < 80
	end,
	MeanTime = {
		Base = 365 * 5,
		{
			Modifier = 0.8,
			Trigger = function(Frame)
				return Frame.Owner:Glory() < 60
			end
		},
		{
			Modifier = .75,
			Trigger = function(Frame)
				return Frame.Owner:Glory() < 40
			end
		},
			Modifier = .33,
			Trigger = function(Frame)
				return Frame.Owner:Glory() < 20
			end
	},
	Id = "RETIN.6"
}

Mission.Load {
	Name = "Warrior gives gifts.",
	Description = "NoDesc",
	OnTrigger = function(Frame)
	--	Frame.Owner:GetPerson():Retinue():Leader():AddGood(Frame.Owner:TakeWealth(1))
	end,
	Trigger = function(Frame)
		local Retinue = Frame.Owner:GetPerson():Retinue()

		if Null(Retinue) then
			return false
		end
		return Retinue:Leader() ~= Frame.Owner
	end,
	MeanTime = {
		Base = 365 * 5,
		{
			Modifier = 0.8,
			Trigger = function(Frame)
				return Frame.Owner:Glory() < 60
			end
		},
		{
			Modifier = .75,
			Trigger = function(Frame)
				return Frame.Owner:Glory() < 40
			end
		},
			Modifier = .33,
			Trigger = function(Frame)
				return Frame.Owner:Glory() < 20
			end
	},
	Id = "RETIN.7"
}

Mission.Load {
	Name = "Dispute settled diplomatically.",
	Description = "Nodesc.",
	OnTrigger = function(Frame)
		Frame.Owner:ChangeOpinion(Frame.From, 5)
		Frame.From:ChangeOpinion(Frame.Owner, 5)
	end,
	OnlyTriggered = false,
	Id = "RETIN.8",
}

Mission.Load {
	Name = "Dispute failed to be settled.",
	Description = "You and [From.FirstName] have been unable to settle the differences between yourselves.",
	OnTrigger = function(Frame)
		Frame.Owner:ChangeOpinion(Frame.From, -5)
	end,
	OnlyTriggered = false,
	Id = "RETIN.9"
}

Mission.Load {
	Name = "Warlord settles dispute.",
	Description = "One of the warriors in your retinue [From.FirstName] [From.LastName] tells you that he and [Quarrel.FirstName] have been quarreling and asks for your help.",
	Options = {
		{
			Text = "Favor [From.FirstName].",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Favor [Quarrel.FirstName].",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Invistiage who caused the issue.",
			Trigger = function(Frame) 
				if Frame.Owner:SkillCheck(Stat.Wit, 100) >= 0 then
					Mission.FireEvent("RETIN.11", Mission.Var("Quarrel"), Mission.Owner)
					--Punish true person.
				else
					Mission.FireEvent("RETIN.12", Mission.From, Mission.Owner)
					--Punish false person.
				end
			end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "RETIN.10"
}

Mission.Load {
	Name = "Retinue fighting",
	Description = "Nodesc.",
	NoMenu = true,	
	OnTrigger = function(Frame)
		local PList = Frame:RandomPerson({Adult = true, Male = true, BigGuy = true, Count = 1})
		if #PList < 1 then return end
		Mission.FireEvent("RETIN.1", PList[1])
	end,
	Trigger = function(Frame) 
		local Retinue = Frame.Owner:GetPerson():Retinue()
		
		if Retinue:Null() == true then
			return false
		end
		return Retinue:Leader() ~= Frame.Owner
	end,
	MeanTime = {
		Base = 365 * 1,
		{
			Modifier = 0.8,
			Trigger = function(Frame)
				return Frame.Owner:HasTrait("Greedy")
			end
		},
		{
			Modifier = 1.2,
			Trigger = function(Frame)
				return Frame.Owner:HasTrait("Coward") or Frame.Owner:HasTrait("Kind")
			end
		}
	},
	Id = "RETIN.11"
}
