local ThiefVar = 0

Mission.Load {
	Name = "Finding the theif",
	Description = "foobar",
	Options = {
		{
			Text = "Ask neighbors for information.",
			AIUtility = function(Frame) end
		},
		{
			Text = "Go door to door.",
			AIUtility = function(Frame) end
		},
		{
			Text = "Accuse [VarTwo.FirstName] [VarTwo.LastName].",
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "THIEF.1"
}
