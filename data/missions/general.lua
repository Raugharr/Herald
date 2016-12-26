Mission.Load {
	Name = "Cart breaks down.",
	Description = "",
	NoMenu = true,
	OnTrigger = function(Frame)
		local Target = Frame.RandomPerson({Adult = true, Male = true}) 

		for Itr in Frame.Owner:GetTraits():Next() do
			local TraitName = Itr:GetName()

			if TraitName == "Greedy" or TraitName:Name() == "Lazy" then
				Mission.FireEvent("General.2", Frame.Owner, Target)
			elseif TraitName == "Honest" then
				Mission.FireEvent("General.3", Frame.Owner, Target)
			end
		end
	end,
	MeanTime = 365 * 5,
	Id = "General.1"
}

Mission.Load {
	Name = "Cart breaks down; unhelpful.",
	Description = "You didn't help [From.FirstName] fix their cart.",
	OnTrigger = function(Frame)
		Frame.Owner:SetOpinion(Frame.From, Relation.Action.General, -10, Relation.Length.Medium, Relation.Opinion.Average)
	end,
	OnlyTriggered = false,
	Id = "General.2"
}

Mission.Load {
	Name = "Cart breaks down; helpful.",
	Description = "You did help [From.FirstName] fix their cart.",
	OnTrigger = function(Frame)
		Frame.Owner:SetOpinion(Frame.From, Relation.Action.General, 10, Relation.Length.Medium, Relation.Opinion.Average)
	end,
	OnlyTriggered = false,
	Id = "General.2"
}

Mission.Load {
	Name = "Help person with problem.",
	Description = "[Target.FirstName] is in need of help with a problem.",
	OnTrigger = function(Frame)
		
	end,
	MeanTime = 365 * 5,
	Id = "General.3"
}

Mission.Load {
	Name = "Help person with problem; ridicule.",
	Description = "[From.FirstName] is mocked instead of helped.",
	OnTrigger = function(Frame)
			
	end,
	MeanTime = 365 * 5,
	Id = "General.3"
}

--[[
These random events should be decided based on the traits of the owner of the event. The owner will then be given a few options whose outcome will
make a group of people like or hate him more. A group of people are people with the same trait.
--]]
