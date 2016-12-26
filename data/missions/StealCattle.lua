Mission.Load {
	Name = "Steal Cattle",
	Description = "It would be nice to have more cattle.",
	Options = {
		{
			Text = "Yes",
			Trigger = function (Frame)
				local Target = Frame:RandomPerson({Male = true, Adult = true})
				local Success = Frame.Owner:OpposedChallange(Target, Stat.Wit)

				if Success >= 0 then
					Mission.FireEvent("STLCT.3", Frame.Owner, Target)	
				else
					Mission.FireEvent("STLCT.2", Frame.Owner, Target)
				end
			end,
			AIUtility = function(Frame)
			end
		},
	},
	Action = Action.Steal,
	OnlyTriggered = false,
	MeanTime = {
		Base = 60,
		{
			Modifier = 0.8,
			Trigger = function(Frame)
				return Frame.Owner:HasTrait("Greedy") == true
			end
		}
	},
	Id = "STLCT.1"
}

Mission.Load {
	Name = "Successful Cattle Raid",
	Description = "You have sucessfully stolen a cow from [From.FirstName] [From.LastName].",
	Id = "STLCT.2",
	OnTrigger = function(Frame)
		Frame.Owner:TakeAnimal(Frame.Owner:GetFamily():GetSettlement(), Frame.From(), "Cow", 1)
	end,
	OnlyTriggered = false,
}

Mission.Load {
	Name = "Caught in the act.",
	Description = "While stealing cattle [From.FirstName] sees you.",
	Options = {
		{
			Text = "Try to run away.",
			Trigger = function(Frame)
				--AddHint(Hint.OpposedChallange, Stat.Agility)
				if Frame.Owner:OpposedChallange(Frame.From, Stat.Agility) > 0 then
					Mission.FireEvent("STLCT.5", Frame.Owner, Frame.From)
					Mission.FireEvent("STLCT.6", Frame.From, Frame.Owner)
				else
					Mission.FireEvent("STLCT.8", Frame.From, Frame.Owner)
					Mission.FireEvent("STLCT.9", Frame.Owner, Frame.From)
				end
			end,
			AIUtility = function(Frame)
				return {Utility.OpposedChallange(Frame.From, Frame.Owner, Stat.Agility)}
			end
		},
		{
			Text = "Convince them otherwise.",
			Trigger = function(Frame)
				--AddHint(Hint.OpposedChallange, Stat.Charisma)
				if Frame.Owner:OpposedChallange(Frame.From, Stat.Charisma) > 0 then
				else
				end
			end,
			AIUtility = function(Frame)
				return {Utility.OpposedChallange(Frame.From, Frame.Owner, Stat.Agility)}
			end
		},
		{
			Text = "Kill [From.FirstName]",
			Trigger = function(Frame)
				Mission.FireEvent("DUEL.2", Frame.Owner, Frame.From)
			end,
			AIUtility = function(Frame)
				return {Utility.OpposedChallange(Frame.From, Frame.Owner, Stat.Combat)}
			end
		},
		{
			Text = "Pay wergeld.",
			Trigger = function(Frame)
			end,

			AIUtility = function(Frame)
			end
		}
	},
	Id = "STLCT.3",
	OnlyTriggered = false,
}

Mission.Load {
	Name = "Missing Cows.",
	Description = "You wake up to find you have lost one of your cattle.",
	--Post on bulitin for help.
	Id = "STLCT.4",
	OnlyTriggered = false,
}

Mission.Load {
	Name = "Outran [From.FirstName].",
	Description = "Seeing [From.FirstName] you ran as fast as you could.",
	Options = {
		{
			Text = "That was close",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame)
			end
		}
	},
	Id = "STLCT.5",
	OnlyTriggered = false,
}

Mission.Load {
	Name = "Thief escapes.",
	Description = "The shadowed figure trying to steal from you has out ran you.",
	Options = {
		{
			Text = "Justice!",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame)
			end
		}
	},
	Id = "STLCT.6",
	OnlyTriggered = true
}

Mission.Load {
	Name = "Mistaken identity.",
	Description = "[From.FirstName] has explained that [From.Pronoun] is just passing by.",
	Options = {
		{
			Text = "Sorry for the misunderstanding.",
			Trigger = function(Frame)

			end,
			AIUtility = function(Frame)

			end
		},
		{
			Text = "Wait a moment.",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.1", Frame.From, Frame.Owner)
			end,
			AIUtility = function(Frame)

			end,
			Condition = function(Frame)
				return Frame.Owner:SkillCheck(Stat.Wit, 100) >= 0
			end
		}
	},
	Id = "STLCT.7",
	OnlyTriggered = true
}

Mission.Load {
	Name = "Thief caught.",
	Description = "You caught the theif who was really [From.FirstName] [From.LastName]!",
	Options = {
		{
			Text = "Publicly shame [From.Pronoun].",
			Trigger = function(Frame) 
				Mission.FireEvent("STLCT.9", Frame.Owner, Frame.From)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "I must satisify my revenge against [From.FirstName]!",
			Trigger = function(Frame)
				Mission.FireEvent("DUEL.2", Frame.Owner, Frame.From)
			end,
			AIUtility = function(Frame) end
		}
	},
	Id = "STLCT.8",
	OnlyTriggered = true
}

--Crashes the game by calling change popularity on a person instead of a BigGuy.
Mission.Load {
	Name = "Caught escaping.",
	Description = "Though you tried to run, [From.FirstName] has caught up to you and seen your face.",
	Options = {
		{
			Text = "There is nothing I can do.",
			Trigger = function(Frame) 
				Frame.From:ChangePopularity(-5)
				Frame.From:SetOpinion(Frame.Owner, Relation.Action.Theft, 10, Relation.Length.Large, Relation.Opinion.Average)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Kill [From.Pronoun].",
			Trigger = function(Frame)
				Mission.FireEvent("DUEL.2", Frame.Owner, Frame.Sender)
			end,
			AIUtility = function(Frame) end
		}
	},
	Id = "STLCT.9",
	OnlyTriggered = true
}
