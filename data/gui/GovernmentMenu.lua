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
	PolicyCont = Menu:CreateContainer(Menu:GetWidth(), Menu:GetHeight() - 360, Container.Vertical)
	DescCont = Menu:CreateContainer(Menu:GetWidth(), 200, Container.Fixed)
end

function Menu.Init(Menu, Data)
	Menu:OnNewChild(Container.Vertical)

	Menu.Government = Data.Settlement
	Menu:CreateLabel("Leader " .. Menu.Government:GetLeader():GetName())
	Menu:CreateLabel(Menu.Government:Rule() .. " " .. Menu.Government:Structure() .. " " .. Menu.Government:Type()):SetFocus(false)
	Menu:CreateLabel("Tax rate " .. Menu.Government:GetTaxRate() .. "%")
	DisplayPolicyList(Menu, Menu.Government)
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
