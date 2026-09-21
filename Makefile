NAME = minishell

CC = cc
CFLAGS = -Wall -Wextra -Werror

LIBFT_DIR = libft
LIBFT = $(LIBFT_DIR)/libft.a

INCLUDES = -I./includes -I$(LIBFT_DIR)

LDFLAGS = -lreadline

SRC = \
	src/main/main.c \
	src/lexer/lexer.c \
	src/lexer/lexer_is.c \
	src/lexer/lexer_utils.c \
	src/lexer/token.c \
	src/lexer/token_utils.c \
	src/lexer/syntax.c \
	src/parser/parser.c \
	src/parser/command.c \
	src/parser/parser_utils.c \
	src/expansion/expansion.c \
	src/expansion/expansion_utils.c \
	src/utils/utils.c

OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(LIBFT) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBFT) $(LDFLAGS) -o $(NAME)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ)
	$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	rm -f $(NAME)
	$(MAKE) -C $(LIBFT_DIR) fclean

re: fclean all

.PHONY: all clean fclean re