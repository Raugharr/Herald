Menu.Width = 500
Menu.Height = 600
Menu.moveable = true;

function DisplayPolicyOptions(Menu, Right, Pol)
	Right:Clear()
	for k, Cat in ipairs(Pol:Options()) do
		local CatCont = Gui.HorizontalContainer(Right:GetWidth(), 30, Menu.Right)
		local CatPol = Gui.HorizontalContainer(Right:GetWidth(), 30, Menu.Right)
		CatCont:CreateLabel(Cat.Name)
		for j, Opt in ipairs(Cat) do
			if Menu.Government:GetPolicyCategory(Pol, k) ~= j then
				CatPol:CreateButton(Opt:Name(),
					function(Widget)
						Plot.Create(Menu.Government:GetLeader(), nil, Plot.Type.ChangePolicy, {Pol, k, j})
					end)
			else
				CatPol:CreateLabel(Opt:Name())
			end
		end
	end
end

function DisplayPolicyCategory(Menu, DescCont, Government, Category)
	for k, Pol in pairs(World.Policies()) do
		if Pol:Category() == Category then
			Menu:CreateButton(Pol:GetName(), 
				function(Widget)
						if DescCont.Text ~= nil then
							DescCont:DestroyChildren()
						end
						DescCont.Text = DescCont:CreateLabel(Pol:GetDescription())
						DescCont.Support = DescCont:CreateLabel("Support " .. Government:PolicyApproval(Pol) .."%")
						DescCont.Support:Below(DescCont.Text)
						local PassBut = DescCont:CreateButton("Pass Policy",
							function(Widget)
							end)
						local SupportBut = DescCont:CreateButton("Increase Support",
						function(Widget)
							World.GetPlayer():GetFaction():SetPolicyInfluence(Pol)
						end)
						PassBut:Below(DescCont.Support)
						SupportBut:RightOf(PassBut)
				end)
		end
	end
end

function PolicyListOnClick(Widget, Parent, DescCont, Government, PolicySelected, Category)
	if PolicySelected == true then
		Parent:DestroyChildren()
	end
	DisplayPolicyCategory(Parent, DescCont, Government, Category)
end

function DisplayPolicyList(Menu, Government)
	local ListCont = Menu:CreateContainer(Menu:GetWidth(), Menu:GetHeight(), Container.Horizontal)
	local PolicyCont = nil
	local DescCont = nil
	
	ListCont.PolicySelected = false
	ListCont:CreateButton("Economy",
		function(Widget)
			PolicyListOnClick(Widget, PolicyCont, DescCont, Government, ListCont.PolicySelected, Policy.Economy)
			ListCont.PolicySelected = true 
		end)
	ListCont:CreateButton("Law",
		function(Widget)
			PolicyListOnClick(Widget, PolicyCont, DescCont, Government, ListCont.PolicySelected, Policy.Law)
			ListCont.PolicySelected = true 
		end)
	ListCont:CreateButton("Military",
		function(Widget)
			PolicyListOnClick(Widget, PolicyCont, DescCont, Government, ListCont.PolicySelected, Policy.Military)
			ListCont.PolicySelected = true 
		end)
	ListCont:Shrink()
	PolicyCont = Menu:CreateContainer(Menu:GetWidth(), Menu:GetHeight() - 420, Container.Vertical)
	DescCont = Menu:CreateContainer(Menu:GetWidth(), 100, Container.Fixed)
	print(DescCont:GetY())
end

function Menu.ShowOptions(Menu)
	local PlayerGov = World.GetPlayer():GetSettlement():GetGovernment()

	Menu:CreateButton("Become Independent",
		function(Widget)
			PlayerGov:LesserRemove()
		end)
end

function Menu.Init(Menu, Data)
	local Table = Menu:CreateTable(1, 5)	
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()

	Menu:OnNewChild(Container.Vertical)

	Menu.Government = Data.Settlement

	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	Table:CreateLabel("Leader " .. Menu.Government:GetLeader():GetName())
	Table:CreateLabel("Rule " .. Menu.Government:Rule()):SetFocus(false)
	Table:CreateLabel("Structure " .. Menu.Government:Structure() .. " " .. Menu.Government:Type() .. " " .. Menu.Government:RankStr())
	if Menu.Government:IsPlayerOwned() == false then
		Menu:CreateButton("Diplomancy",
			function(Widget)
				Menu:Close()
				Gui.CreateWindow("Diplomancy", {Government = Data.Settlement})
			end)
	else
		Menu:CreateButton("Diplomancy",
			function(Widget)
				Menu:ShowOptions()
			end)
	end

	--[[Menu.Tabs = Menu:CreateContainer(Menu:GetWidth(), Menu:GetHeight() - 420, Container.Horizontal)
	Menu.Tabs:CreateButton("Clients",
		function(Widget)
		end)
	Menu.Tabs:CreateButton("Treaties", 
		function(Widget)
		end)
	--DisplayPolicyList(Menu, Menu.Government)
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)--]]
	Menu.Tabs = Menu:CreateStack(Menu:GetWidth(), Menu:GetHeight() - 420)
	local Clients = Menu.Tabs:AddTab("Clients")
	local Treaties = Menu.Tabs:AddTab("Treaties")
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
end

function TreatyOptions(Menu, TarGov)
	local PlayerGov = World.GetPlayer():GetGovernment()	
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
