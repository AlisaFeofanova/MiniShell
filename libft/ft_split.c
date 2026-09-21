/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alfeofan <alfeofan@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 17:01:44 by alfeofan          #+#    #+#             */
/*   Updated: 2025/10/17 17:58:09 by alfeofan         ###   ####lausanne.ch   */
/*                                                                            */
/* ************************************************************************** */
#include "libft.h"

static int	word_count(const char *s, char c)
{
	int	count;
	int	in_word;

	count = 0;
	in_word = 0;
	while (*s)
	{
		if (*s != c && !in_word)
		{
			in_word = 1;
			count++;
		}
		else if (*s == c)
			in_word = 0;
		s++;
	}
	return (count);
}

static char	*fill_word(const char *str, int start, int end)
{
	char	*word;

	word = malloc((end - start + 1) * sizeof(char));
	if (!word)
		return (NULL);
	ft_strlcpy(word, str + start, end - start + 1);
	return (word);
}

static void	*ft_free(char **strs, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		free(strs[i]);
		i++;
	}
	free(strs);
	return (NULL);
}

static int	word_len(char const *str, char c)
{
	int	len;

	len = 0;
	while (str[len] == c)
		len++;
	while (str[len] && str[len] != c)
		len++;
	return (len);
}

char	**ft_split(char const *s, char c)
{
	char	**result;
	int		j;

	if (!s)
		return (NULL);
	result = malloc((word_count(s, c) + 1) * sizeof(char *));
	if (!result)
		return (NULL);
	j = 0;
	while (*s)
	{
		while (*s == c)
			s++;
		if (word_len(s, c) > 0)
		{
			result[j] = fill_word(s, 0, word_len(s, c));
			if (!(result[j]))
				return (ft_free(result, j));
			j++;
			s += word_len(s, c);
		}
	}
	result[j] = NULL;
	return (result);
}
