function Gui.Init()
	Gui.SetMenu("MainMenu")
end

Gui.LoadSkin {
	Name = "Default",
	Default = {
		Font = Gui.Font("Timeless.ttf", 14),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		--Background = Gui.Color(36, 24, 14),
		Background = Gui.Color(102, 89, 79),
	},
	Button = {
		Font = Gui.Font("Timeless.ttf", 16),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		Background = Gui.Color(102, 89, 79),
		Margins = {8, 8, 8, 8},
		Padding = {16, 0, 8, 0}
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
		Margins = {4, 4, 4, 4},
		BorderColor = Gui.Color(179, 151, 0),
		BorderWidth = 3
	},
	DefaultSkin = true
}

Gui.LoadSkin {
	Name = "Big",
	Default = {
		Font = Gui.Font("Timeless.ttf", 20),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		Background = Gui.Color(36, 24, 14),
		Margins = {4, 4, 4, 4}
	},
	Button = {
		Font = Gui.Font("Timeless.ttf", 18),
		FocusColor = Gui.Color(255, 215, 0),
		UnfocusColor = Gui.Color(230, 194, 0),
		Background = Gui.Color(102, 89, 79),
		--Background = Gui.Color(170, 149, 133),
		Padding = {8, 0, 16, 0},
		Margins = {6, 10, 12, 6}
	}
}

Gui.LoadSkin { 
	Name = "Header",
	Default = {
		Font = Gui.Font("Plain Germanica.ttf", 17),
		FocusColor = Gui.Color(31, 31, 107),
		UnfocusColor = Gui.Color(255, 255, 255),
		Background = Gui.Color(36, 24, 14)
	}
}

Gui.LoadSkin { 
	Name = "Title",
	Default = {
		Font = Gui.Font("Plain Germanica.ttf", 38),
		FocusColor = Gui.Color(31, 31, 107),
		UnfocusColor = Gui.Color(255, 255, 255),
		Margins = {8, 8, 8, 8},
		Background = Gui.Color(36, 24, 14)
	}
}
