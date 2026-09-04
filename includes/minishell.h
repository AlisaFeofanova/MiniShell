/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ch <chguerre@42lausanne.ch>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 15:19:56 by ch                #+#    #+#             */
/*   Updated: 2026/09/04 15:26:04 by ch               ###   ########.ch       */
/*                                                                            */
/* ************************************************************************** */

#if !defined(MINISHELL_H)
# define MINISHELL_H

# include <unistd.h>
# include <stdlib.h>
# include <stdio.h>
# include <sys/wait.h>
# include <errno.h>

typedef struct s_redir
{
	int				type;
	char			*target;
	struct s_redir	*next;
}	t_redir;

typedef struct s_cmd
{
	char				*args;
	char				*path;
	t_redir				*redir;
	struct s_command	*next;

}	t_cmd;

#endif // MINISHELL_H
