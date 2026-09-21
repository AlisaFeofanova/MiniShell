#include "../../includes/minishell.h"

int	is_space(char c)
{
	return (c == ' ' || c == '\t');
}

int	is_operator(char c)
{
	return (c == '|' || c == '<' || c == '>');
}

int	is_quote(char c)
{
	return (c == '\'' || c == '"');
}

int	is_pipe(char c)
{
	return (c == '|');
}

int	is_redirection(char c)
{
	return (c == '<' || c == '>');
}