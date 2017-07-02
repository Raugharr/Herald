Mission.Load {
	Name = "In search of a home",
	Description = "As your family looks for a new place to call home, you come across the village [NewHome.Name]. Do you wish to ask for permission to become a member of this village or look else where?",
	Options = {
		{
			Text = "Ask to stay.",
			OnTrigger = function(Frame)
				Mission.FireEvent("FamMove.2", Frame.From, Frame.Owner, {{"SetList", Frame:GetVar("SetList")}})
			end,
		},
		{
			Text = "Look for a home else where.",
			OnTrigger = function(Frame)
				local SetList = Frame:GetVar("SetList")
				local Setlmnt = SetList:Front()

				Mission.FireEvent("FamMove.1", Frame.Owner, Setlmnt:GetLeader(), {{"NewHome", Setlmnt}, {"SetList", SetList}})
				SetList:RemoveFront()
			end
		}
	},
	MeanTime = 15,
	OnlyTriggered = false,
	Id = "FamMove.1"
}

Mission.Load {
	Name = "Strangers ask to join village",
	Description = "A family of strangers have asked if they can join your village.",
	Options = {
		{
			Text = "Allow them to stay.",
			OnTrigger = function(Frame)
				Frame.From:GetFamily():SetHome(Frame.Owner:GetFamily():GetHome())
			end
		},
		{
			Text = "Demand that they leave.",
			OnTrigger = function(Frame)

			end
		}
	},
	OnlyTriggered = false,
	Id = "FamMove.2"
}

Mission.Load {
	Name = "Allowed to join village",
	Description = "[From.FirstName] [From.LastName] has allowed your family to live in his village.",
	OnTrigger = function(Frame)
		Frame.Owner:GetFamily():SetSettlement(Frame.From:GetSettlement())
	end,
	OnlyTriggered = false,
	Id = "FamMove.3"
}

Mission.Load {
	Name = "Prevented from joining village",
	Description = "[From.FirstName] [From.LastName] has refused to allow your family to live in his village.",
	OnTrigger = function(Frame)
		Mission.FireEvent("FamMove.1", Frame.Owner, Frame.From, {{"SetList", Frame:GetVar("SetList")}})
	end,
	OnlyTriggered = false,
	Id = "FamMove.3"
}
