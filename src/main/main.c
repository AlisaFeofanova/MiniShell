/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ch <chguerre@42lausanne.ch>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 15:28:25 by ch                #+#    #+#             */
/*   Updated: 2026/09/04 16:46:49 by ch               ###   ########.ch       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"
void	alse_parser(t_cmd *cmd);

int	main(int argc, char **argv, char **envp)
{
	(void)argc;
	(void)argv;
	t_cmd	cmd;

	false_parser(cmd);
	executor (cmd, envp);
	return (0);
}