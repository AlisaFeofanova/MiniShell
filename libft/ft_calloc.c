/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_calloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alfeofan <alfeofan@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 18:40:02 by alfeofan          #+#    #+#             */
/*   Updated: 2025/10/17 17:25:04 by alfeofan         ###   ####lausanne.ch   */
/*                                                                            */
/* ************************************************************************** */
#include "libft.h"

static unsigned char	*malloc_check(unsigned char *ptr)
{
	if (!ptr)
	{
		return (NULL);
	}
	return (ptr);
}

void	*ft_calloc(size_t count, size_t size)
{
	size_t			total_size;
	unsigned char	*ptr;
	size_t			i;

	if (count == 0 || size == 0)
	{
		ptr = malloc(0);
		return (malloc_check(ptr));
	}
	total_size = count * size;
	ptr = malloc(total_size);
	if (!ptr)
		return (malloc_check(ptr));
	i = 0;
	while (i < total_size)
		ptr[i++] = 0;
	return ((void *)ptr);
}
