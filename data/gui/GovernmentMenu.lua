Menu.__savestate = false;
Menu.moveable = true;

function DisplayPolicyOptions(Menu, Right, Pol)
	Right:Clear()
	for k, Cat in ipairs(Pol:Options()) do
		local CatCont = Gui.HorizontalContainer(0, 0, Right:GetWidth(), 30, Menu.Right)
		local CatPol = Gui.HorizontalContainer(0, 0, Right:GetWidth(), 30, Menu.Right)
		CatCont:CreateLabel(Cat.Name)
		for j, Opt in ipairs(Cat) do
			if Menu.Government:GetPolicyCategory(Pol, k) ~= j then
				CatPol:CreateButton(Opt:Name(),
					function()
						Plot.Create(Menu.Government:GetLeader(), nil, Plot.Type.ChangePolicy, {Pol, k, j})
					end)
			else
				CatPol:CreateLabel(Opt:Name())
			end
		end
	end
end

function DisplayPolicyCategory(Menu, Right, Category)
	Right:Clear()
	for k, Pol in pairs(World.Policies()) do
		if Pol:Category() == Category then
			if Menu.Government:HasPolicy(Pol) ~= true then
			Right:CreateButton(Pol:Name(), 
				function()
					Plot.Create(Menu.Government:GetLeader(), nil, Plot.Type.NewPolicy, Pol)
				end)
			else
				Right:CreateButton(Pol:Name(),
					function()
						DisplayPolicyOptions(Menu, Right, Pol)
					end)
			end
		end
	end
end

function DisplayPolicyList(Menu, Left, Right)
	Left:Clear()
	Right:Clear()
	Left:CreateButton("Economy",
		function()
			DisplayPolicyCategory(Menu, Right, Policy.Economy)
		end)
	Left:CreateButton("Law",
		function()
			DisplayPolicyCategory(Menu, Right, Policy.Law)
		end)

	Left:CreateButton("Military",
		function()
			DisplayPolicyCategory(Menu, Right, Policy.Military)
		end)
	Left:CreateButton("Back",
		function()
			DisplayGovernment(Menu, Left, Right)
		end)
end

function DisplayAppointments(Menu, Left, Right)
	local Judge = "None"
	local Marshall = "None"
	local Steward = "None"
	local Temp = nil

	Right:Clear()
	Temp = Menu.Government:GetJudge()
	if Null(Temp) == false then
		Judge = Temp:GetName()	
	end

	Temp = Menu.Government:GetMarshall()
	if(Null(Temp) == false) then
		Marshall = Temp:GetName()	
	end

	Temp = Menu.Government:GetSteward()
	if(Null(Temp) == false) then
		Steward = Temp:GetName()	
	end
	Right:CreateLabel("Judge: " .. Judge)
	Right:CreateLabel("Marshall: " .. Marshall)
	Right:CreateLabel("Steward: " .. Steward) 
end

function DisplayGovernment(Menu, Left, Right)
	Left:Clear()
	Right:Clear()

	local TitleCon = Gui.HorizontalContainer(0, 0, Menu:GetWidth() / 2, 30, Menu.Left)
	local Title = TitleCon:CreateLabel("Government")

	TitleCon:SetFocus(false)
	Title:SetFocus(false)
	Title:SetX(TitleCon:GetHorizontalCenter(Title))

	Left:CreateLabel(Menu.Government:Rule() .. " " .. Menu.Government:Structure() .. " " .. Menu.Government:Type()):SetFocus(false)
	Left:CreateButton("View Policies",
		function()
			DisplayPolicyList(Menu, Left, Right)
		end)
	Left:CreateButton("View Appointments",
		function()
			DisplayAppointments(Menu, Left, Right)
		end)
	Menu.Left:CreateButton("Back", Gui.PopMenu)
end

function Menu.Init(Menu, Data)
	Menu.Government = Data["Settlement"]
	Menu.Left = Gui.VerticalContainer(0, 0, Menu:GetWidth() / 2, Menu:GetHeight(), Menu)
	Menu.Right = Gui.VerticalContainer(Menu:GetWidth() / 2, 0, Menu:GetWidth(), Menu:GetHeight(), Menu)
	DisplayGovernment(Menu, Menu.Left, Menu.Right)	
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
