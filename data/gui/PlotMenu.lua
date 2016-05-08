Menu.__savestate = false;
Menu.moveable = true;

require("BigGuyAux")

function DisplayPlotDefenders(Menu, Left, Right)
	FillPersonTable(CreatePersonTable(Right, 8), FillList(Menu.Plot:Defenders().Front, Menu.Plot:Defenders())) 
end

function DisplayPlotters(Menu, Left, Right)
	FillPersonTable(CreatePersonTable(Right, 8, FillList(Menu.Plot:Plotters())))
end

function DisplayPlotMenu(Menu, Left, Right)
	Left:CreateButton("Show Plotters", 
		function()
			DisplayPlotters(Menu, Left, Right)
		end)
	Left:CreateButton("Show Defenders", 
	function()
		DisplayPlotDefenders(Menu, Left, Right)
	end)
	Left:CreateButton("Back", GUI.PopMenu)		
end

function Menu.Init(Menu, Data)
	Menu.Left = GUI.VerticalContainer(0, 0, 400, Menu:GetHeight(), Menu)
	Menu.Right = GUI.VerticalContainer(401, 0, Menu:GetWidth(), Menu:GetHeight(), Menu) 
	Menu.Plot = Data.Plot

	DisplayPlotMenu(Menu, Menu.Left, Menu.Right)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
