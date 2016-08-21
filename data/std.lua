ManorConstraints = {Min = 100, Max = 400, Interval = 50}
BabyAvg = {0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999}
AgeGroups = {0, 2190, 4745, 5659, 21900, 36500}
FamilyTypes = {{0.84, "Farmer"}, {0.05, "Herder"}, {0.09, "Hunter"}, {0.01, "Metalsmith"}, {0.01, "Miller"}}

function CopyTable(Old)
	local New = {}
	for k, v in pairs(Old) do
		New[k] = v
	end
	return New
end

local function Peasant(Person)
	if(Person:GetAge() < ToMonth("Years", 13)) then
		return "Child"
	else if(Person:GetGender() == Person.Male) then
		return "Peasant"
	end
	end
		return "Woman"
end

function Farmer(Size, Location)
	local Table = {}
	local Wheat = Crop("Wheat")
	local Rye = Crop("Rye")
	local Barley = Crop("Barley")
	local Oats = Crop("Oats")
	
	Table.Goods = {
		--CreateGood("Wheat", (Person.DailyNut / Wheat.NutVal) * 365 * Size * (Wheat.YieldMult - 1) + (Wheat.PerAcre * 15)),
		CreateGood("Rye", Rye.PerAcre * 16),
		--CreateGood("Barley", Barley.PerAcre * 5),
		--CreateGood("Oats", Oats.PerAcre * 5),
		CreateGood("Scratch Plow", 1),
		CreateGood("Sickle", 1),
		CreateGood("Spear", 3)
	}
	Table.Field = {16, 16}
	Table.Buildings = {CreateBuilding(GetBuildMat("Dirt"), GetBuildMat("Board"), GetBuildMat("Hay"), Location, 100, "Human")}
	Table.Animals = {{"Chicken", Size * 2}}
	Table.AI = Peasant
	Table.Caste = "Peasant"
	return Table
end

function Miller(Size, Location)
	local Table = Farmer(Size, Location)
	
	Table.Caste = "Craftsman"
	return Table
end

function Herder(Size, Location)
	local Table = {}
	local Wheat = Crop("Wheat")
	local Goat = GetAnimal("Goat")
	local Pig = GetAnimal("Pig")
	local GoatCt = Size
	local PigCt = Size * 2
	
	Table.Goods = {CreateGood("Wheat", (8 / Wheat.NutVal) * 365 * 16 * Size), 
		CreateGood("Barley", (Pig.Nutrition * 356 * GoatCt) + (Goat.Nutrition * 365 * GoatCt)), CreateGood("Spear", 3)}
	Table.Field = {20}
	Table.Buildings = {CreateBuilding(GetBuildMat("Dirt"), GetBuildMat("Board"), GetBuildMat("Hay"), Location, 100, "All")}
	Table.Animals = {{"Goat", GoatCt}, {"Pig", GoatCt}}
	Table.AI = Peasant		
	Table.Caste = "Peasant"
	return Table
end

function Hunter(Size, Location)
	local Table = Farmer(Size, Location)

	Table.Animals ={{"Ox", Random(10, 20)}}
	Table.Buildings = {CreateBuilding(GetBuildMat("Dirt"), GetBuildMat("Board"), GetBuildMat("Hay"), Location, 100, "All")}
	Table.Caste = "Warrior"
	return Table
end

function Metalsmith(Size, Location)
	local Table = Farmer(Size, Location)
	
	Table.Caste = "Craftsman"
	return Table
end

function Lumberjack(Size, Location)
	local Table = Farmer(Size, Location)
	
	table.insert(Table, {"Axe", 1})
	Table.AI = Peasant
	return Table;
end
