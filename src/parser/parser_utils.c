#include "../../includes/minishell.h"

t_redir	*redir_new(t_redir_type type, char *file)
{
	t_redir	*redir;

	redir = malloc(sizeof(t_redir));
	if (!redir)
		return (NULL);
	redir->type = type;
	redir->file = ft_strdup(file);
	if (!redir->file)
	{
		free(redir);
		return (NULL);
	}
	redir->next = NULL;
	return (redir);
}

void	redir_add_back(t_redir **redirs, t_redir *new_redir)
{
	t_redir	*current;

	if (!redirs || !new_redir)
		return ;
	if (!*redirs)
	{
		*redirs = new_redir;
		return ;
	}
	current = *redirs;
	while (current->next)
		current = current->next;
	current->next = new_redir;
}

void	redir_clear(t_redir **redirs)
{
	t_redir	*current;
	t_redir	*next;

	if (!redirs)
		return ;
	current = *redirs;
	while (current)
	{
		next = current->next;
		free(current->file);
		free(current);
		current = next;
	}
	*redirs = NULL;
}

char	**argv_add(char **argv, char *value)
{
	int		size;
	int		i;
	char	**new_argv;

	size = 0;
	while (argv && argv[size])
		size++;

	new_argv = malloc(sizeof(char *) * (size + 2));
	if (!new_argv)
		return (NULL);

	i = 0;
	while (i < size)
	{
		new_argv[i] = argv[i];
		i++;
	}
	new_argv[size] = ft_strdup(value);
	if (!new_argv[size])
	{
		free(new_argv);
		return (NULL);
	}
	new_argv[size + 1] = NULL;
	free(argv);
	return (new_argv);
}