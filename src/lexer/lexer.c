#include "../../includes/minishell.h"

static t_token	*read_word(char *input, int *i)
{
	int		start;
	int		end;
	char	*word;
	t_token	*token;

	start = *i;
	end = word_end(input, *i);
	word = ft_substr(input, start, end - start);
	if (!word)
		return (NULL);
	token = token_new(word, TOKEN_WORD);
	free(word);
	if (!token)
		return (NULL);
	*i = end;
	return (token);
}

static t_token	*read_operator(char *input, int *i)
{
	char	*value;
	t_token	*token;
	t_token_type	type;

	if (input[*i] == '|')
		type = TOKEN_PIPE;
	else if (input[*i] == '<' && input[*i + 1] == '<')
		type = TOKEN_HEREDOC;
	else if (input[*i] == '>' && input[*i + 1] == '>')
		type = TOKEN_APPEND;
	else if (input[*i] == '<')
		type = TOKEN_REDIR_IN;
	else
		type = TOKEN_REDIR_OUT;
	
	if (is_double_operator(input, *i))
	{
		value = ft_substr(input, *i, 2);
		*i += 2;
	}
	else
	{
		value = ft_substr(input, *i, 1);
		*i += 1;
	}
	if (!value)
		return (NULL);
	token = token_new(value, type);
	free(value);
	return (token);
}

t_token	*lexer(char *input)
{
	t_token	*tokens;
	t_token	*token;
	int		i;

	tokens = NULL;
	i = 0;
	while (input[i])
	{
		skip_spaces(input, &i);
		if (!input[i])
			break ;
		if (is_operator(input[i]))
			token = read_operator(input, &i);
		else
			token = read_word(input, &i);
		if (!token)
		{
			token_clear(&tokens);
			return (NULL);
		}
		token_add_back(&tokens, token);
	}
	return (tokens);
}