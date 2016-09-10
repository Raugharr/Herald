local LeaderStat = {}
local BanditCount = 12
local BanditStrength = BanditCount * 6

LeaderStat[Stat.Combat] = 75
LeaderStatpStat.Strength] = 75
LeaderStat[Stat.Toughness] = 75

local function EnoughWarriors(Frame)
	local Settlement = Frame.Owner:GetSettlement()
	local Warriors = Settlement:GetMaxWarriors()
	local Men = (Settlement:MaleAdults() - Warriors) + (Warriors * 4)

	if Men > BanditStrength then
		return true
	return false
end

local function FightBandits(Frame)
	local Settlement = Frame.Owner:GetSettlement()
	local Warriors = 0
	local Men = 0
	local WarriorRatio = 0 
	local MenDead = 0 
	local WarriorsDead = 0

	if Frame[FyrdRaised] == 1 then
		Warriors = Settlement:GetMaxWarriors()
		Men = (Settlement:MaleAdults() - Warriors)
		WarriorRatio = Warriors / Men
	elseif Frame[FyrdRaised] == 2 then
		Warriors = Settlement:GetMaxWarriors()
		Men = 0
		WarriorRatio = 1
	elseif Frame[FyrdRaised] == 3 then 
		Warriors = Frame.Owner:GetRetinue():GetSize()
		Men = 0
		WarriorRatio = 1
	end
	MenDead = (BanditStrength * 4) / ((1 - WarriorRatio) * Men)
	WarriorsDead = BanditStrength / (WarriorRatio * Warriors) 
	if MenDead > Men then
		MenDead = Men
	end
	if WarriorsDead > Warriors then
		WarriorsDead = Warriors
	end
	for Itr in Frame.RandomPerson({Warrior = true, Male = true, Count = MenDead}) do
		Itr:Kill()
	end
	for Itr in Frame.RandomPerson({Warrior = false, Male = true, Count = WarriorsDead}) do
		Itr:Kill()
	end
end

Mission.Load {
	Name = "Bandits Spotted",
	Description = "While working on the outskirts of the village you see a dozen or more bandits in the distance heading towards the village.",
	Options = {
		{
			Text = "Bring anyone who is willing to fight.",
			Trigger = function(Frame) 
			Frame[FyrdRaised] = 1
			if EnoughWarriors(Frame) == true then
				Mission.FireEvent("BANDT.2", Frame.Owner)
				Frame.Owner:ChangePopularity(-1)
			else
				Mission.FireEvent("BANDT.3", Frame.Owner)
			end
			end,
			AIUtility = function(Frame) 
				return {Utility.Linear(Frame.Owner:GetSettlement():GetMaxWarriors(), BanditCount * 1.5),
						Utility.IParabula((Settlement:MaleAdults() - Warriors), Settlement:MaleAdults())}
			end
		},
		{
			Text = "Bring every warrior.",
			Trigger = function(Frame) 
			Frame[FyrdRaised] = 2
			if EnoughWarriors(Frame) == true then
				Mission.FireEvent("BANDT.2", Frame.Owner)
				Frame.Owner:ChangePopularity(-.5)
			else
				Mission.FireEvent("BANDT.3", Frame.Owner)
			end
			end,
			AIUtility = function(Frame) 
				return Utility.Linear(Frame.Owner:GetSettlement():GetMaxWarriors(), BanditCount * 1.5)
			end,
			Condition = function(Frame)
				return Frame.Owner:GetSettlement():GetMaxWarriors() > 0
			end
		},
		{
			Text = "Bring your retinue.",
			Trigger = function(Frame) 
			Frame[FyrdRaised] = 3
			if EnoughWarriors(Frame) == true then
				Mission.FireEvent("BANDT.2", Frame.Owner)
				Frame.Owner:ChangePopularity(-.5)
			else
				Mission.FireEvent("BANDT.3", Frame.Owner)
			end
			end,
			AIUtility = function(Frame) end
		},
	},
	Id = "BANDT.1",
	MeanTime = {
		Base = 3650
	}
}

Mission.Load {
	Name = "Bandits Arive",
	Description = "When the bandits arrive at your village they find that every warrior from the village is waiting for them. Seeing so many people ready to fight as made them pause but not enough to leave with nothing. The bandit leader steps foward and demands payment or they will attack.",
	Options = {
		{
			Text = "Refuse to give the bandits anything.",
			Trigger = function(Frame) 
				FightBandits(Frame)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Give them enough food for a week.",
			Trigger = function(Frame) 
				Frame.Owner:GetFamily():TakeNutrition(Person.Nutrition * BanditCount * 7)
				Mission.FireEvent("BANDT.8", Frame.Owner)
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				local Nutrition = Frame.Owner:GetFamily():GetNutrition()

				return (Nutrition < Frame.Owner:GetFamiy():GetNutReq() * 30)
			end
		},
		{
			Text = "Give them gifts.",
			Trigger = function(Frame)
				Frame.Owner:TakeWealth(Person.Nutrition * BanditCount * 10)
				Mission.FireEvent("BANDT.8", Frame.Owner)
			 end,
			AIUtility = function(Frame) end
			Condition = function(Frame)
				local Wealth = Frame.Owner:GetFamily():GetWealth()

				return (Wealth < Frame.Owner:GetFamiy():GetNutReq() * 30)
			end
		}
	},
	Id = "BANDT.2",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Bandits Arive",
	Description = "When the bandits arrive at your village they see everyone you have mustered. The bandit leader steps foward and laughs at the fyrd ready to stop him.",
	Options = {
		{
			Text = "We will not live ruled by a foreigner.",
			Trigger = function(Frame) 
				FightBandits(Frame)
			end,
			AIUtility = function(Frame) end	
		},
		{
			Text = "Bribe them with enough goods to leave.",
			Trigger = function(Frame)
				Frame.Owner:TakeWealth(Person.Nutrition * BanditCount * 10)
				Mission.FireEvent("BANDT.8", Frame.Owner)
			end,
			AIUtility = function(Frame) end
			Condition = function(Frame)
				return Frame.Owner:HasWealth(Person.Nutrition * BanditCount * 10)
			end
		},
		{
			Text = "We will pay tribute only if you can defeat me.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame)
				return {Utility.Stat(Frame.Owner:GetCombat(), LeaderStat[Stat.Combat]),
						Utility.Stat(Frame.Owner:GetStrength(), LeaderStat[Stat.Strength]),
						Utility.Stat(Frame.Owner:GetToughness(), LeaderStat[Stat.Toughness])}
			end
		},
		{
			Text = "There is no need for bloodshed; give yearly tribute.",
			Trigger = function(Frame) 
				Frame.Owner:SetFlag("BanditTribute", true)
				for Itr in Frame.Owner:GetSettlement():GetFamilies() do
					Itr:TakeWealth(Person.Nutrition * 7) 
				end
			end,
			AIUtility = function(Frame) end
		}
	},
	Id = "BANDT.3",
	OnlyTriggered = false
}

Mission.Load {
	Name = "A fight between warriors.",
	Description = "You have accepted the challange from [From.FirstName] and will fight to the death.",
	Options = {
		{
			Name = "May your sword miss and your shield crumble.",
			Trigger = function(Frame)
				local Result = Mission.CombatRound(Frame.Owner, Frame.Sender)
				if Result > 1 then
					if Random(1, 5) ~= 1 then
						Mission.FireEvent("BANDT.5", Frame.Owner, Frame.Sender)
					else
						Mission.FireEvent("BANDT.6", Frame.OWner, Frame.Sender)
					end
				elseif Result < 1 then
					Mission.FireEvent("BANDT.7", Frame.Owner, Frame.Sender)
				end
			end
		}
	},
	Id = "BANDT.4",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Defeated bandit leader.",
	Description = "After a protracted duel you have emerged victorius agains the bandit leader. The rest of the bandits, disheartened leave without causing any more trouble.",
	Options {
		{
			Text = "Hopefully they wont return again.",
			Trigger = function(Frame)
				Frame.Owner:ChangePopularity(1)
			end,
		}
	},
	Id = "BANDT.5",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Bandits angered by leader's death.",
	Description = "Seeing their leader fall to the ground dead only angers the remaining bandits into breaking their word and attacking your village anyways.",
	Options = {
		{
			Name = "Bandits can never be trusted.",
			Trigger = function(Frame) 
				FightBandits(Frame)
			end,
		}
	},
	Id = "BANDT.6",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Bandit leader victorius.",
	Description = "Everyone in your village is silent as the battle ends in your defeat. No one else is willing to stand up to the bandits now that their chef has fallen.",
	Options = {
		{
			Name = "We are doomed.",
			Trigger = function(Frame) end,
		}
	},
	Id = "BANDT.7",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Bandits paid off.",
	Description = "The bandits being payed offer no further trouble and leave the same way as they came.",
	Options = {
		{
			Name = "Hopefully they won't return.",
			Trigger = function(Frame) end,
		}
	},
	Id = "BANDT.8",
	OnlyTriggered = false
}
