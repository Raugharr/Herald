Mission.Load {
	Name = "Bandits Spotted",
	Description = "While working on the outskirts of the village you see a dozen or more bandits in the distance heading towards the village.",
	Options = {
		{
			Text = "Bring anyone who is willing to fight.",
			Trigger = function(Frame) 
				local Settlement = Frame.Owner:GetSettlement()
				local Warriors = Settlement:GetMaxWarriors()
				local Men = (Settlement:MaleAdults() - Warriors) + (Warriors * 2)

				if Men > 20 then
					Mission.FireEvent("BANDT.2", Frame.Owner)
				else
					Mission.FireEvent("BANDT.3", Frame.Owner)
				end
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Bring every warrior.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end

		},
		{
			Text = "Bring your retinue.",
			Trigger = function(Frame) end,
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
			Text = "Refuse to give them anything.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Give them enough food for a week.",
			Trigger = function(Frame) 
				Frame.Owner:GetFamily():TakeNutrition(Person.Nutrition * 12 * 7)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Give them trade goods.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
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
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end	
		},
		{
			Text = "Bribe them with enough weapons to leave.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		},
		{
			Text = "We will pay tribute only if you can defeat me.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		},
		{
			Text = "There is no need for bloodshed; give yearly tribute.",
			Trigger = function(Frame) end,
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
			AIUtility = function(Frame) end			
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
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
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
			AIUtility = function(Frame) end
		}
	},
	Id = "BANDT.7",
	OnlyTriggered = false
}
