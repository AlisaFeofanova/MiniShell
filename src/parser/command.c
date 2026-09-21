#include "../../includes/minishell.h"

t_cmd	*cmd_new(void)
{
	t_cmd	*cmd;

	cmd = malloc(sizeof(t_cmd));
	if (!cmd)
		return (NULL);
	cmd->argv = NULL;
	cmd->redirs = NULL;
	cmd->next = NULL;
	return (cmd);
}

void	cmd_clear(t_cmd **cmds)
{
	t_cmd	*current;
	t_cmd	*next;
	int		i;

	if (!cmds)
		return ;
	current = *cmds;
	while (current)
	{
		next = current->next;
		if (current->argv)
		{
			i = 0;
			while (current->argv[i])
			{
				free(current->argv[i]);
				i++;
			}
			free(current->argv);
		}
		redir_clear(&current->redirs);
		free(current);
		current = next;
	}
	*cmds = NULL;
}

void	cmd_add_back(t_cmd **cmds, t_cmd *new_cmd)
{
	t_cmd	*current;

	if (!cmds || !new_cmd)
		return ;
	if (!*cmds)
	{
		*cmds = new_cmd;
		return ;
	}
	current = *cmds;
	while (current->next)
		current = current->next;
	current->next = new_cmd;
}