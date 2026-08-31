# Create the Token Structure

## 1. Что такое Token?

**Token (токен)** — это отдельная смысловая часть пользовательского ввода.

Например:

```bash
echo "hello world" > output.txt
```

Lexer должен разделить эту строку на отдельные элементы:

```text
echo
"hello world"
>
output.txt
```

Каждый такой элемент становится **token**.

---

# 2. Зачем нужна Token Structure?

В Minishell данные проходят через несколько этапов:

```text
User Input
    |
    v
  Lexer
    |
    v
 Tokens
    |
    v
 Parser
    |
    v
 Command Structure
    |
    v
 Executor
```

Token Structure — это промежуточное представление между Lexer и Parser.

Lexer отвечает:

> "Что находится в строке?"

Parser отвечает:

> "Что означает эта последовательность токенов?"

---

# 3. Какие данные должен содержать Token?

Минимальная структура токена должна хранить:

1. Сам текст токена.
2. Тип токена.
3. Указатель на следующий токен.

Например:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
} t_token;
```

---

# 4. Token Type

Нужно определить типы токенов.

Например:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC
}   t_token_type;
```

Получаем:

```text
TOKEN_WORD
TOKEN_PIPE
TOKEN_REDIR_IN
TOKEN_REDIR_OUT
TOKEN_APPEND
TOKEN_HEREDOC
```

---

# 5. Что означает каждый Token Type?

## `TOKEN_WORD`

Обычный текст:

```bash
echo
hello
file.txt
$USER
"hello world"
```

Например:

```text
echo hello
```

получается:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
```

---

# 6. `TOKEN_PIPE`

Символ:

```bash
|
```

Например:

```bash
ls | wc
```

Tokens:

```text
TOKEN_WORD("ls")
TOKEN_PIPE("|")
TOKEN_WORD("wc")
```

---

# 7. `TOKEN_REDIR_IN`

Символ:

```bash
<
```

Используется для input redirection.

Например:

```bash
cat < input.txt
```

Tokens:

```text
TOKEN_WORD("cat")
TOKEN_REDIR_IN("<")
TOKEN_WORD("input.txt")
```

---

# 8. `TOKEN_REDIR_OUT`

Символ:

```bash
>
```

Например:

```bash
echo hello > output.txt
```

Tokens:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
TOKEN_REDIR_OUT(">")
TOKEN_WORD("output.txt")
```

---

# 9. `TOKEN_APPEND`

Символ:

```bash
>>
```

Используется для append redirection.

Например:

```bash
echo hello >> output.txt
```

Tokens:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
TOKEN_APPEND(">>")
TOKEN_WORD("output.txt")
```

---

# 10. `TOKEN_HEREDOC`

Символ:

```bash
<<
```

Например:

```bash
cat << EOF
hello
EOF
```

Tokens:

```text
TOKEN_WORD("cat")
TOKEN_HEREDOC("<<")
TOKEN_WORD("EOF")
```

---

# 11. Полная структура

Рекомендуемый базовый вариант:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC
}   t_token_type;

typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

---

# 12. Почему нужен `next`?

Tokens удобно хранить в linked list.

Например:

```bash
echo hello | wc -l
```

После Lexer:

```text
+-------------+
| echo        |
| WORD        |
+------+------+
       |
       v
+-------------+
| hello       |
| WORD        |
+------+------+
       |
       v
+-------------+
| |           |
| PIPE        |
+------+------+
       |
       v
+-------------+
| wc          |
| WORD        |
+------+------+
       |
       v
+-------------+
| -l          |
| WORD        |
+-------------+
```

В памяти:

```text
token1 -> token2 -> token3 -> token4 -> token5 -> NULL
```

---

# 13. Пример создания Token

Можно сделать функцию:

```c
t_token *token_new(char *value, t_token_type type)
{
    t_token *token;

    token = malloc(sizeof(t_token));
    if (!token)
        return (NULL);

    token->value = value;
    token->type = type;
    token->next = NULL;

    return (token);
}
```

Использование:

```c
t_token *token;

token = token_new(ft_strdup("echo"), TOKEN_WORD);
```

Получим:

```text
token
 |
 +-- value = "echo"
 +-- type = TOKEN_WORD
 +-- next = NULL
```

---

# 14. Добавление Token в список

Нужна функция:

```c
void token_add_back(t_token **tokens, t_token *new_token)
{
    t_token *current;

    if (!*tokens)
    {
        *tokens = new_token;
        return;
    }

    current = *tokens;
    while (current->next)
        current = current->next;

    current->next = new_token;
}
```

Теперь можно строить список:

```c
token_add_back(&tokens, token_new(ft_strdup("echo"), TOKEN_WORD));
token_add_back(&tokens, token_new(ft_strdup("hello"), TOKEN_WORD));
token_add_back(&tokens, token_new(ft_strdup("|"), TOKEN_PIPE));
token_add_back(&tokens, token_new(ft_strdup("wc"), TOKEN_WORD));
```

Получится:

```text
echo → hello → | → wc → NULL
```

---

# 15. Lexer должен создавать Tokens

Lexer получает:

```text
echo hello | wc
```

и создаёт:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
TOKEN_PIPE("|")
TOKEN_WORD("wc")
```

То есть Lexer выполняет:

```text
input
  |
  v
read characters
  |
  v
identify token
  |
  v
create token
  |
  v
add token to list
```

---

# 16. Пример Lexer

Упрощённая логика:

```c
t_token *lexer(char *input)
{
    t_token *tokens;
    int     i;

    tokens = NULL;
    i = 0;

    while (input[i])
    {
        if (input[i] == '|')
        {
            add_token(&tokens, "|", TOKEN_PIPE);
            i++;
        }
        else if (input[i] == '<')
        {
            if (input[i + 1] == '<')
            {
                add_token(&tokens, "<<", TOKEN_HEREDOC);
                i += 2;
            }
            else
            {
                add_token(&tokens, "<", TOKEN_REDIR_IN);
                i++;
            }
        }
        else if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                add_token(&tokens, ">>", TOKEN_APPEND);
                i += 2;
            }
            else
            {
                add_token(&tokens, ">", TOKEN_REDIR_OUT);
                i++;
            }
        }
        else
        {
            /*
             * Parse a WORD.
             */
        }
    }

    return (tokens);
}
```

Это только концептуальный пример.

Полноценный Lexer должен учитывать:

```text
spaces
quotes
single quotes
double quotes
operators
empty strings
environment variables
$?
syntax
```

---

# 17. Quotes и Token Structure

Очень важно:

```bash
echo "hello world"
```

не должно превращаться в:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

Правильно:

```text
WORD("echo")
WORD("hello world")
```

Потому что:

```text
"hello world"
```

является одним Shell word.

---

# 18. Quotes нельзя обрабатывать просто как обычные символы

Lexer должен понимать состояние:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

Например:

```bash
echo "hello world"
```

При чтении:

```text
echo
 ↓
NORMAL

" 
 ↓
DOUBLE_QUOTE

hello world
 ↓
DOUBLE_QUOTE

"
 ↓
NORMAL
```

Поэтому оба пробела внутри:

```text
hello world
```

не разделяют слово.

---

# 19. Operators

Shell имеет специальные операторы:

```text
|
<
>
<<
>>
```

Они должны быть отдельными tokens.

Например:

```bash
cat < file
```

не должно быть:

```text
WORD("cat")
WORD("<")
WORD("file")
```

Правильно:

```text
WORD("cat")
REDIR_IN("<")
WORD("file")
```

---

# 20. Почему `>` и `>>` должны быть разными типами?

Потому что Executor должен выполнять их по-разному.

### `>`

```bash
echo hello > file
```

Открывает файл с очисткой содержимого:

```text
O_WRONLY
O_CREAT
O_TRUNC
```

### `>>`

```bash
echo hello >> file
```

Добавляет в конец:

```text
O_WRONLY
O_CREAT
O_APPEND
```

Поэтому Parser/Executor должен знать:

```text
TOKEN_REDIR_OUT
```

или:

```text
TOKEN_APPEND
```

---

# 21. Почему `<<` отдельный Token?

Потому что:

```bash
cat << EOF
```

означает Heredoc.

Executor должен запускать совершенно другую логику:

```text
<<
 ↓
read lines
 ↓
stop at delimiter
 ↓
provide input to command
```

Поэтому:

```text
TOKEN_HEREDOC
```

обязателен.

---

# 22. Token Structure vs Command Structure

Не нужно путать Token и Command.

### Tokens

Описывают синтаксические элементы:

```text
echo
hello
|
wc
-l
```

### Command structure

Описывает уже разобранную команду:

```text
command:
    argv = ["echo", "hello"]
    pipe = true
```

Pipeline:

```bash
echo hello | wc -l
```

Tokens:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
WORD("-l")
```

Parser преобразует их примерно в:

```text
Command 1:
    argv = ["echo", "hello"]

        |

Command 2:
    argv = ["wc", "-l"]
```

---

# 23. Token List перед Parser

Parser должен получать примерно:

```text
TOKEN_WORD("echo")
        |
        v
TOKEN_WORD("hello")
        |
        v
TOKEN_PIPE("|")
        |
        v
TOKEN_WORD("wc")
        |
        v
TOKEN_WORD("-l")
```

После этого Parser может определить:

```text
command 1
    |
    +-- echo
    +-- hello
    |
    +-- PIPE
    |
command 2
    |
    +-- wc
    +-- -l
```

---

# 24. Syntax Validation с Tokens

Token Structure также необходима для проверки syntax.

Например:

```bash
echo hello |
```

Tokens:

```text
WORD("echo")
WORD("hello")
PIPE("|")
```

Parser видит:

```text
PIPE
 ↓
ничего после него
```

Это syntax error.

---

## Другой пример

```bash
echo > 
```

Tokens:

```text
WORD("echo")
REDIR_OUT(">")
```

После `>` должен быть WORD.

Но его нет.

Поэтому:

```text
syntax error
```

---

# 25. Invalid Pipe

Например:

```bash
| echo
```

Tokens:

```text
PIPE("|")
WORD("echo")
```

Parser может определить:

```text
PIPE находится в начале
```

→ syntax error.

---

# 26. Invalid Redirection

```bash
echo > | cat
```

Tokens:

```text
WORD("echo")
REDIR_OUT(">")
PIPE("|")
WORD("cat")
```

После:

```text
>
```

ожидается:

```text
WORD
```

Но получен:

```text
PIPE
```

→ syntax error.

---

# 27. Token Types и Parser Rules

Parser может использовать правила:

```text
WORD
WORD
WORD
```

→ valid command.

```text
WORD PIPE WORD
```

→ valid pipeline.

```text
WORD REDIR_OUT WORD
```

→ valid redirection.

```text
WORD REDIR_IN WORD
```

→ valid input redirection.

```text
WORD HEREDOC WORD
```

→ valid heredoc.

---

# 28. Token Cleanup

Так как tokens создаются через `malloc`, их нужно освобождать.

Например:

```c
void free_tokens(t_token *tokens)
{
    t_token *next;

    while (tokens)
    {
        next = tokens->next;
        free(tokens->value);
        free(tokens);
        tokens = next;
    }
}
```

Важно:

```text
malloc token
malloc value
```

означает:

```text
free value
free token
```

---

# 29. Memory Ownership

Нужно заранее определить:

> Кто владеет `value`?

Хороший вариант:

```text
Token owns value
```

То есть:

```c
token->value
```

выделяется отдельно и освобождается вместе с token.

Например:

```c
token = malloc(sizeof(t_token));
token->value = ft_strdup("hello");
```

При удалении:

```c
free(token->value);
free(token);
```

---

# 30. Рекомендуемый Header

Можно создать:

```text
includes/
    minishell.h
    token.h
```

Например:

```c
#ifndef TOKEN_H
# define TOKEN_H

typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC
}   t_token_type;

typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;

t_token *token_new(char *value, t_token_type type);
void    token_add_back(t_token **tokens, t_token *new_token);
void    free_tokens(t_token *tokens);

#endif
```

---

# 31. Лучше ли хранить Token Type как enum?

Да.

Вместо:

```c
int type;
```

лучше:

```c
t_token_type type;
```

Потому что код становится понятнее:

```c
if (token->type == TOKEN_PIPE)
```

вместо:

```c
if (token->type == 2)
```

---

# 32. Возможное расширение структуры

На начальном этапе достаточно:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

Но при более сложном Lexer можно добавить информацию:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    int             quoted;
    struct s_token  *next;
}   t_token;
```

Например:

```text
quoted = 1
```

может означать, что token был сформирован из quoted text.

Однако **не добавляйте поля без необходимости**. Сначала реализуйте минимальную структуру.

---

# 33. Tokenization Example

Input:

```bash
echo "hello world" > output.txt | cat
```

Lexer должен получить:

```text
+--------------------+
| echo               |
| TOKEN_WORD         |
+--------------------+
          |
          v
+--------------------+
| hello world        |
| TOKEN_WORD         |
+--------------------+
          |
          v
+--------------------+
| >                  |
| TOKEN_REDIR_OUT    |
+--------------------+
          |
          v
+--------------------+
| output.txt         |
| TOKEN_WORD         |
+--------------------+
          |
          v
+--------------------+
| |                  |
| TOKEN_PIPE         |
+--------------------+
          |
          v
+--------------------+
| cat                |
| TOKEN_WORD         |
+--------------------+
```

---

# 34. Full Lexer → Parser Flow

```text
                USER INPUT
                    |
                    v
        ┌─────────────────────┐
        │        LEXER        │
        └──────────┬──────────┘
                   |
                   v
             TOKEN LIST
                   |
                   v
        ┌─────────────────────┐
        │       PARSER       │
        └──────────┬──────────┘
                   |
                   v
          COMMAND STRUCTURE
                   |
                   v
        ┌─────────────────────┐
        │      EXECUTOR      │
        └─────────────────────┘
```

---

# 35. Implementation Order

Для команды из двух человек лучше реализовывать Token Structure в таком порядке.

## Step 1 — Define enum

Создать:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC
}   t_token_type;
```

---

## Step 2 — Define struct

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

---

## Step 3 — Implement constructor

```c
t_token *token_new(char *value, t_token_type type);
```

Проверить:

```text
malloc
value
type
next
```

---

## Step 4 — Implement append

```c
void token_add_back(t_token **tokens, t_token *new_token);
```

---

## Step 5 — Implement cleanup

```c
void free_tokens(t_token *tokens);
```

Проверить:

```text
no leaks
no double free
```

---

## Step 6 — Create debug function

Очень полезно иметь:

```c
void print_tokens(t_token *tokens);
```

Например:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
[WORD] -l
```

Это сильно упростит debugging Lexer.

---

# 36. Debug Function

Пример:

```c
void print_tokens(t_token *tokens)
{
    while (tokens)
    {
        printf("TYPE=%d VALUE=[%s]\n",
            tokens->type,
            tokens->value);
        tokens = tokens->next;
    }
}
```

Лучше сделать красивое отображение типов:

```text
TYPE=WORD
TYPE=PIPE
TYPE=REDIR_IN
TYPE=REDIR_OUT
TYPE=APPEND
TYPE=HEREDOC
```

---

# 37. Test Cases

## Test 1

Input:

```bash
echo hello
```

Expected:

```text
WORD: echo
WORD: hello
```

---

## Test 2

Input:

```bash
echo hello | wc
```

Expected:

```text
WORD: echo
WORD: hello
PIPE: |
WORD: wc
```

---

## Test 3

Input:

```bash
cat < input.txt
```

Expected:

```text
WORD: cat
REDIR_IN: <
WORD: input.txt
```

---

## Test 4

Input:

```bash
echo hello > output.txt
```

Expected:

```text
WORD: echo
WORD: hello
REDIR_OUT: >
WORD: output.txt
```

---

## Test 5

Input:

```bash
echo hello >> output.txt
```

Expected:

```text
WORD: echo
WORD: hello
APPEND: >>
WORD: output.txt
```

---

## Test 6

Input:

```bash
cat << EOF
```

Expected:

```text
WORD: cat
HEREDOC: <<
WORD: EOF
```

---

## Test 7

Input:

```bash
echo "hello world"
```

Expected:

```text
WORD: echo
WORD: hello world
```

---

## Test 8

Input:

```bash
echo '$USER'
```

Token should preserve:

```text
$USER
```

The decision about whether expansion happens belongs to later processing according to your architecture, but the lexer must preserve the syntactic information needed for correct handling.

---

# 38. Important Design Principle

Не смешивайте эти этапы:

```text
LEXING
PARSING
EXPANSION
EXECUTION
```

Lexer:

```text
"Что это?"
```

Parser:

```text
"Как эти tokens связаны?"
```

Expansion:

```text
"Во что превращаются $VAR и $??"
```

Executor:

```text
"Что реально нужно выполнить?"
```

---

# 39. Minimal Token API

Для начала вам достаточно:

```c
t_token *token_new(char *value, t_token_type type);

void    token_add_back(
            t_token **tokens,
            t_token *new_token
        );

void    free_tokens(t_token *tokens);

void    print_tokens(t_token *tokens);
```

---

# 40. Final Architecture

После выполнения этого этапа у вас должно быть:

```text
                   INPUT
                     |
                     v
              ┌─────────────┐
              │    LEXER    │
              └──────┬──────┘
                     |
                     v
              ┌─────────────┐
              │  t_token    │
              │             │
              │ value       │
              │ type        │
              │ next        │
              └──────┬──────┘
                     |
                     v
              ┌─────────────┐
              │   PARSER    │
              └──────┬──────┘
                     |
                     v
              COMMAND LIST
                     |
                     v
              ┌─────────────┐
              │  EXECUTOR   │
              └─────────────┘
```

---

# 41. Final Checklist

### Token Structure

* [ ] Создан `t_token_type`.
* [ ] Создан `t_token`.
* [ ] `value` хранит текст token.
* [ ] `type` хранит тип token.
* [ ] `next` связывает tokens.

### Token Types

* [ ] `TOKEN_WORD`
* [ ] `TOKEN_PIPE`
* [ ] `TOKEN_REDIR_IN`
* [ ] `TOKEN_REDIR_OUT`
* [ ] `TOKEN_APPEND`
* [ ] `TOKEN_HEREDOC`

### Functions

* [ ] `token_new()`
* [ ] `token_add_back()`
* [ ] `free_tokens()`
* [ ] `print_tokens()`

### Lexer

* [ ] Words превращаются в `TOKEN_WORD`.
* [ ] `|` превращается в `TOKEN_PIPE`.
* [ ] `<` превращается в `TOKEN_REDIR_IN`.
* [ ] `>` превращается в `TOKEN_REDIR_OUT`.
* [ ] `>>` превращается в `TOKEN_APPEND`.
* [ ] `<<` превращается в `TOKEN_HEREDOC`.
* [ ] Quotes учитываются при определении границ WORD.
* [ ] Token list корректно передаётся Parser.

### Memory

* [ ] Каждый token освобождается.
* [ ] Каждый `value` освобождается.
* [ ] Нет memory leaks.
* [ ] Нет double free.
* [ ] Нет use-after-free.

---

# Главное, что нужно понять

Token — это **не команда**.

Token — это элемент синтаксиса.

Например:

```bash
echo hello > file | cat
```

становится:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("cat")
```

А уже **Parser** превращает эту последовательность в структуру команд.

Поэтому правильная архитектура:

```text
                    RAW INPUT
                       |
                       v
                    LEXER
                       |
                       v
              ┌────────────────┐
              │  TOKEN LIST    │
              ├────────────────┤
              │ WORD           │
              │ WORD           │
              │ REDIRECTION    │
              │ WORD           │
              │ PIPE           │
              │ WORD           │
              └───────┬────────┘
                      |
                      v
                    PARSER
                      |
                      v
               COMMAND LIST
                      |
                      v
                   EXECUTOR
```

**Главная задача этого этапа:** создать надёжную структуру `t_token`, чтобы Lexer мог представить пользовательский input в виде последовательности понятных Parser элементов.
