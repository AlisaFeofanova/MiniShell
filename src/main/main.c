#include "../../includes/minishell.h"

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;
	char	*input;

	(void)argc;
	(void)argv;

	shell.envp = envp;
	shell.exit_status = 0;
	shell.tokens = NULL;
	shell.commands = NULL;

	while (1)
	{
		input = readline("minishell$ ");
		if (!input)
		{
			printf("exit\n");
			break ;
		}
		
		if (*input != '\0')
			add_history(input);

		shell.tokens = lexer(input);
		if (syntax_error(shell.tokens))
			printf("minishell: syntax error\n");
		else
		{
			print_tokens(shell.tokens);
			shell.commands = parser(shell.tokens);
			print_commands(shell.commands);
			cmd_clear(&shell.commands);
		}
		token_clear(&shell.tokens);

		free(input);
	}
	return (0);
}