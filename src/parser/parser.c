#include "../../includes/minishell.h"

static t_redir_type	get_redir_type(t_token_type type)
{
	if (type == TOKEN_REDIR_IN)
		return (REDIR_IN);
	if (type == TOKEN_REDIR_OUT)
		return (REDIR_OUT);
	if (type == TOKEN_APPEND)
		return (REDIR_APPEND);
	return (REDIR_HEREDOC);
}

static int	parse_redirection(t_token **tokens, t_cmd *cmd)
{
	t_redir	*redir;
	t_token	*operator;
	t_token	*file;

	operator = *tokens;
	file = operator->next;
	if (!file || file->type != TOKEN_WORD)
		return (0);
	redir = redir_new(get_redir_type(operator->type), file->value);
	if (!redir)
		return (0);
	redir_add_back(&cmd->redirs, redir);
	*tokens = file->next;
	return (1);
}

static int	parse_word(t_token **tokens, t_cmd *cmd)
{
	char	**new_argv;

	new_argv = argv_add(cmd->argv, (*tokens)->value);
	if (!new_argv)
		return (0);
	cmd->argv = new_argv;
	*tokens = (*tokens)->next;
	return (1);
}

static t_cmd	*parse_command(t_token **tokens)
{
	t_cmd	*cmd;

	cmd = cmd_new();
	if (!cmd)
		return (NULL);
	while (*tokens && (*tokens)->type != TOKEN_PIPE)
	{
		if ((*tokens)->type == TOKEN_WORD)
		{
			if (!parse_word(tokens, cmd))
			{
				cmd_clear(&cmd);
				return (NULL);
			}
		}
		else
		{
			if (!parse_redirection(tokens, cmd))
			{
				cmd_clear(&cmd);
				return (NULL);
			}
		}
	}
	return (cmd);
}

t_cmd	*parser(t_token *tokens)
{
	t_cmd	*commands;
	t_cmd	*cmd;

	commands = NULL;
	while (tokens)
	{
		cmd = parse_command(&tokens);
		if (!cmd)
		{
			cmd_clear(&commands);
			return (NULL);
		}
		cmd_add_back(&commands, cmd);
		if (tokens && tokens->type == TOKEN_PIPE)
			tokens = tokens->next;
	}
	return (commands);
}