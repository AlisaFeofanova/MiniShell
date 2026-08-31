# Создание базовых структур Parser

## Содержание

* [1. Цель задачи](#1-цель-задачи)
* [2. Что такое Parser](#2-что-такое-parser)
* [3. Lexer vs Parser](#3-lexer-vs-parser)
* [4. Задачи Parser](#4-задачи-parser)
* [5. Зачем нужны структуры Parser](#5-зачем-нужны-структуры-parser)
* [6. Базовая грамматика Shell](#6-базовая-грамматика-shell)
* [7. Структура команды](#7-структура-команды)
* [8. Redirections — перенаправления](#8-redirections--перенаправления)
* [9. Pipes — конвейеры](#9-pipes--конвейеры)
* [10. AST](#10-ast)
* [11. Базовые структуры данных](#11-базовые-структуры-данных)
* [12. Структура Token](#12-структура-token)
* [13. Структура AST Node](#13-структура-ast-node)
* [14. Контекст Parser](#14-контекст-parser)
* [15. Функции Parser](#15-функции-parser)
* [16. Парсинг простой команды](#16-парсинг-простой-команды)
* [17. Парсинг Redirection](#17-парсинг-redirection)
* [18. Парсинг Pipe](#18-парсинг-pipe)
* [19. Пример AST](#19-пример-ast)
* [20. Обработка ошибок](#20-обработка-ошибок)
* [21. Частые ошибки](#21-частые-ошибки)
* [22. Тестирование](#22-тестирование)
* [23. Финальный чек-лист](#23-финальный-чек-лист)
* [24. Общая архитектура](#24-общая-архитектура)

---

# 1. Цель задачи

Задача:

> **Create Basic Parser Structures**

означает создание структур данных и базовых функций, которые позволят `Minishell` преобразовать список токенов, созданный Lexer, в структуру, описывающую команду.

Общий pipeline:

```text
Input
  ↓
Lexer
  ↓
Tokens
  ↓
Parser
  ↓
Command / AST
  ↓
Expansion
  ↓
Execution
```

Например:

```bash
echo hello
```

Lexer создаёт:

```text
WORD("echo")
WORD("hello")
```

Parser должен преобразовать это в структуру:

```text
COMMAND
├── command: echo
└── argument: hello
```

---

# 2. Что такое Parser

**Parser** получает токены от Lexer и определяет их грамматический смысл.

Можно запомнить так:

### Lexer отвечает:

> "Какие части есть во входной строке?"

### Parser отвечает:

> "Как эти части связаны между собой?"

Например:

```bash
echo hello | grep h
```

Lexer видит:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("h")
```

Parser понимает:

```text
        PIPE
       /    \
   COMMAND COMMAND
    /  \     /  \
  echo hello grep h
```

То есть Parser превращает список отдельных токенов в структуру команды.

---

# 3. Lexer vs Parser

Очень важно не смешивать обязанности Lexer и Parser.

## Lexer

Input:

```bash
echo hello > file | grep h
```

Output:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("grep")
WORD("h")
```

Lexer определяет **токены**.

---

## Parser

Parser получает:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("grep")
WORD("h")
```

и строит:

```text
PIPE
├── COMMAND
│   ├── echo
│   ├── hello
│   └── > file
│
└── COMMAND
    ├── grep
    └── h
```

Parser определяет **структуру**.

---

# 4. Задачи Parser

Базовый Parser для Minishell должен уметь:

* [ ] Распознавать команды.
* [ ] Распознавать аргументы.
* [ ] Распознавать Pipe `|`.
* [ ] Распознавать Redirections.
* [ ] Связывать оператор перенаправления с его аргументом.
* [ ] Создавать структуры команд.
* [ ] Обнаруживать базовые syntax errors.
* [ ] Сохранять порядок команд.
* [ ] Подготавливать данные для Executor.

Parser **не должен выполнять команды**.

Например:

```bash
ls -la
```

Parser должен создать:

```text
command = ls
arguments = [-la]
```

Но Parser не должен вызывать:

```c
execve()
```

---

# 5. Зачем нужны структуры Parser

Без структуры команд Executor будет сложно понять, что именно нужно выполнить.

Например:

```bash
cat file.txt | grep hello > result.txt
```

Executor должен понимать:

```text
Command 1:
    program = cat
    argument = file.txt

Pipe

Command 2:
    program = grep
    argument = hello
    output = result.txt
```

Parser создаёт эту информацию.

---

# 6. Базовая грамматика Shell

Упрощённо грамматику Minishell можно представить так:

```text
input
    → pipeline

pipeline
    → command
    → command PIPE pipeline

command
    → words
    → words redirections

redirection
    → REDIR_IN WORD
    → REDIR_OUT WORD
    → APPEND WORD
    → HEREDOC WORD
```

Ещё проще:

```text
pipeline
    |
    +── command
    |
    +── PIPE
    |
    +── command
```

---

# 7. Структура команды

Команда может содержать:

```text
COMMAND
├── имя команды
├── аргументы
└── redirections
```

Например:

```bash
grep hello file.txt
```

Получаем:

```text
COMMAND
├── command = grep
├── arg = hello
└── arg = file.txt
```

---

## 7.1 Пример

Input:

```bash
ls -la /tmp
```

Структура:

```text
COMMAND
├── argv[0] = "ls"
├── argv[1] = "-la"
└── argv[2] = "/tmp"
```

Это уже очень близко к тому, что в дальнейшем понадобится Executor.

---

# 8. Redirections — перенаправления

В Minishell используются четыре основных оператора:

```text
<    input
>    output
>>   append
<<   heredoc
```

Примеры:

```bash
cat < input.txt
```

```bash
echo hello > output.txt
```

```bash
echo hello >> output.txt
```

```bash
cat << EOF
hello
EOF
```

Parser должен связать оператор с его target.

---

# 8.1 Структура Redirection

Для:

```bash
echo hello > output.txt
```

можно создать:

```text
COMMAND
├── argv
│   ├── echo
│   └── hello
│
└── REDIRECTION
    ├── type = OUTPUT
    └── target = output.txt
```

---

# 8.2 Несколько Redirections

Input:

```bash
cat < input.txt > output.txt
```

Структура:

```text
COMMAND
├── cat
│
├── REDIR_IN
│   └── input.txt
│
└── REDIR_OUT
    └── output.txt
```

У одной команды может быть несколько redirections.

---

# 9. Pipes — конвейеры

Pipe:

```text
|
```

соединяет `stdout` одной команды с `stdin` другой.

Например:

```bash
ls | grep .c
```

Концептуально:

```text
COMMAND(ls)
      |
     PIPE
      |
COMMAND(grep .c)
```

---

# 9.1 Несколько Pipes

Input:

```bash
cat file | grep hello | wc -l
```

Структура:

```text
        PIPE
       /    \
    PIPE     COMMAND
   /    \      |
COMMAND COMMAND wc -l
  |       |
 cat     grep
 file    hello
```

Также можно представить pipeline как список:

```text
COMMAND → COMMAND → COMMAND
   cat      grep       wc
```

с pipe-связью между ними.

Какой вариант выбрать — зависит от архитектуры вашего проекта.

---

# 10. AST

**AST** означает:

> Abstract Syntax Tree

или:

> Абстрактное синтаксическое дерево.

AST показывает грамматическую структуру команды.

Для:

```bash
echo hello | grep hello
```

можно построить:

```text
          PIPE
         /    \
     COMMAND  COMMAND
      /   \     /   \
    echo hello grep hello
```

Корневой элемент:

```text
PIPE
```

потому что весь input представляет собой pipeline.

---

# 10.1 Зачем нужен AST?

AST удобно использовать для сложных команд.

Например:

```bash
cat < input.txt | grep hello > output.txt
```

Можно представить:

```text
             PIPE
            /    \
       COMMAND   COMMAND
       /    \      /   \
     cat    <     grep  >
             |          |
        input.txt   output.txt
                   +
                  hello
```

Executor может затем обходить эту структуру и выполнять её.

---

# 11. Базовые структуры данных

Один из возможных вариантов архитектуры:

```text
t_token
t_redir
t_command
t_ast
```

Например:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
}   t_token;
```

---

# 11.1 Структура Redirection

```c
typedef struct s_redir
{
    int             type;
    char            *target;
    struct s_redir  *next;
}   t_redir;
```

Возможные типы:

```c
#define REDIR_IN   1
#define REDIR_OUT  2
#define APPEND     3
#define HEREDOC    4
```

---

# 11.2 Структура Command

Простая структура:

```c
typedef struct s_command
{
    char                **argv;
    t_redir             *redirs;
    struct s_command    *next;
}   t_command;
```

Для:

```bash
echo hello | grep hello
```

можно получить:

```text
command1
    argv = ["echo", "hello"]
    next → command2

command2
    argv = ["grep", "hello"]
    next = NULL
```

---

# 11.3 Структура AST

Если используется AST:

```c
typedef struct s_ast
{
    int             type;
    struct s_ast    *left;
    struct s_ast    *right;
    t_command       *command;
}   t_ast;
```

Типы:

```c
#define AST_COMMAND 1
#define AST_PIPE    2
```

Для:

```bash
echo hello | grep hello
```

получаем:

```text
        AST_PIPE
        /      \
 AST_COMMAND  AST_COMMAND
```

---

# 12. Структура Token

Parser зависит от корректной структуры Token.

Например:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
}   t_token;
```

Для:

```bash
echo hello > file
```

список токенов:

```text
+-----------+
| WORD      |
| echo      |
+-----------+
      |
      v
+-----------+
| WORD      |
| hello     |
+-----------+
      |
      v
+-----------+
| REDIR_OUT |
| >         |
+-----------+
      |
      v
+-----------+
| WORD      |
| file      |
+-----------+
```

---

# 12.1 Типы Token

Можно использовать enum:

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

Теперь вместо:

```c
if (!ft_strcmp(token->value, "|"))
```

можно использовать:

```c
if (token->type == TOKEN_PIPE)
```

Это делает код Parser значительно понятнее.

---

# 13. Структура AST Node

Базовый AST node может содержать:

```text
type
left
right
command
```

Например:

```c
typedef struct s_ast
{
    t_token         *token;
    t_command       *command;
    struct s_ast    *left;
    struct s_ast    *right;
}   t_ast;
```

Однако не нужно добавлять поля просто "на всякий случай".

Структуры должны соответствовать архитектуре вашего Executor.

---

# 13.1 Пример AST

Input:

```bash
echo hello | wc -c
```

AST:

```text
             PIPE
            /    \
       COMMAND  COMMAND
        /   \      /   \
      echo hello  wc   -c
```

---

# 14. Контекст Parser

Удобно создать отдельную структуру Parser:

```c
typedef struct s_parser
{
    t_token *current;
    t_ast   *root;
}   t_parser;
```

Например:

```c
t_parser parser;

parser.current = tokens;
parser.root = NULL;
```

---

# 14.1 Зачем нужен `current`?

`current` показывает, на каком токене сейчас находится Parser.

Например:

```text
WORD("echo")
      ↓
WORD("hello")
      ↓
PIPE
      ↓
WORD("grep")
```

Parser может перемещаться:

```c
parser.current = parser.current->next;
```

Это особенно удобно при рекурсивном Parser.

---

# 15. Функции Parser

Parser можно разделить на несколько функций:

```c
t_ast       *parse_input(t_token *tokens);
t_ast       *parse_pipeline(t_parser *parser);
t_ast       *parse_command(t_parser *parser);
t_redir     *parse_redirection(t_parser *parser);
int         is_redirection(int type);
int         is_command_end(int type);
```

---

# 15.1 Ответственность функций

### `parse_input()`

Начинает Parsing:

```text
input
 ↓
pipeline
```

---

### `parse_pipeline()`

Обрабатывает:

```text
command | command | command
```

---

### `parse_command()`

Обрабатывает:

```text
command arguments redirections
```

---

### `parse_redirection()`

Обрабатывает:

```text
< file
> file
>> file
<< EOF
```

---

# 16. Парсинг простой команды

Input:

```bash
echo hello
```

Tokens:

```text
WORD("echo")
WORD("hello")
```

Parser начинает с:

```text
WORD("echo")
```

Создаёт Command.

Затем:

```text
WORD("hello")
```

добавляется в `argv`.

Итог:

```text
COMMAND
├── argv[0] = "echo"
└── argv[1] = "hello"
```

---

# 16.1 Базовый Pseudocode

```text
parse_command():

    create command

    while current token is WORD:

        add token value to argv

        move to next token

    return command
```

Но этого недостаточно для redirections.

---

# 17. Парсинг Redirection

Input:

```bash
echo hello > file
```

Tokens:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

Parser:

```text
WORD
  ↓
argv

WORD
  ↓
argv

REDIR_OUT
  ↓
expect WORD
  ↓
create redirection
```

Результат:

```text
COMMAND
├── argv
│   ├── echo
│   └── hello
│
└── redirs
    └── OUTPUT → file
```

---

# 17.1 Pseudocode

```text
parse_command():

    create command

    while current token is not PIPE and not EOF:

        if current token is WORD:
            add to argv

        else if current token is REDIRECTION:
            parse redirection

        else:
            syntax error

    return command
```

---

# 17.2 Парсинг одной Redirection

```text
parse_redirection():

    save current operator

    move to next token

    if current token is not WORD:
        syntax error

    create redirection

    redirection.type = operator.type
    redirection.target = current.value

    move to next token
```

Для:

```bash
echo > output.txt
```

Parser видит:

```text
REDIR_OUT
    ↓
WORD("output.txt")
```

и создаёт:

```text
type = REDIR_OUT
target = "output.txt"
```

---

# 18. Парсинг Pipe

Input:

```bash
echo hello | grep hello
```

Tokens:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

Сначала Parser создаёт:

```text
COMMAND
├── echo
└── hello
```

Затем встречает:

```text
PIPE
```

Это означает:

> Текущая команда закончилась, и после Pipe должна быть следующая команда.

После этого Parser создаёт:

```text
COMMAND
├── grep
└── hello
```

И соединяет их:

```text
          PIPE
         /    \
    COMMAND  COMMAND
     /   \    /   \
   echo hello grep hello
```

---

# 18.1 Правила Pipe

Корректная структура:

```text
command | command
```

Некорректно:

```bash
| echo
```

Некорректно:

```bash
echo |
```

Некорректно:

```bash
echo | | cat
```

Parser должен обнаруживать такие ошибки.

---

# 18.2 Pseudocode

```text
parse_pipeline():

    left = parse_command()

    while current token is PIPE:

        consume PIPE

        if next token cannot start a command:
            syntax error

        right = parse_command()

        create PIPE node

        pipe.left = left
        pipe.right = right

        left = pipe

    return left
```

Для:

```bash
a | b | c
```

получится:

```text
        PIPE
       /    \
     PIPE    c
    /   \
   a     b
```

---

# 19. Пример AST

Рассмотрим:

```bash
cat input.txt | grep hello > output.txt
```

Tokens:

```text
WORD("cat")
WORD("input.txt")
PIPE
WORD("grep")
WORD("hello")
REDIR_OUT
WORD("output.txt")
```

AST:

```text
                 PIPE
                /    \
               /      \
          COMMAND    COMMAND
          /    \       /    \
        cat  input.txt grep  hello
                         |
                       REDIR
                         |
                     output.txt
```

---

# 19.1 Представление первой команды

```text
argv:
    ["cat", "input.txt"]

redirs:
    none
```

Вторая команда:

```text
argv:
    ["grep", "hello"]

redirs:
    OUTPUT → "output.txt"
```

---

# 19.2 Ещё один пример

Input:

```bash
< input.txt cat | grep hello >> result.txt
```

Первая команда:

```text
COMMAND
├── argv
│   └── cat
│
└── REDIR_IN
    └── input.txt
```

Вторая:

```text
COMMAND
├── argv
│   ├── grep
│   └── hello
│
└── APPEND
    └── result.txt
```

Между ними:

```text
PIPE
```

---

# 20. Обработка ошибок

Parser должен обнаруживать неправильную структуру команды.

Например:

```bash
|
```

```bash
| ls
```

```bash
ls |
```

```bash
ls | | wc
```

Это syntax errors.

---

# 20.1 Ошибки Redirection

Некорректно:

```bash
echo >
```

Потому что после `>` отсутствует filename.

Также:

```bash
cat <
```

```bash
cat >>
```

```bash
cat <<
```

После каждого redirection должен находиться `WORD`.

---

# 20.2 Пример

Input:

```bash
echo > | cat
```

Tokens:

```text
WORD("echo")
REDIR_OUT
PIPE
WORD("cat")
```

После:

```text
REDIR_OUT
```

Parser ожидает:

```text
WORD
```

но получает:

```text
PIPE
```

Следовательно:

```text
SYNTAX ERROR
```

---

# 20.3 Функция Parser Error

Можно создать helper:

```c
void parser_error(const char *message)
{
    ft_putstr_fd("minishell: syntax error: ", 2);
    ft_putendl_fd((char *)message, 2);
}
```

Точный текст ошибки должен соответствовать требованиям вашего проекта.

---

# 21. Частые ошибки

## Ошибка 1 — Parser выполняет команды

Parser не должен вызывать:

```c
execve()
fork()
wait()
```

Его задача — создать структуру.

---

# 21.1 Ошибка 2 — Смешивание Lexer и Parser

Parser не должен снова анализировать отдельные символы.

Lexer уже должен преобразовать:

```text
|
>
>>
<
<<
```

в token types.

Parser работает с готовыми токенами.

---

# 21.2 Ошибка 3 — Игнорирование Redirection

Неправильно:

```bash
echo hello > file
```

Parser создаёт:

```text
COMMAND
├── echo
└── hello
```

и забывает:

```text
> file
```

Правильно:

```text
COMMAND
├── argv
│   ├── echo
│   └── hello
│
└── REDIR_OUT
    └── file
```

---

# 21.3 Ошибка 4 — Pipe становится аргументом

Неправильно:

```bash
echo hello | grep hello
```

представить как:

```text
argv:
    echo
    hello
    |
    grep
    hello
```

Правильно:

```text
PIPE
├── COMMAND
│   ├── echo
│   └── hello
│
└── COMMAND
    ├── grep
    └── hello
```

---

# 21.4 Ошибка 5 — Не проверять положение Pipe

Parser должен отклонять:

```bash
| ls
```

```bash
ls |
```

```bash
ls | | wc
```

---

# 21.5 Ошибка 6 — Не проверять target Redirection

Нужно отклонять:

```bash
echo >
```

и:

```bash
echo < |
```

потому что после redirection должен находиться `WORD`.

---

# 21.6 Ошибка 7 — Потеря порядка команд

Для:

```bash
cat | grep | wc
```

порядок должен сохраняться:

```text
cat
 ↓
grep
 ↓
wc
```

Executor зависит от этого порядка.

---

# 22. Тестирование

## Test 1 — Простая команда

```bash
echo hello
```

Ожидается:

```text
COMMAND
├── echo
└── hello
```

---

## Test 2 — Несколько аргументов

```bash
ls -la /tmp
```

Ожидается:

```text
COMMAND
├── ls
├── -la
└── /tmp
```

---

## Test 3 — Output redirection

```bash
echo hello > file
```

Ожидается:

```text
COMMAND
├── echo
├── hello
└── REDIR_OUT → file
```

---

## Test 4 — Input redirection

```bash
cat < input.txt
```

Ожидается:

```text
COMMAND
├── cat
└── REDIR_IN → input.txt
```

---

## Test 5 — Append

```bash
echo hello >> file
```

Ожидается:

```text
COMMAND
├── echo
├── hello
└── APPEND → file
```

---

## Test 6 — Heredoc

```bash
cat << EOF
```

Ожидается:

```text
COMMAND
├── cat
└── HEREDOC → EOF
```

---

## Test 7 — Pipe

```bash
echo hello | grep hello
```

Ожидается:

```text
       PIPE
      /    \
   echo    grep
    |       |
  hello    hello
```

---

## Test 8 — Несколько Pipes

```bash
cat file | grep hello | wc -l
```

Ожидается:

```text
PIPE
├── PIPE
│   ├── cat file
│   └── grep hello
│
└── wc -l
```

---

## Test 9 — Несколько Redirections

```bash
cat < input.txt > output.txt
```

Ожидается:

```text
COMMAND
├── cat
├── REDIR_IN → input.txt
└── REDIR_OUT → output.txt
```

---

## Test 10 — Pipe + Redirection

```bash
cat file | grep hello > result.txt
```

Ожидается:

```text
PIPE
├── COMMAND
│   ├── cat
│   └── file
│
└── COMMAND
    ├── grep
    ├── hello
    └── REDIR_OUT → result.txt
```

---

## Test 11 — Некорректный Pipe

```bash
| echo
```

Ожидается:

```text
SYNTAX ERROR
```

---

## Test 12 — Pipe в конце

```bash
echo hello |
```

Ожидается:

```text
SYNTAX ERROR
```

---

## Test 13 — Два Pipe подряд

```bash
echo hello | | cat
```

Ожидается:

```text
SYNTAX ERROR
```

---

## Test 14 — Нет target у Redirection

```bash
echo >
```

Ожидается:

```text
SYNTAX ERROR
```

---

## Test 15 — Redirection перед Pipe

```bash
echo > | cat
```

Ожидается:

```text
SYNTAX ERROR
```

---

# 22.1 Таблица тестов

| Input             | Ожидаемый результат          |
| ----------------- | ---------------------------- |
| `echo hello`      | Одна команда                 |
| `ls -la`          | Команда + аргумент           |
| `cat < file`      | Команда + input redirection  |
| `echo hi > file`  | Команда + output redirection |
| `echo hi >> file` | Команда + append             |
| `cat << EOF`      | Команда + heredoc            |
| `ls \| grep .c`   | Две команды + pipe           |
| `a \| b \| c`     | Три команды + два pipe       |
| `cat < in > out`  | Команда + две redirections   |
| `\| ls`           | Syntax error                 |
| `ls \|`           | Syntax error                 |
| `ls \| \| wc`     | Syntax error                 |
| `echo >`          | Syntax error                 |
| `echo > \| cat`   | Syntax error                 |

---

# 23. Финальный чек-лист

## Parser Structures

* [ ] Создана `t_token`.
* [ ] Определены token types.
* [ ] Создана `t_command`.
* [ ] Создана структура для redirections.
* [ ] Создана AST structure, если используется AST.
* [ ] Создан Parser context, если он нужен.

## Commands

* [ ] Простые команды парсятся.
* [ ] Аргументы сохраняются.
* [ ] Порядок команд сохраняется.

## Redirections

* [ ] `<` распознаётся.
* [ ] `>` распознаётся.
* [ ] `>>` распознаётся.
* [ ] `<<` распознаётся.
* [ ] Targets сохраняются.
* [ ] Отсутствующий target вызывает syntax error.

## Pipes

* [ ] `|` создаёт связь между командами.
* [ ] Команда слева от Pipe парсится.
* [ ] Команда справа от Pipe парсится.
* [ ] Несколько Pipes работают.
* [ ] Pipe в начале отклоняется.
* [ ] Pipe в конце отклоняется.
* [ ] Два Pipe подряд отклоняются.

## Memory

* [ ] AST nodes корректно выделяются.
* [ ] Commands корректно освобождаются.
* [ ] Redirections корректно освобождаются.
* [ ] Token references обрабатываются безопасно.
* [ ] Нет memory leaks.
* [ ] Нет invalid reads.
* [ ] Нет double free.

---

# 24. Общая архитектура

На этом этапе архитектура Minishell должна выглядеть примерно так:

```text
                     USER INPUT
                         |
                         v
                       LEXER
                         |
                         v
                    TOKEN LIST
                         |
                         v
                      PARSER
                         |
              +----------+----------+
              |                     |
              v                     v
          COMMANDS                 AST
              |                     |
              +----------+----------+
                         |
                         v
                  PARAMETER EXPANSION
                         |
                         v
                    QUOTE REMOVAL
                         |
                         v
                     EXECUTOR
                         |
              +----------+----------+
              |                     |
              v                     v
            PIPE                 REDIRECTION
              |                     |
              +----------+----------+
                         |
                         v
                      execve()
```

---

# Главная идея

Parser больше **не должен анализировать отдельные символы**.

Lexer уже преобразовал:

```bash
cat file | grep hello > result.txt
```

в:

```text
WORD("cat")
WORD("file")
PIPE
WORD("grep")
WORD("hello")
REDIR_OUT
WORD("result.txt")
```

Теперь Parser должен понять отношения между ними:

```text
                  PIPE
                 /    \
                /      \
           COMMAND    COMMAND
           /    \      /    \
         cat   file   grep  hello
                           |
                        REDIR_OUT
                            |
                       result.txt
```

Самый важный принцип:

> **Lexer определяет TOKENS. Parser определяет STRUCTURE. Executor выполняет COMMAND.**

---

# Что нужно реализовать на этом этапе

Минимальный результат работы должен быть примерно таким:

```text
INPUT
  ↓
Lexer
  ↓
t_token list
  ↓
Parser
  ↓
t_command / AST
```

Например:

```bash
cat file.txt | grep hello > result.txt
```

должно превратиться из:

```text
WORD("cat")
WORD("file.txt")
PIPE
WORD("grep")
WORD("hello")
REDIR_OUT
WORD("result.txt")
```

в структурированное представление:

```text
                    PIPE
                   /    \
                  /      \
             COMMAND    COMMAND
             /    \      /    \
           cat   file   grep  hello
                              |
                         REDIR_OUT
                              |
                         result.txt
```

После этого Executor уже сможет использовать эту структуру для:

* создания процессов;
* создания pipes;
* настройки `stdin`;
* настройки `stdout`;
* открытия файлов;
* обработки heredoc;
* запуска `execve()`.

Именно поэтому **Parser является мостом между Lexer и Executor**.
