#include "../../includes/minishell.h"

static char	*token_type_to_string(t_token_type type)
{
	if (type == TOKEN_WORD)
		return ("WORD");
	if (type == TOKEN_PIPE)
		return ("PIPE");
	if (type == TOKEN_REDIR_IN)
		return ("REDIR_IN");
	if (type == TOKEN_REDIR_OUT)
		return ("REDIR_OUT");
	if (type == TOKEN_APPEND)
		return ("APPEND");
	if (type == TOKEN_HEREDOC)
		return ("HEREDOC");
	return ("UNKNOWN");
}

void	print_tokens(t_token *tokens)
{
	while (tokens)
	{
		printf("TOKEN %-12s [%s]\n",
			token_type_to_string(tokens->type),
			tokens->value);
		tokens = tokens->next;
	}
}

static char	*redir_type_to_string(t_redir_type type)
{
	if (type == REDIR_IN)
		return ("<");
	if (type == REDIR_OUT)
		return (">");
	if (type == REDIR_APPEND)
		return (">>");
	return ("<<");
}

void	print_commands(t_cmd *cmds)
{
	t_cmd		*cmd;
	t_redir		*redir;
	int			i;
	int			number;

	cmd = cmds;
	number = 1;
	while (cmd)
	{
		printf("\nCOMMAND %d\n", number);

		printf("  argv:\n");
		i = 0;
		while (cmd->argv && cmd->argv[i])
		{
			printf("    [%s]\n", cmd->argv[i]);
			i++;
		}

		printf("  redirections:\n");
		redir = cmd->redirs;
		while (redir)
		{
			printf("    %s [%s]\n",
				redir_type_to_string(redir->type),
				redir->file);
			redir = redir->next;
		}

		cmd = cmd->next;
		number++;
	}
}