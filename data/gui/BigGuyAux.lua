local StatStr = {
	"Combat is a measurement of how well you are in single combat.",
	"Strength represents how strong a character is.",
	"Foo",
	"Foo",
	"Foo",
	"Charisma determines how well a character is able to convince another character of a fact, regardless if the fact is true or not.",
	"Intelligence is a measurement of how well a character is able to learn."
}

local ActionStr = {
	"Convince the local population through gifts that you are fit to rule them.",
	"Foo",
	"Foo",
	"Foo",
	"Foo",
	"Challange a person to a duel, if they refuse they will lose glory.",
	"Foo"
}

function CreateToolTipOnHover(Widget, Str)
	Widget.Str = Str
	Widget:OnHover(
	function(Widget)
		Widget.Window = CreateInfoWindow(Widget.Str)
	end)
	Widget:OnHoverLoss(
		function(Widget)
			Widget.Window:Close()
		end)
end

function BGStatsContainer(Menu, Guy)
	local Container = Menu:CreateTable(2, 7) --FIXME: shouldnt need to use a fixed max width and height.
	local Skin = Container:GetSkin()
	local Font = Skin:Table():GetFont()
	local Labels = {}

	Labels[1] = Container:CreateLabel("Combat");
	Container:CreateLabel(Guy:GetCombat())
	Labels[2] = Container:CreateLabel("Strength");
	Container:CreateLabel(Guy:GetStrength())
	Labels[3] = Container:CreateLabel("Toughness");
	Container:CreateLabel(Guy:GetToughness())
	Labels[4] = Container:CreateLabel("Agility");
	Container:CreateLabel(Guy:GetAgility())
	Labels[5] = Container:CreateLabel("Wit");
	Container:CreateLabel(Guy:GetWit())
	Labels[6] = Container:CreateLabel("Charisma");
	Container:CreateLabel(Guy:GetCharisma())
	Labels[7] = Container:CreateLabel("Intelligence");
	Container:CreateLabel(Guy:GetIntelligence())

	for k, v in ipairs(Labels) do
		CreateToolTipOnHover(Labels[k])
		Labels[k].Str = StatStr[k]	
	end
	Container:Shrink()
	return Container
end

function FillPersonTable(Tbl, PersonList)
	for k, v in ipairs(PersonList) do 
		Tbl:CreateLabel(v:GetName())	
		Tbl:CreateLabel(v:GetAge())
		if(v:GetGender() == 1) then
			Tbl:CreateLabel("Male")
		else
			Tbl:CreateLabel("Female")
		end
	end
end

function CreatePersonTable(Parent, Cols)
	local Table = Parent:CreateTable(3, Cols + 1)
	local Skin = Parent:GetSkin()
	local Font = Skin:Table():GetFont()
	
	Table:CreateLabel("Name")
	Table:CreateLabel("Age")
	Table:CreateLabel("Gender")
	return Table
end

function CreateWarriorTable(Parent, Cols)
	local Table = Parent:CreateTable(2, Cols + 1)
	local Skin = Parent:GetSkin()
	local Font = Skin:Table():GetFont()

	Table:CreateLabel("Name")
	Table:CreateLabel("Age")
	return Table
end

function FillWarriorTable(Tbl, PersonList)
--PersonList):Itr():Next() Is what ipairs below should resolve to.
	for v in ipairs(PersonList) do 
		Tbl:CreateLabel(v:GetName())	
		Tbl:CreateLabel(v:GetAge())
	end
end

function FillRelationList(Tbl, OpinionList)
	for k, v in ipairs(OpinionList) do
		Tbl:CreateLabel(v:Action())
		Tbl:CreateLabel(v:Relation())
	end
end

function CreateAnimalTable(Parent, Cols)
	local Table = Parent:CreateTable(4, Cols + 1)

	Table:CreateLabel("Name")
	Table:CreateLabel("Nutrition")
	Table:CreateLabel("Age")
	Table:CreateLabel("Gender")
	return Table
end

function CreateRelationTable(Parent, Cols)
	local Table = Parent:CreateTable(2, Cols + 1)

	Table:CreateLabel("Action");
	Table:CreateLabel("Modifier");
	return Table
end

function FillList(ItrFunc, ItrArg)
	local Table = {}
	local Idx = 1

	for v in ItrFunc(ItrArg) do
		Table[Idx] = v
		Idx = Idx + 1
	end
	return Table
end

function GenderName(Animal)
	if Person.Male == Animal:GetGender() then
		return "Male"
	end
	return "Female"
end

function GeneralActions(Menu, Owner, Target)
	local Labels = {}
	local ActionMenu = Gui.VerticalContainer(Menu, Menu:GetWidth() / 2, Menu:GetHeight() / 2)
	local InfoMenu = Gui.VerticalContainer(Menu, Menu:GetWidth() / 2, Menu:GetHeight() / 2)
	local DescLabel = nil
	local DescButton = nil
	local DescTbl = {}
	local Actions = {
		Action.RaisePop,
		Action.GainGlory,
		Action.Steal,
		Action.Befriend,
		Action.Murder,
		Action.Duel,
		Action.Slander
	}

function WidgetSetAction(Widget)
	Menu.Action = Widget.Action 
	Widget.DescMenu[1]:SetText(Widget.Str)	
end
	if Owner:Equal(Target) or Target == nil then
		Labels[1] = ActionMenu:CreateButton("Raise Popularity", WidgetSetAction)
		Labels[2] = ActionMenu:CreateButton("Raise Glory", WidgetSetAction)
	else
		Labels[3] = ActionMenu:CreateButton("Steal", WidgetSetAction)
		Labels[4] = ActionMenu:CreateButton("Befriend", WidgetSetAction)
		Labels[5] = ActionMenu:CreateButton("Murder", WidgetSetAction)
		Labels[6] = ActionMenu:CreateButton("Duel", WidgetSetAction)
		Labels[7] = ActionMenu:CreateButton("Slander", WidgetSetAction)
	end
	for k, v in pairs(Labels) do 
		v.Action = Actions[k]
		v.Str = ActionStr[k]
		v.DescMenu = DescTbl 
	end
	Menu:CreateButton("Close",
		function(Widget)
			Widget:GetParent():Close()
		end)
	DescLabel = InfoMenu:CreateLabel("NoDesc", true)
	DescButton = InfoMenu:CreateButton("Perform Action",
		function(Widget)
			if Target == nil then
				Target = Owner
			end
			Owner:SetAction(Menu.Action, Target)
			Menu:Close()
		end)
	DescTbl[1] = DescLabel
	DescTbl[2] = DescButton
end

function DisplayBigGuys(Menu, Owner)
	local Table = nil 
	local Agent = nil
	local Player = World.GetPlayer()
	local Relation = nil
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Settlement = Owner:GetFamily():GetSettlement()
	
	Table = Menu:CreateTable(4, 10)
	Table:CreateLabel("Name")
	Table:CreateLabel("Action");
	Table:CreateLabel("Our Opinion");
	Table:CreateLabel("Their Opinion");
	for Guy in Settlement:GetBigGuys():Front() do
		if Guy ~= World:GetPlayer() then
			Table:CreateButton(Guy:GetPerson():GetName(),
				function()
					Gui.CreateWindow("ViewPersonMenu", {Person = Guy:GetPerson()})--{Owner = Owner, Target = Guy:GetPerson()})
					Menu:GetParent():Close()
				end)
			Agent = Guy:GetAgent()
			if Agent ~= nil then
				Table:CreateLabel(Agent:GetAction())
				else
					Table:CreateLabel("Player")
			end
			Table:CreateLabel(RelationToStr(Player, Guy))
			Table:CreateLabel(RelationToStr(Guy, Player))
		end
	end
	Table:Shrink()
	return Table
end
