#include "../../includes/minishell.h"

int	is_double_operator(char *input, int i)
{
	if ((input[i] == '<' && input[i + 1] == '<')
		|| (input[i] == '>' && input[i + 1] == '>'))
		return (1);
	return (0);
}

void	skip_spaces(char *input, int *i)
{
	while (input[*i] && is_space(input[*i]))
		(*i)++;
}

int	word_end(char *input, int i)
{
	char	quote;

	while (input[i])
	{
		if (is_quote(input[i]))
		{
			quote = input[i];
			i++;
			while (input[i] && input[i] != quote)
				i++;
			if (input[i])
				i++;
		}
		else if (is_space(input[i]) || is_operator(input[i]))
			break ;
		else
			i++;
	}
	return (i);
}

int	has_unclosed_quote(char *input)
{
	int		i;
	char	quote;

	i = 0;
	while (input[i])
	{
		if (is_quote(input[i]))
		{
			quote = input[i];
			i++;
			while (input[i] && input[i] != quote)
				i++;
			if (!input[i])
				return (1);
			i++;
		}
		else
			i++;
	}
	return (0);
}