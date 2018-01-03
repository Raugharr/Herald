function Menu.Init(Menu, Guy)
	--local Guy = Menu.Guy	
	local Person = Guy:GetPerson()
	local Retinue = Person:Retinue()
	local Warriors = nil 
	local Branches = Retinue:GetBranches()
	if Retinue == nil then
		return	
	end
	Warriors = Retinue:Warriors()
	Menu:Clear()
	Menu:Paragraph("You currently have " .. Warriors:GetSize() .. " warriors in your retinue.")
	Menu:Paragraph("The leader of the retinue is " .. Retinue:Leader():GetPerson():GetName() .. " and has " .. Retinue:Leader():Glory() .. " glory.")
	for Val in ipairs(Branches) do
		Menu:CreateButton(Val:Leader():GetPerson():GetName(),
			function(Widget)
				Gui.CreateWindow("Retinue", {Guy =  Val:Leader()})
			end)
		Menu:CreateLabel("Troops: " .. Val:TroopCt())
	end
	FillWarriorTable(CreateWarriorTable(Right, Warriors:GetSize()), Warriors)
	CreateButton("Raise Retinue", 
		function(Widget)
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
