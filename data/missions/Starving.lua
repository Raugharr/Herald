function GiveStarvingFood(Frame, Honor)
	local Nut = Frame.Person:GetFamily():GetNutReq() * 90

	Frame.Person:GetFamily():ChangeNutrition(Nut)
	Frame.Owner:GetFamily():ChangeNutrition(-Nut)
	Frame.Owner:ChangePopularity(Honor)
end

--[[Mission.Load {
	Name = "Starving Family.",
	Description = "A family in your village is nearly out of food and will surely starve without the help of another.",
	Options = {
		{
			Text = "Give them food freely.",
			Trigger = function(Frame)
				GiveStarvingFood(Frame, 1.5)
			end,
			AIUtility = function(Frame) end	
		},
		{
			Text = "Give them food; make them pay it back.",
			Trigger = function(Frame)
				GiveStarvingFood(Frame, 1.0)
			end,
			AIUtility = function(Frame) end	
		},
		{
			Text = "Give them food; make them thralls.",
			Trigger = function(Frame)
				GiveStarvingFood(Frame, 0.5)
			end,
			AIUtility = function(Frame) end	
		},
		{
			Text = "I have no spare food to give.",
			Trigger = function(Frame)
				Frame.Owner:ChangePopularity(-0.5)
			end,
			AIUtility = function(Frame) end	
		}
	},
	Event = Event.OnStarving,
	Id = "STARV.1"
}--]]
