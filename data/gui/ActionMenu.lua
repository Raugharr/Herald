Menu.moveable = true
Menu.Width = 500
Menu.Height = 600

require("BigGuyAux")

function Menu.Init(Menu, Data)
	Menu:OnNewChild(Container.Horizontal)
	GeneralActions(Menu, Data.Owner, Data.Target)	
end

function Menu.Think(Menu)

end
