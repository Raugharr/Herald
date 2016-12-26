Traits = {
	{
		Name = "Brave",
		Likes = {"Loyal"},
		Dislikes = {"Coward"},
		Prevents = {"Coward"}
	},
	{
		Name = "Coward",
		Likes = {"Coward"},
		Dislikes = {"Brave", "Loyal"},
		Prevents = {"Brave"}
	},
	{
		Name = "Greedy",
		Likes = {"Ambitious"},
		Dislikes = {"Greedy"},
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
		Likes = {"Greedy"},
		Dislikes = {"Coward"},
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
		Likes = {"Honest", "Kind"},
		Dislikes = {"Greedy"},
		Prevents = {}
	},
	{
		Name = "Lazy",
		Likes = {},
		Dislikes = {"Ambitious"},
		Prevents = {"Ambitious"}
	},
	{
		Name = "Kind",
		Likes = {"Kind", "Honest"},
		Dislikes = {"Cruel"},
		Prevents = {"Cruel"}
	},
	{
		Name = "Cruel",
		Likes = {"Cruel", "Greedy"},
		Dislikes = {"Kind"},
		Prevents = {}
	},
	{
		Name = "Narcissist",
		Likes = {},
		Dislikes = {"Honest", "Narcissist"},
		Prevents = {}
	},
	{
		Name = "Alcholic",
		Likes = {"Alcholic"},
		Dislikes = {},
		Prevents = {}
	}
}
