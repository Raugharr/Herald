Mission.Load {
	Name = "Raise Popularity",
	Description = "Attempt to increase how popular you are with everone.",
	OnTrigger = function(Frame)
		Frame.Owner:ChangePopularity(1)
	end,
	Action = Action.RaisePop,
	OnlyTriggered = false,
	MeanTime = 90,
	Id = "Actions.1"
}

Mission.Load {
	Name = "Successful Envoy",
	Description = "[Target.FirstName]'s diplomatic envoy to [From.FirstName] was a success.",
	OnTrigger = function(Frame)
		local OwnerGov = Frame.Owner:GetPerson():GetFamily():GetSettlement():GetGovernment()
		local TargetGov = Frame.From:GetPerson():GetFamily():GetSettlement():GetGovernment()
		local Rel = OwnerGov:GetRelation(TargetGov)
		
		if Rel == nil then
			Rel = OwnerGov:CreateRelation(TargetGov)
			Rel:ChangeOpinion(Relation.Action.Envoy, Relation.Opinion.Average, Relation.Length.Medium, Relation.Opinion.Average)
			return;
		end
	end,
	Action = Action.Envoy,
	OnlyTriggered = false,
	MeanTime = 30,
	Id = "Envoy.1"
}

--[[Mission.Load {
	Name = "Murder Compensation",
	Description = "[Defender.FirstName] a relative of the deceased person comes to you pressing a legal claim. [Defender.Pronoun] demands that you [Demand].",
	Options = {
		{
			Text = "Refuse to give [Defender.FirstName] anything.",
			Trigger = function(Frame)
			    local Crisis = Frame:GetVar("Crisis")
			    
			    Crisis:SetResponce(-3)
			    Mission.FireEvent("Crisis.1", Frame:GetVar("Defender"))
			end
		},
		{
			Text = "Convince [Defender.FirstName] [Offender.FirstName] is innocent.",
			Trigger = function(Frame)
			    local Defender = Frame:GetVar("Defender")
			    local Success = Frame.Owner:OpposedChallange(Defender, Stat.Charisma)
			    
			    if Success >= 0 then
			        Mission.FireEvent("Murder.2", Defender);
			    else
			        Mission.FireEvent("Crisis.1", Defender)
			    end
			end
		},
		{
			Text = "Give half of the wergild.",
			Trigger = function(Frame)
			    Frame:GetVar("Crisis"):SetResponce(1)
			end,
			Condition = function(Frame)
			    local Offender = Frame:GetVar("Offender")
			    
			    return Offender:HasWealth(50) --half a cow.
			end
			AIUtility = function(Frame)
			end
		},
		{
			Text = "Give full wergild.",
			Trigger = function(Frame)
			    Frame:GetVar("Crisis"):SetResponce(2)
			end,
			Condition = function(Frame)
			    local Offender = Frame:GetVar("Offender")
		        
		        return Offender:HasWealth(100) --1 cow worth.
		},
		{
			Text = "Allow a trial.",
			Trigger = function(Frame)
			    local Tbl = {}
			    local Defender = Frame:GetVar("Defender")
			    local Offender = Frame:GetVar("Offender")
			    
			    Tbl.Defender = Defender;
			    Tbl.Offender = Offender;
			    Mission.FireEvent("Trial.2", Defender)
			    Mission.FireEvent("Trial.1", Frame.Owner, NULL, Tbl);
			end
		},
		{
			Text = "Agree to outlaw [Offender.FirstName].",
			Trigger = function(Frame)
			    local Outlaw = Frame:GetVar("Offender")
			    
			    Outlaw:Kill()
			    Frame:GetVar("Crisis"):SetResponce(3)
			end
		}
	},
	Id = "Murder.1"
}--]]

--Mission.Load {
  --  Name = "Compensation denied.",
    --Description = "The leader of [Offender.FirstName] [Offender.LastName]'s village has refused to give you any compensation.",
    --Id = "Crisis.1"
--}
