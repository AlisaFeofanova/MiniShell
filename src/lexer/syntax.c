#include "../../includes/minishell.h"

int	syntax_error(t_token *tokens)
{
	if (!tokens)
		return (0);
	if (tokens->type == TOKEN_PIPE)
		return (1);
	while (tokens)
	{
		if (tokens->type == TOKEN_PIPE)
		{
			if (!tokens->next)
				return (1);
			if (tokens->next->type == TOKEN_PIPE)
				return (1);
		}
		if (tokens->type != TOKEN_WORD
			&& tokens->type != TOKEN_PIPE)
		{
			if (!tokens->next)
				return (1);
			if (tokens->next->type != TOKEN_WORD)
				return (1);
		}
		tokens = tokens->next;
	}
	return (0);
}