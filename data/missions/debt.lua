Mission.Load {
	Name = "Debt owed",
	Description = "You owe [Debt.AsMoney] worth of goods.",
	Options = {
		{
			Text = "Pay the debt",
			Trigger = function(Frame)
		--		local Debt = Frame:GetVar("Debt")
				
		--		Frame.Owner:TransferGoods(Frame.Sender, Debt)
			end,
			Condition = function(Frame)
		--		local Debt = Frame:GetVar("Debt")
		--		local Wealth = Frame:GetVar("Wealth")

		--		return Debt <= Wealth
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Refuse to pay",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Run away.",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame)
			end
		},
		{
			Text = "Sell yourself into slavery.",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame) end
		}
	},
	OnTrigger = function(Frame)
		Frame:SetVar("Wealth", Frame.Owner:GetWealth())
	end,
	OnlyTriggered = true,
	Id = "Debt.1"
}


