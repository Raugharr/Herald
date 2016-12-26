function Gui.Init()
	--GUI.SetFont(GUI.GetFont("Timeless.ttf", 18))
	--GUI.SetUnfocusColor(188, 188, 188)
	--GUI.SetUnfocusColor(198, 198, 198)
	--GUI.SetUnfocusColor(204, 204, 204)
	--GUI.SetFocusColor(255, 255, 255)
	Gui.SetMenu("MainMenu")
end

Gui.LoadSkin {
	Name = "Default",
	Default = {
		Font = Gui.Font("Timeless.ttf", 15),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		Background = Gui.Color(36, 24, 14),
		--Margins = {4, 4, 4, 4}
	},
	Button = {
		Font = Gui.Font("Timeless.ttf", 18),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		Background = Gui.Color(128, 128, 128)
	},
	Label = {
		Font = Gui.Font("Timeless.ttf", 16),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		Background = Gui.Color(36, 24, 14),
		Margins = {4, 4, 4, 4}
	},
	Container = {
		Font = Gui.Font("Timeless.ttf", 15),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		Background = Gui.Color(36, 24, 14),
		Margins = {4, 4, 4, 4}
	},
	DefaultSkin = true
}

Gui.LoadSkin { 
	Name = "Header",
	Default = {
		Font = Gui.Font("Plain Germanica.ttf", 16),
		FocusColor = Gui.Color(31, 31, 107),
		--UnfocusColor = Gui.Color(31, 31, 107),
		UnfocusColor = Gui.Color(255, 255, 255),
		Background = Gui.Color(36, 24, 14)
	}
}
