# Understanding Token Types in Minishell

## Содержание

* [1. Что такое Token](#1-что-такое-token)
* [2. Зачем нужны Token Types](#2-зачем-нужны-token-types)
* [3. Lexer и Tokens](#3-lexer-и-tokens)
* [4. Основные типы токенов](#4-основные-типы-токенов)
* [5. WORD](#5-word)
* [6. PIPE](#6-pipe)
* [7. REDIR_IN](#7-redir_in)
* [8. REDIR_OUT](#8-redir_out)
* [9. APPEND](#9-append)
* [10. HEREDOC](#10-heredoc)
* [11. END / EOF](#11-end--eof)
* [12. Почему кавычки меняют Token Type](#12-почему-кавычки-меняют-token-type)
* [13. Quoted Operators](#13-quoted-operators)
* [14. Operators внутри WORD](#14-operators-внутри-word)
* [15. Tokenization Examples](#15-tokenization-examples)
* [16. Token Structure](#16-token-structure)
* [17. Enum для Token Types](#17-enum-для-token-types)
* [18. Как Lexer определяет Token Type](#18-как-lexer-определяет-token-type)
* [19. Алгоритм Lexer](#19-алгоритм-lexer)
* [20. Parser и Token Types](#20-parser-и-token-types)
* [21. Tokens → AST / Command Structure](#21-tokens--ast--command-structure)
* [22. Ошибки Lexer](#22-ошибки-lexer)
* [23. Ошибки Parser](#23-ошибки-parser)
* [24. Типичные ошибки](#24-типичные-ошибки)
* [25. Примеры для тестирования](#25-примеры-для-тестирования)
* [26. Checklist](#26-checklist)
* [27. Вопросы для проверки знаний](#27-вопросы-для-проверки-знаний)
* [28. Главная модель](#28-главная-модель)

---

# 1. Что такое Token

**Token** — это отдельная смысловая единица командной строки.

Например:

```bash
echo hello
```

можно представить как:

```text
WORD("echo")
WORD("hello")
```

А:

```bash
echo hello | grep hello
```

как:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

Token содержит как минимум:

```text
value
type
```

Например:

```text
value = "echo"
type  = WORD
```

---

# 2. Зачем нужны Token Types

Parser должен понимать структуру команды.

Например:

```bash
cat file.txt > output.txt
```

Lexer должен определить:

```text
WORD("cat")
WORD("file.txt")
REDIR_OUT(">")
WORD("output.txt")
```

Parser теперь понимает:

```text
command = cat
argument = file.txt
output redirection = output.txt
```

Без Token Types Parser не сможет отличить:

```text
>
```

от:

```text
hello
```

Поэтому:

> Token Type сообщает Parser, какое значение имеет конкретный фрагмент командной строки.

---

# 3. Lexer и Tokens

Общий pipeline:

```text
RAW INPUT
    |
    v
+---------+
|  LEXER  |
+---------+
    |
    v
TOKENS
    |
    v
+---------+
| PARSER  |
+---------+
    |
    v
COMMAND STRUCTURE
    |
    v
EXPANSION / EXECUTION
```

Например:

```bash
echo hello | grep hello
```

Lexer создаёт:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

Parser использует эти токены для построения структуры команды.

---

# 4. Основные типы токенов

Для стандартного Minishell необходимы следующие основные типы:

```text
WORD
PIPE
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
```

Также удобно иметь:

```text
EOF
```

или:

```text
END
```

для обозначения конца списка токенов.

---

## Таблица

| Token     | Символ | Значение                      |
| --------- | ------ | ----------------------------- |
| WORD      | текст  | command / argument / filename |
| PIPE      | `\|`   | pipe между командами          |
| REDIR_IN  | `<`    | input redirection             |
| REDIR_OUT | `>`    | output redirection            |
| APPEND    | `>>`   | append output                 |
| HEREDOC   | `<<`   | heredoc                       |
| EOF       | конец  | конец input                   |

---

# 5. WORD

`WORD` — самый распространённый тип токена.

К нему относятся:

```text
echo
hello
file.txt
$USER
abc
"hello world"
```

Например:

```bash
echo hello
```

токены:

```text
WORD("echo")
WORD("hello")
```

---

## 5.1 Command тоже WORD

Очень важно:

```bash
echo hello
```

`echo` не имеет отдельного типа:

```text
COMMAND
```

На этапе Lexer:

```text
WORD("echo")
WORD("hello")
```

Parser уже определяет, что первый `WORD` является command.

---

## 5.2 Filename тоже WORD

Например:

```bash
cat file.txt
```

Lexer:

```text
WORD("cat")
WORD("file.txt")
```

`file.txt` также является `WORD`.

---

## 5.3 Filename после redirection

Например:

```bash
echo hello > output.txt
```

Lexer:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

`output.txt` всё равно:

```text
WORD
```

Parser понимает, что этот WORD является target для redirection.

---

# 6. PIPE

Pipe:

```text
|
```

имеет Token Type:

```text
PIPE
```

Например:

```bash
ls | grep txt
```

получаем:

```text
WORD("ls")
PIPE("|")
WORD("grep")
WORD("txt")
```

Pipe разделяет две команды.

Логически:

```text
ls
 |
grep
```

---

# 6.1 Pipe не является WORD

Без кавычек:

```bash
echo hello | grep hello
```

`|`:

```text
PIPE
```

а не:

```text
WORD("|")
```

---

# 6.2 Pipe внутри кавычек

Очень важно:

```bash
echo "|"
```

получаем:

```text
WORD("echo")
WORD("|")
```

а не:

```text
PIPE
```

То же самое:

```bash
echo '|'
```

→

```text
WORD("echo")
WORD("|")
```

---

# 7. REDIR_IN

Символ:

```text
<
```

имеет тип:

```text
REDIR_IN
```

Например:

```bash
cat < input.txt
```

Lexer:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

Parser понимает:

```text
input redirection
        |
        v
   input.txt
```

---

# 7.1 Как работает `<`

Команда:

```bash
cat < file.txt
```

означает:

```text
stdin ← file.txt
```

То есть программа `cat` получает стандартный ввод из файла.

---

# 7.2 `<` внутри кавычек

```bash
echo "<"
```

даёт:

```text
WORD("echo")
WORD("<")
```

`<` здесь не является redirection.

---

# 8. REDIR_OUT

Символ:

```text
>
```

имеет тип:

```text
REDIR_OUT
```

Например:

```bash
echo hello > output.txt
```

токены:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

## 8.1 Что означает `>`

```bash
echo hello > output.txt
```

означает:

```text
stdout → output.txt
```

Если файл существует, обычная `>` redirection открывает его с очисткой содержимого.

---

# 8.2 `>` внутри кавычек

```bash
echo ">"
```

→

```text
WORD("echo")
WORD(">")
```

---

# 9. APPEND

Два символа:

```text
>>
```

образуют один оператор:

```text
APPEND
```

Например:

```bash
echo hello >> output.txt
```

Lexer:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("output.txt")
```

---

## 9.1 Отличие `>` и `>>`

```text
>   → overwrite
>>  → append
```

Например:

```bash
echo hello > file
```

перезаписывает файл.

А:

```bash
echo hello >> file
```

добавляет текст в конец.

---

## 9.2 `>>` — один Token

Очень важно:

```text
>>
```

это:

```text
APPEND
```

а не:

```text
REDIR_OUT
REDIR_OUT
```

Lexer должен распознавать двойной оператор как единый token.

---

# 10. HEREDOC

Два символа:

```text
<<
```

образуют:

```text
HEREDOC
```

Например:

```bash
cat << EOF
hello
EOF
```

Lexer:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

`EOF` — delimiter heredoc.

---

## 10.1 Что делает `<<`

`<<` говорит Shell:

> Получи input от пользователя до тех пор, пока не встретишь delimiter.

Например:

```bash
cat << EOF
hello
world
EOF
```

Input:

```text
hello
world
```

---

## 10.2 `<<` — один Token

Как и `>>`:

```text
<<
```

должен стать:

```text
HEREDOC
```

а не:

```text
REDIR_IN
REDIR_IN
```

---

# 11. END / EOF

Lexer должен знать, где заканчивается input.

Например:

```bash
echo hello
```

после:

```text
WORD("echo")
WORD("hello")
```

можно иметь:

```text
EOF
```

Это особенно удобно для Parser.

---

# 12. Почему кавычки меняют Token Type

Ключевое правило:

> Token Type определяется не только самим символом, но и контекстом.

Например:

```bash
echo |
```

получаем:

```text
WORD("echo")
PIPE("|")
```

Но:

```bash
echo "|"
```

получаем:

```text
WORD("echo")
WORD("|")
```

Почему?

Потому что во втором случае `|` находится внутри quotes.

---

# 13. Quoted Operators

Операторы внутри кавычек становятся частью `WORD`.

### Pipe

```bash
echo "|"
```

→

```text
WORD("echo")
WORD("|")
```

### Input

```bash
echo "<"
```

→

```text
WORD("echo")
WORD("<")
```

### Output

```bash
echo ">"
```

→

```text
WORD("echo")
WORD(">")
```

### Append

```bash
echo ">>"
```

→

```text
WORD("echo")
WORD(">>")
```

### Heredoc

```bash
echo "<<"
```

→

```text
WORD("echo")
WORD("<<")
```

---

# 14. Operators внутри WORD

Оператор может находиться рядом с текстом.

Например:

```bash
echo hello>file
```

Lexer должен получить:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

Не требуется пробел между `hello` и `>`.

---

## Ещё пример

```bash
echo hello|grep
```

получаем:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
```

---

## Ещё пример

```bash
cat<file
```

получаем:

```text
WORD("cat")
REDIR_IN("<")
WORD("file")
```

---

# 15. Tokenization Examples

## Example 1

Input:

```bash
echo hello
```

Tokens:

```text
WORD("echo")
WORD("hello")
```

---

## Example 2

Input:

```bash
echo hello world
```

Tokens:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

---

## Example 3

Input:

```bash
ls | grep txt
```

Tokens:

```text
WORD("ls")
PIPE("|")
WORD("grep")
WORD("txt")
```

---

## Example 4

Input:

```bash
cat < input.txt
```

Tokens:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

## Example 5

Input:

```bash
echo hello > output.txt
```

Tokens:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

## Example 6

Input:

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("output.txt")
```

---

## Example 7

Input:

```bash
cat << EOF
```

Tokens:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

## Example 8

Input:

```bash
echo "hello world"
```

Tokens:

```text
WORD("echo")
WORD("hello world")
```

---

## Example 9

Input:

```bash
echo "$USER"
```

Tokens:

```text
WORD("echo")
WORD("$USER")
```

Expansion происходит позже.

---

## Example 10

Input:

```bash
echo '$USER'
```

Tokens:

```text
WORD("echo")
WORD("$USER")
```

Но Lexer/Word должен сохранить информацию о quoting, потому что expansion будет отличаться.

---

# 16. Token Structure

В C можно создать структуру:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
}   t_token;
```

Например:

```text
t_token
   |
   +-- value = "echo"
   +-- type  = WORD
   |
   v
t_token
   |
   +-- value = "|"
   +-- type  = PIPE
   |
   v
t_token
   |
   +-- value = "grep"
   +-- type  = WORD
```

---

# 16.1 Doubly Linked List

Можно использовать:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
    struct s_token  *prev;
}   t_token;
```

Это может упростить некоторые операции Parser.

---

# 17. Enum для Token Types

Лучше использовать `enum`.

Например:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC,
    TOKEN_EOF
}   t_token_type;
```

Теперь вместо:

```c
type = 3;
```

можно писать:

```c
type = TOKEN_REDIR_OUT;
```

Это намного понятнее.

---

# 18. Как Lexer определяет Token Type

Основная логика:

```text
Current character
       |
       v
 Is it whitespace?
       |
       +-- YES → skip
       |
       NO
       |
       v
 Is it ' or "?
       |
       +-- YES → parse WORD with quotes
       |
       NO
       |
       v
 Is it | ?
       |
       +-- YES → PIPE
       |
       NO
       |
       v
 Is it < ?
       |
       +-- YES → REDIR_IN or HEREDOC
       |
       NO
       |
       v
 Is it > ?
       |
       +-- YES → REDIR_OUT or APPEND
       |
       NO
       |
       v
 Parse WORD
```

---

# 19. Алгоритм Lexer

Рекомендуемый общий алгоритм:

```text
while input is not finished:

    1. Skip spaces

    2. Check operators

       "|"   → PIPE
       "<"   → REDIR_IN
       ">"   → REDIR_OUT
       "<<"  → HEREDOC
       ">>"  → APPEND

    3. Otherwise parse WORD

       WORD may contain:
       - normal characters
       - single quotes
       - double quotes
       - escaped characters

    4. Add token to token list

    5. Continue
```

Но есть важное условие:

> Operators распознаются только в `NORMAL` quote state.

---

# 19.1 Почему нельзя просто искать символы

Нельзя делать:

```c
if (input[i] == '|')
    token->type = PIPE;
```

без проверки контекста.

Потому что:

```bash
echo "|"
```

содержит `|`, но это не PIPE.

Правильно:

```text
if state == NORMAL && input[i] == '|'
    → PIPE
```

---

# 19.2 Аналогично для `<` и `>`

Нельзя просто:

```c
if (input[i] == '>')
    REDIR_OUT
```

Потому что:

```bash
echo ">"
```

должно быть:

```text
WORD(">")
```

---

# 20. Parser и Token Types

Parser использует Token Types для понимания грамматики.

Например:

```text
WORD
WORD
PIPE
WORD
WORD
```

Parser может построить:

```text
Command 1
    |
    +-- echo
    +-- hello

PIPE

Command 2
    |
    +-- grep
    +-- hello
```

---

# 20.1 Redirection

Input:

```bash
echo hello > output.txt
```

Tokens:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

Parser понимает:

```text
COMMAND
 |
 +-- argv:
 |     echo
 |     hello
 |
 +-- redirection:
       >
       output.txt
```

---

# 20.2 Heredoc

Input:

```bash
cat << EOF
```

Tokens:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

Parser понимает:

```text
command = cat

heredoc delimiter = EOF
```

---

# 21. Tokens → AST / Command Structure

Есть два распространённых подхода.

## Подход 1 — Command List

Создавать структуру команды:

```c
typedef struct s_command
{
    char    **argv;
    // redirections
    // etc.
}   t_command;
```

---

## Подход 2 — AST

Можно создать дерево:

```text
          PIPE
         /    \
        /      \
     CMD        CMD
      |          |
     ls         grep
```

Для:

```bash
ls | grep txt
```

---

## Redirection

Например:

```bash
cat < input.txt
```

можно представить:

```text
CMD
 |
 +-- argv: cat
 |
 +-- REDIR_IN
       |
       +-- input.txt
```

---

# 22. Ошибки Lexer

Lexer отвечает за некоторые синтаксические проблемы.

Например:

```bash
echo "hello
```

имеет незакрытую кавычку.

Lexer должен обнаружить:

```text
UNCLOSED_QUOTE
```

или вернуть ошибку.

---

## 22.1 Parser errors

Некоторые ошибки относятся уже к Parser.

Например:

```bash
|
```

Pipe находится там, где ожидается command.

Или:

```bash
echo hello |
```

После pipe нет следующей команды.

Или:

```bash
echo >
```

После redirection нет filename.

Важно разделять:

```text
Lexer errors
```

и:

```text
Parser errors
```

---

# 23. Ошибки Parser

### Только pipe

```bash
|
```

Некорректная структура.

---

### Два pipe

```bash
echo hello || grep hello
```

Для базового Minishell `||` не является разрешённым оператором.

Нужно корректно обработать такой input согласно требованиям проекта.

---

### Pipe в конце

```bash
echo hello |
```

После `PIPE` должен идти command.

---

### Redirection без filename

```bash
echo hello >
```

После:

```text
REDIR_OUT
```

должен идти:

```text
WORD
```

---

### Heredoc без delimiter

```bash
cat <<
```

После:

```text
HEREDOC
```

должен идти:

```text
WORD
```

---

# 24. Типичные ошибки

## Ошибка 1 — `>>` как два `>`

Неправильно:

```text
REDIR_OUT
REDIR_OUT
```

Правильно:

```text
APPEND
```

---

## Ошибка 2 — `<<` как два `<`

Неправильно:

```text
REDIR_IN
REDIR_IN
```

Правильно:

```text
HEREDOC
```

---

## Ошибка 3 — quoted operator как operator

Неправильно:

```bash
echo "|"
```

→

```text
PIPE
```

Правильно:

```text
WORD("|")
```

---

## Ошибка 4 — command имеет отдельный type

Не нужно создавать:

```text
COMMAND
```

для:

```text
echo
```

На этапе Lexer:

```text
WORD("echo")
```

Parser определит его роль.

---

## Ошибка 5 — filename после redirection имеет специальный type

Для:

```bash
cat < input.txt
```

не нужно:

```text
REDIR_IN
FILENAME
```

Достаточно:

```text
REDIR_IN
WORD("input.txt")
```

---

## Ошибка 6 — operators требуют пробелов

Неправильно считать:

```bash
echo hello>file
```

обычным WORD.

Правильно:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

Операторы не требуют пробелов.

---

# 25. Примеры для тестирования

## Basic

```bash
echo hello
```

Ожидается:

```text
WORD echo
WORD hello
```

---

## Pipe

```bash
echo hello | grep hello
```

Ожидается:

```text
WORD echo
WORD hello
PIPE
WORD grep
WORD hello
```

---

## Input redirection

```bash
cat < input
```

Ожидается:

```text
WORD cat
REDIR_IN
WORD input
```

---

## Output redirection

```bash
echo hello > output
```

Ожидается:

```text
WORD echo
WORD hello
REDIR_OUT
WORD output
```

---

## Append

```bash
echo hello >> output
```

Ожидается:

```text
WORD echo
WORD hello
APPEND
WORD output
```

---

## Heredoc

```bash
cat << EOF
```

Ожидается:

```text
WORD cat
HEREDOC
WORD EOF
```

---

## No spaces

```bash
cat<input
```

Ожидается:

```text
WORD cat
REDIR_IN
WORD input
```

---

```bash
echo hello>file
```

Ожидается:

```text
WORD echo
WORD hello
REDIR_OUT
WORD file
```

---

```bash
echo hello|grep
```

Ожидается:

```text
WORD echo
WORD hello
PIPE
WORD grep
```

---

## Quotes

```bash
echo "hello | world"
```

Ожидается:

```text
WORD echo
WORD hello | world
```

---

```bash
echo 'hello > world'
```

Ожидается:

```text
WORD echo
WORD hello > world
```

---

## Concatenation

```bash
echo hello"world"
```

Ожидается:

```text
WORD echo
WORD helloworld
```

---

## Multiple operators

```bash
cat < input > output
```

Ожидается:

```text
WORD cat
REDIR_IN
WORD input
REDIR_OUT
WORD output
```

---

## Complex example

```bash
cat < input.txt | grep "hello world" >> result.txt
```

Ожидается:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
PIPE("|")
WORD("grep")
WORD("hello world")
APPEND(">>")
WORD("result.txt")
```

---

# 26. Checklist

Перед тем как считать тему **Token Types** изученной, нужно понимать:

## Basic Tokens

* [ ] Что такое Token.
* [ ] Что такое Token Type.
* [ ] Зачем Parser нужны Token Types.
* [ ] Что такое `WORD`.
* [ ] Что такое `PIPE`.
* [ ] Что такое `REDIR_IN`.
* [ ] Что такое `REDIR_OUT`.
* [ ] Что такое `APPEND`.
* [ ] Что такое `HEREDOC`.
* [ ] Что такое `EOF`.

---

## Operators

Нужно уметь распознать:

```text
|
<
>
<<
>>
```

как:

```text
PIPE
REDIR_IN
REDIR_OUT
HEREDOC
APPEND
```

---

## Quotes

Нужно понимать:

```bash
echo "|"
echo "<"
echo ">"
echo "<<"
echo ">>"
```

Почему все операторы становятся:

```text
WORD
```

---

## No-space operators

Понимать:

```bash
cat<input
```

```bash
echo hello>file
```

```bash
echo hello|grep
```

---

## Parser

Понимать:

```text
Lexer
  ↓
Tokens
  ↓
Parser
  ↓
Command structure
```

---

# 27. Вопросы для проверки знаний

### Основы

1. Что такое Token?
2. Что такое Token Type?
3. Какие Token Types нужны для Minishell?
4. Почему `echo` является `WORD`, а не `COMMAND`?
5. Почему filename является `WORD`?

### Operators

6. Какой type имеет `|`?
7. Какой type имеет `<`?
8. Какой type имеет `>`?
9. Какой type имеет `>>`?
10. Какой type имеет `<<`?

### Quotes

11. Какой token получится из:

```bash
echo "|"
```

12. Какой token получится из:

```bash
echo ">"
```

13. Почему `|` внутри кавычек не является `PIPE`?

### Lexer

14. Должен ли оператор иметь пробелы вокруг себя?
15. Как обработать:

```bash
cat<input
```

16. Как обработать:

```bash
echo hello>file
```

17. Как обработать:

```bash
echo hello|grep
```

### Parser

18. Что должен делать Parser с:

```text
WORD
WORD
PIPE
WORD
```

19. Что должен делать Parser, если после `>` нет `WORD`?

20. Что должен делать Parser с:

```text
WORD
HEREDOC
WORD
```

---

# 28. Главная модель

Запомните эту модель:

```text
                    RAW INPUT
                        |
                        v
                 +--------------+
                 |    LEXER     |
                 +--------------+
                        |
                        v
                     TOKENS
                        |
          +-------------+-------------+
          |             |             |
          v             v             v
        WORD        OPERATORS      EOF
                      |
          +-----------+-----------+
          |           |           |
          v           v           v
        PIPE        REDIRS      HEREDOC
          |           |
          |       +---+---+---+
          |       |   |   |   |
          |       v   v   v   v
          |       <   >  <<  >>
          |
          v
       PARSER
          |
          v
   COMMAND STRUCTURE
          |
          v
       EXPANSION
          |
          v
       EXECUTION
```

---

# Самое главное

Для Minishell необходимо чётко разделять:

```text
CHARACTER
    ↓
LEXER
    ↓
TOKEN
    ↓
TOKEN TYPE
    ↓
PARSER
```

Например:

```bash
echo hello | grep hello
```

становится:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

А:

```bash
echo "hello | world"
```

становится:

```text
WORD("echo")
WORD("hello | world")
```

То есть **один и тот же символ ****`|`**** может иметь совершенно разный Token Type в зависимости от quote context**.

Главные Token Types для Minishell:

```text
WORD
PIPE
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
EOF
```

Если вы понимаете, **как Lexer превращает raw input в эти типы токенов и почему quotes влияют на результат**, вы готовы переходить к следующему уровню: **Parser / grammar / command structure**.
