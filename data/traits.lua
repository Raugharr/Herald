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
		Likes = {"Coward"},
		Prevents = {"Brave"}
	},
	{
		Name = "Greedy",
		Dislikes = {"Greedy"},
		Likes = {"Ambitious"},
		Prevents = {}
	},
	{
		Name = "Loyal",
		Likes = {"Loyal", "Brave"},
		Dislikes = {"Coward"},
		Prevents = {}
	},
	{
		Name = "Ambitious",
		Dislikes = {"Coward"},
		Likes = {"Greedy"},
		Prevents = {}
	},
	{
		Name = "Glutton",
		Likes = {"Glutton"},
		Dislikes = {},
		Prevents = {}
	},
	{
		Name = "Honest",
		Likes = {},
		Dislikes = {},
		Prevents = {}
	},
	{
		Name = "Lazy",
		Likes = {},
		Dislikes = {},
		Prevents = {}
	},
	{
		Name = "Kind",
		Likes = {},
		Dislikes = {},
		Prevents = {}
	},
	{
		Name = "Narcissist",
		Likes = {},
		Dislikes = {},
		Prevents = {}
	},
	{
		Name = "Alcholic",
		Likes = {"Alcholic"},
		Dislikes = {},
		Prevents = {}
	}
}
