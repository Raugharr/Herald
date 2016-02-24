Traits = {
	{
		Name = "Brave",
		Likes = {"Loyal"},
		Dislikes = {"Coward"},
		Prevents = {"Coward"}
	},
	{
		Name = "Coward",
		Dislikes = {"Brave", "Loyal"},
		Likes = {},
		Prevents = {"Brave"}
	},
	{
		Name = "Greedy",
		Dislikes = {"Greedy"},
		Likes = {},
		Prevents = {}
	},
	{
		Name = "Loyal",
		Likes = {"Brave"},
		Dislikes = {"Coward"},
		Prevents = {}
	}
}
