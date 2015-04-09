ManorConstraints = {Min = 50, Max = 800, Interval = 50}
BabyAvg = {0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999}
AgeGroups = {0, 2190, 4745, 5659, 21900, 36500}
FamilyTypes = {{0.75, "Farmer"}, {0.2, "Herder"}, {0.05, "Lumberjack"}}

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
	else if(Person:GetGender() == 1) then
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
		CreateGood("Wheat", Wheat.PerAcre * 15 * Size + (Wheat.PerAcre * 15)),
		CreateGood("Rye", Rye.PerAcre * 5),
		CreateGood("Barley", Barley.PerAcre * 5),
		CreateGood("Oats", Oats.PerAcre * 5),
		CreateGood("Scratch Plow", 1)
	}
	Table.Field = {16, 16}
	Table.Buildings = {CreateBuilding({{"One Room House", 10, 15}}, GetBuildMat("Dirt"), GetBuildMat("Board"), GetBuildMat("Hay"), Location, "Human")}
	Table.Animals = {{"Chicken", Size * 2}}
	Table.AI = Peasant
	return Table
end

function Herder(Size, Location)
	local Table = {}
	local Goat = GetAnimal("Goat")
	local Pig = GetAnimal("Pig")
	local GoatCt = Size
	local PigCt = Size * 2
	
	Table.Goods = {CreateGood("Barley", (Pig.Nutrition * 356 * GoatCt) + (Goat.Nutrition * 365 * GoatCt))}
	Table.Field = {20}
	Table.Buildings = {CreateBuilding({{"One Room House", 10, 15}}, GetBuildMat("Dirt"), GetBuildMat("Board"), GetBuildMat("Hay"), Location, "All")}
	Table.Animals = {{"Goat", GoatCt}, {"Pig", GoatCt}}
	Table.AI = Peasant			
	return Table
end

function Lumberjack(Size, Location)
	local Table = Farmer(Size)
	
	table.insert(Table, {"Axe", 1})
	Table.AI = Peasant
	return Table;
end
