#ifndef MINISHELL_H
# define MINISHELL_H

# include "../libft/libft.h"
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <readline/readline.h>
# include <readline/history.h>

typedef enum e_token_type
{
	TOKEN_WORD,
	TOKEN_PIPE,
	TOKEN_REDIR_IN,
	TOKEN_REDIR_OUT,
	TOKEN_APPEND,
	TOKEN_HEREDOC
}	t_token_type;

typedef struct s_token
{
	char			*value;
	t_token_type	type;
	struct s_token	*next;
}	t_token;

/* ==================== t_cmd ==================== */

typedef enum e_redir_type
{
	REDIR_IN,
	REDIR_OUT,
	REDIR_APPEND,
	REDIR_HEREDOC
}	t_redir_type;

typedef struct s_redir
{
	t_redir_type		type;
	char				*file;
	struct s_redir		*next;
}	t_redir;

typedef struct s_cmd
{
	char			**argv;
	t_redir			*redirs;
	struct s_cmd	*next;
}	t_cmd;




typedef struct s_shell
{
	char		**envp;
	int			exit_status;
	t_token		*tokens;
	t_cmd		*commands;
}	t_shell;

/* ==================== LEXER ==================== */

// lexer_is.c
int		is_space(char c);
int		is_operator(char c);
int		is_quote(char c);
int		is_pipe(char c);
int		is_redirection(char c);

// lexer_utils.c
int     is_double_operator(char *input, int i);
void	skip_spaces(char *input, int *i);
int     word_end(char *input, int i);
int		has_unclosed_quote(char *input);

// syntax.c
int		syntax_error(t_token *tokens);

// token.c
t_token	*token_new(char *value, t_token_type type);

// token_utils.c
void	token_add_back(t_token **tokens, t_token *new_token);
void	token_clear(t_token **tokens);

// lexer.c
t_token	*lexer(char *input);

/* ==================== PARSER ==================== */

// parser_utils.c
t_redir	*redir_new(t_redir_type type, char *file);
void	redir_add_back(t_redir **redirs, t_redir *new_redir);
void	redir_clear(t_redir **redirs);
char	**argv_add(char **argv, char *value);

// command.c
t_cmd	*cmd_new(void);
void	cmd_add_back(t_cmd **cmds, t_cmd *new_cmd);
void	cmd_clear(t_cmd **cmds);

// parser.c
t_cmd	*parser(t_token *tokens);

/* ==================== EXPANSION ==================== */

char	*expand_variables(char *word, t_shell *shell);

/* ==================== UTILS ==================== */

void	free_shell(t_shell *shell);
void	print_tokens(t_token *tokens);
void	print_commands(t_cmd *cmds);

#endif