# Понимание проверки синтаксиса (Syntax Validation) в Minishell

## Содержание

* [1. Что такое Syntax Validation](#1-что-такое-syntax-validation)
* [2. Где происходит проверка синтаксиса](#2-где-происходит-проверка-синтаксиса)
* [3. Lexer vs Parser](#3-lexer-vs-parser)
* [4. Грамматика Shell](#4-грамматика-shell)
* [5. Допустимые и недопустимые последовательности токенов](#5-допустимые-и-недопустимые-последовательности-токенов)
* [6. Синтаксис Pipe](#6-синтаксис-pipe)
* [7. Синтаксис Redirection](#7-синтаксис-redirection)
* [8. Синтаксис Heredoc](#8-синтаксис-heredoc)
* [9. Несколько Redirections](#9-несколько-redirections)
* [10. Кавычки и Syntax Validation](#10-кавычки-и-syntax-validation)
* [11. Пустой ввод](#11-пустой-ввод)
* [12. Пробелы](#12-пробелы)
* [13. Неподдерживаемые операторы](#13-неподдерживаемые-операторы)
* [14. Обнаружение Syntax Errors](#14-обнаружение-syntax-errors)
* [15. Exit Status](#15-exit-status)
* [16. Parser State Machine](#16-parser-state-machine)
* [17. Алгоритм Syntax Validation](#17-алгоритм-syntax-validation)
* [18. Примеры](#18-примеры)
* [19. Типичные ошибки](#19-типичные-ошибки)
* [20. Структура реализации](#20-структура-реализации)
* [21. Стратегия тестирования](#21-стратегия-тестирования)
* [22. Checklist](#22-checklist)
* [23. Вопросы для проверки знаний](#23-вопросы-для-проверки-знаний)
* [24. Главная модель](#24-главная-модель)

---

# 1. Что такое Syntax Validation

**Syntax Validation** — это проверка того, соответствует ли последовательность токенов грамматическим правилам Shell.

Например:

```bash
echo hello | grep hello
```

синтаксически корректна.

Lexer создаёт:

```text
WORD
WORD
PIPE
WORD
WORD
```

А команда:

```bash
echo hello |
```

некорректна, потому что после `PIPE` должна находиться следующая команда.

Токены:

```text
WORD
WORD
PIPE
```

Parser должен обнаружить эту ошибку **до выполнения команды**.

---

# 2. Где происходит проверка синтаксиса

Общий pipeline Minishell:

```text
RAW INPUT
    |
    v
  LEXER
    |
    v
  TOKENS
    |
    v
SYNTAX VALIDATION
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

На практике Syntax Validation обычно является частью Parser.

Можно запомнить:

```text
Lexer:
"What tokens are here?"

Parser:
"Are these tokens arranged correctly?"
```

То есть:

**Lexer определяет элементы.**

**Parser проверяет их структуру.**

---

# 3. Lexer vs Parser

Это одно из самых важных различий в Minishell.

## Lexer

Для:

```bash
echo hello > file
```

Lexer создаёт:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

Lexer отвечает за распознавание:

* WORD
* PIPE
* `<`
* `>`
* `<<`
* `>>`
* quotes
* границ токенов

---

## Parser

Parser получает:

```text
WORD
WORD
REDIR_OUT
WORD
```

и проверяет:

```text
WORD WORD REDIR_OUT WORD
```

Это допустимая структура.

Но:

```bash
echo hello >
```

даёт:

```text
WORD
WORD
REDIR_OUT
```

Parser видит:

```text
REDIR_OUT
    ↓
ожидается WORD
    ↓
ничего нет
```

Результат:

```text
Syntax Error
```

---

# 4. Грамматика Shell

Для базового Minishell можно представить грамматику примерно так:

```text
command      → WORD
             | command WORD
             | command redirection

redirection  → REDIR_IN WORD
             | REDIR_OUT WORD
             | APPEND WORD
             | HEREDOC WORD

pipeline     → command
             | pipeline PIPE command
```

Это упрощённая модель, но она очень полезна для понимания Parser.

---

## Упрощённая структура команды

```text
COMMAND
   |
   +-- WORD
   |
   +-- WORD
   |
   +-- REDIRECTION
          |
          +-- operator
          +-- WORD
```

Pipeline:

```text
COMMAND PIPE COMMAND
```

Например:

```bash
cat file | grep hello
```

представляется как:

```text
COMMAND
    |
    +-- WORD("cat")
    +-- WORD("file")

PIPE

COMMAND
    |
    +-- WORD("grep")
    +-- WORD("hello")
```

---

# 5. Допустимые и недопустимые последовательности токенов

Parser проверяет отношения между токенами.

## Корректные последовательности

```text
WORD
```

Например:

```bash
ls
```

---

```text
WORD WORD
```

Например:

```bash
echo hello
```

---

```text
WORD PIPE WORD
```

Например:

```bash
ls | cat
```

---

```text
WORD REDIR_OUT WORD
```

Например:

```bash
echo hello > file
```

---

```text
WORD REDIR_IN WORD
```

Например:

```bash
cat < file
```

---

```text
WORD APPEND WORD
```

Например:

```bash
echo hello >> file
```

---

```text
WORD HEREDOC WORD
```

Например:

```bash
cat << EOF
```

---

## Некорректные последовательности

```text
PIPE
```

Например:

```bash
|
```

---

```text
WORD PIPE
```

Например:

```bash
echo hello |
```

---

```text
PIPE WORD
```

Например:

```bash
| echo
```

---

```text
WORD REDIR_OUT
```

Например:

```bash
echo >
```

---

```text
WORD REDIR_IN
```

Например:

```bash
cat <
```

---

```text
WORD HEREDOC
```

Например:

```bash
cat <<
```

---

# 6. Синтаксис Pipe

Оператор:

```text
|
```

имеет Token Type:

```text
PIPE
```

Его основное правило:

```text
COMMAND PIPE COMMAND
```

Например:

```bash
ls | grep txt
```

корректно.

---

## 6.1 Pipe в начале

```bash
| ls
```

Токены:

```text
PIPE
WORD
```

Некорректно.

Почему?

Parser ожидает:

```text
COMMAND
```

перед `PIPE`.

---

## 6.2 Pipe в конце

```bash
ls |
```

Токены:

```text
WORD
PIPE
```

Некорректно.

После `PIPE` должна находиться следующая команда.

---

## 6.3 Два Pipe подряд

```bash
ls | | grep
```

Токены:

```text
WORD
PIPE
PIPE
WORD
```

Некорректно.

После первого:

```text
PIPE
```

Parser ожидает начало новой команды, а получает второй:

```text
PIPE
```

---

## 6.4 Несколько Pipe

Это корректно:

```bash
cat file | grep hello | wc -l
```

Токены:

```text
WORD
WORD
PIPE
WORD
WORD
PIPE
WORD
WORD
```

Структура:

```text
cat
 |
grep
 |
wc
```

---

# 7. Синтаксис Redirection

Каждый оператор перенаправления должен иметь после себя `WORD`.

Правила:

```text
REDIR_IN   → WORD
REDIR_OUT  → WORD
APPEND     → WORD
HEREDOC    → WORD
```

---

# 7.1 Input Redirection

Корректно:

```bash
cat < input.txt
```

Токены:

```text
WORD
REDIR_IN
WORD
```

Некорректно:

```bash
cat <
```

Токены:

```text
WORD
REDIR_IN
```

После `<` отсутствует `WORD`.

---

# 7.2 Output Redirection

Корректно:

```bash
echo hello > output.txt
```

Токены:

```text
WORD
WORD
REDIR_OUT
WORD
```

Некорректно:

```bash
echo hello >
```

Токены:

```text
WORD
WORD
REDIR_OUT
```

---

# 7.3 Append

Корректно:

```bash
echo hello >> output.txt
```

Токены:

```text
WORD
WORD
APPEND
WORD
```

Некорректно:

```bash
echo hello >>
```

Токены:

```text
WORD
WORD
APPEND
```

---

# 7.4 Heredoc

Корректно:

```bash
cat << EOF
```

Токены:

```text
WORD
HEREDOC
WORD
```

Некорректно:

```bash
cat <<
```

Токены:

```text
WORD
HEREDOC
```

Отсутствует delimiter.

---

# 8. Синтаксис Heredoc

Heredoc имеет структуру:

```text
HEREDOC WORD
```

Например:

```bash
cat << EOF
```

где:

```text
EOF
```

— delimiter.

Shell затем читает строки до тех пор, пока не встретит этот delimiter.

Например:

```bash
cat << EOF
hello
world
EOF
```

Heredoc input:

```text
hello
world
```

Parser на этом этапе должен понимать структуру:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

Само чтение heredoc происходит позже.

---

# 9. Несколько Redirections

Одна команда может иметь несколько redirections.

Например:

```bash
cat < input > output
```

корректна.

Токены:

```text
WORD("cat")
REDIR_IN("<")
WORD("input")
REDIR_OUT(">")
WORD("output")
```

Также синтаксически допустимо:

```bash
echo hello > file1 > file2
```

Токены:

```text
WORD
WORD
REDIR_OUT
WORD
REDIR_OUT
WORD
```

Parser не должен автоматически считать несколько redirections ошибкой.

---

## Несколько Input Redirections

Например:

```bash
cat < file1 < file2
```

также может быть синтаксически корректной.

Что произойдёт при выполнении — это уже вопрос обработки redirections, а не Syntax Validation.

Очень важно разделять:

> Syntax Validation проверяет, является ли структура допустимой.

Она не отвечает на вопрос:

> Имеет ли команда полезное или ожидаемое поведение?

---

# 10. Кавычки и Syntax Validation

Кавычки в первую очередь обрабатываются Lexer.

Например:

```bash
echo "|"
```

`|` находится внутри quotes.

Поэтому токены:

```text
WORD
WORD
```

а не:

```text
WORD
PIPE
```

---

## Другой пример

```bash
echo "hello > world"
```

`>` является обычным текстом.

Получаем:

```text
WORD
WORD
```

---

## Незакрытые кавычки

Например:

```bash
echo "hello
```

Кавычка не закрыта.

Lexer должен обнаружить:

```text
UNCLOSED_QUOTE
```

или вернуть соответствующую ошибку.

Parser не должен пытаться выполнять такую команду как обычную.

---

# 11. Пустой ввод

Если пользователь просто нажал Enter:

```text
""
```

токенов нет.

Обычно это не является Syntax Error.

Логика:

```text
INPUT
  ↓
NO TOKENS
  ↓
DO NOTHING
  ↓
SHOW PROMPT AGAIN
```

---

# 11.1 Только пробелы

Например:

```text
"       "
```

После обработки пробелов:

```text
NO TOKENS
```

Это также обычно игнорируется.

---

# 12. Пробелы

Пробелы разделяют слова, но сами обычно не становятся токенами.

Например:

```bash
echo hello world
```

превращается в:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

Пробелы исчезают во время tokenization.

---

## Важно

Операторы не требуют пробелов.

Например:

```bash
echo hello>file
```

корректно разбирается как:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

Также:

```bash
cat<input
```

→

```text
WORD("cat")
REDIR_IN("<")
WORD("input")
```

---

# 13. Неподдерживаемые операторы

Minishell не реализует все возможности Bash.

В базовом Minishell необходимо поддерживать:

```text
|
<
>
<<
>>
```

Операторы вроде:

```text
&&
||
;
```

не входят в базовую требуемую функциональность Minishell.

Важно, чтобы они не превращались случайно в корректную shell-конструкцию.

Например:

```bash
echo hello && echo world
```

не должен внезапно выполняться так, будто `&&` полностью поддерживается.

В зависимости от архитектуры Lexer/Parser можно:

```text
обнаружить неподдерживаемый оператор
```

или:

```text
создать токены, которые Parser отклонит
```

Главное:

> Неподдерживаемый синтаксис не должен молча превращаться в другую корректную команду.

---

# 14. Обнаружение Syntax Errors

Parser может использовать простые правила.

## Правило 1 — Pipe должен соединять две команды

```text
COMMAND PIPE COMMAND
```

Поэтому некорректны:

```text
PIPE
```

```text
PIPE COMMAND
```

```text
COMMAND PIPE
```

```text
COMMAND PIPE PIPE COMMAND
```

---

## Правило 2 — Redirection должен иметь WORD после себя

Для:

```text
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
```

следующий токен должен быть:

```text
WORD
```

Например:

```text
WORD REDIR_OUT WORD
```

корректно.

А:

```text
WORD REDIR_OUT PIPE
```

некорректно.

---

# 14.1 Пример

Input:

```bash
echo hello > | grep
```

Tokens:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
PIPE("|")
WORD("grep")
```

Parser видит:

```text
REDIR_OUT
    ↓
ожидался WORD
    ↓
получен PIPE
```

Результат:

```text
Syntax Error
```

---

# 14.2 Другой пример

```bash
echo hello | >
```

Токены:

```text
WORD
WORD
PIPE
REDIR_OUT
```

После `PIPE` должна начинаться команда.

Здесь структура незавершённая, а `REDIR_OUT` также требует `WORD` после себя.

Следовательно:

```text
SYNTAX ERROR
```

Главная идея:

> Parser проверяет последовательность Token Types, а не просто отдельные символы.

---

# 15. Exit Status

Syntax Error должен приводить к ненулевому exit status.

Для Bash-подобного поведения syntax errors обычно используют:

```text
exit status = 2
```

Например:

```bash
echo hello |
```

приводит к Syntax Error.

После этого:

```bash
echo $?
```

должен показать соответствующий код ошибки.

В Minishell необходимо правильно обновлять состояние `last exit status`.

---

# 15.1 Почему Exit Status важен

Shell хранит результат предыдущей команды в:

```bash
$?
```

Например:

```bash
echo hello |
```

→ Syntax Error.

Затем:

```bash
echo $?
```

должен вернуть ненулевой код, соответствующий ошибке синтаксиса.

Следовательно, Syntax Validation должна не только вывести сообщение об ошибке.

Она также должна:

```text
1. обнаружить ошибку
2. остановить выполнение
3. установить правильный exit status
4. вернуть управление shell loop
```

---

# 16. Parser State Machine

Очень полезно представить Parser как State Machine.

Например:

```text
EXPECT_COMMAND
EXPECT_WORD
EXPECT_ELEMENT
```

---

## State 1 — EXPECT_COMMAND

В начале Parser ожидает начало команды.

Обычно:

```text
WORD
```

Например:

```bash
ls
```

корректно.

---

## State 2 — После WORD

После `WORD` Parser может встретить:

```text
WORD
PIPE
REDIRECTION
EOF
```

Например:

```bash
echo hello
```

или:

```bash
echo hello | grep
```

или:

```bash
echo hello > file
```

---

## State 3 — После PIPE

После:

```text
PIPE
```

Parser ожидает новую команду.

Например:

```bash
ls | cat
```

корректно.

Но:

```bash
ls | |
```

некорректно.

---

## State 4 — После REDIRECTION

После:

```text
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
```

Parser ожидает:

```text
WORD
```

Например:

```text
REDIR_OUT WORD
```

или:

```text
HEREDOC WORD
```

---

# 16.1 State Diagram

Упрощённо:

```text
             +----------------+
             | EXPECT_COMMAND |
             +----------------+
                     |
                    WORD
                     |
                     v
             +----------------+
             | EXPECT_ELEMENT |
             +----------------+
              /      |       \
             /       |        \
          WORD      PIPE      REDIR
           |         |          |
           |         |          v
           |         |      EXPECT_WORD
           |         |          |
           |         |         WORD
           |         |          |
           |         |          v
           |         +------> EXPECT_ELEMENT
           |
           +--------------------------+
```

Главная идея:

```text
После PIPE → ожидаем новую command.

После REDIRECTION → ожидаем WORD.

После WORD → можем получить WORD, PIPE, REDIRECTION или EOF.
```

---

# 17. Алгоритм Syntax Validation

Упрощённый алгоритм:

```text
1. Получить первый token.

2. Если tokens отсутствуют:
       return SUCCESS.

3. Если первый token == PIPE:
       SYNTAX ERROR.

4. Пока есть tokens:

       Если current == WORD:
           перейти к следующему token.

       Если current == REDIR_IN:
           следующий token должен быть WORD.

       Если current == REDIR_OUT:
           следующий token должен быть WORD.

       Если current == APPEND:
           следующий token должен быть WORD.

       Если current == HEREDOC:
           следующий token должен быть WORD.

       Если current == PIPE:
           следующий token должен начинать новую command.

5. Если последний token == PIPE:
       SYNTAX ERROR.

6. Если последний token == REDIRECTION:
       SYNTAX ERROR.

7. Иначе:
       SYNTAX VALID.
```

---

# 17.1 Pseudocode

Упрощённая реализация:

```c
int validate_syntax(t_token *tokens)
{
    t_token *current;

    current = tokens;

    if (!current)
        return (0);

    if (current->type == TOKEN_PIPE)
        return (syntax_error(current));

    while (current)
    {
        if (is_redirection(current->type))
        {
            if (!current->next
                || current->next->type != TOKEN_WORD)
                return (syntax_error(current));
        }

        if (current->type == TOKEN_PIPE)
        {
            if (!current->next
                || current->next->type == TOKEN_PIPE)
                return (syntax_error(current));
        }

        current = current->next;
    }

    return (0);
}
```

Это только упрощённый пример.

В реальном проекте Parser, скорее всего, будет тесно связан с построением command structure.

---

# 18. Примеры

## Valid Example 1

```bash
echo hello
```

Tokens:

```text
WORD
WORD
EOF
```

Результат:

```text
VALID
```

---

## Valid Example 2

```bash
ls | grep txt
```

Tokens:

```text
WORD
PIPE
WORD
WORD
EOF
```

Результат:

```text
VALID
```

---

## Valid Example 3

```bash
cat < input.txt
```

Tokens:

```text
WORD
REDIR_IN
WORD
EOF
```

Результат:

```text
VALID
```

---

## Valid Example 4

```bash
echo hello > output.txt
```

Tokens:

```text
WORD
WORD
REDIR_OUT
WORD
EOF
```

Результат:

```text
VALID
```

---

## Valid Example 5

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD
WORD
APPEND
WORD
EOF
```

Результат:

```text
VALID
```

---

## Valid Example 6

```bash
cat << EOF
```

Tokens:

```text
WORD
HEREDOC
WORD
EOF
```

Результат:

```text
VALID
```

---

# 18.1 Invalid Examples

## Pipe в начале

```bash
| echo hello
```

Tokens:

```text
PIPE
WORD
WORD
```

Результат:

```text
SYNTAX ERROR
```

---

## Pipe в конце

```bash
echo hello |
```

Tokens:

```text
WORD
WORD
PIPE
```

Результат:

```text
SYNTAX ERROR
```

---

## Два Pipe подряд

```bash
echo hello | | grep
```

Tokens:

```text
WORD
WORD
PIPE
PIPE
WORD
```

Результат:

```text
SYNTAX ERROR
```

---

## Redirection без filename

```bash
echo hello >
```

Tokens:

```text
WORD
WORD
REDIR_OUT
```

Результат:

```text
SYNTAX ERROR
```

---

## Input без filename

```bash
cat <
```

Tokens:

```text
WORD
REDIR_IN
```

Результат:

```text
SYNTAX ERROR
```

---

## Heredoc без delimiter

```bash
cat <<
```

Tokens:

```text
WORD
HEREDOC
```

Результат:

```text
SYNTAX ERROR
```

---

## Redirection перед Pipe

```bash
echo hello > | cat
```

Tokens:

```text
WORD
WORD
REDIR_OUT
PIPE
WORD
```

После `REDIR_OUT` ожидался:

```text
WORD
```

получен:

```text
PIPE
```

Результат:

```text
SYNTAX ERROR
```

---

# 18.2 Сложный корректный пример

```bash
cat < input.txt | grep "hello world" >> result.txt
```

Tokens:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")

PIPE

WORD("grep")
WORD("hello world")

APPEND(">>")
WORD("result.txt")
```

Структура:

```text
             PIPE
            /    \
           /      \
        cat        grep
         |           |
      < input     "hello world"
                       |
                  >> result.txt
```

Результат:

```text
VALID
```

---

# 19. Типичные ошибки

## Ошибка 1 — Проверять raw characters вместо tokens

Не стоит строить Syntax Validation только на:

```c
if (input[i] == '|')
```

Lexer уже должен определить:

```text
TOKEN_PIPE
```

Parser должен работать с:

```text
TOKEN_PIPE
```

а не с сырыми символами.

---

# Ошибка 2 — Считать quoted operators ошибкой

Например:

```bash
echo "|"
```

Это не Pipe.

Lexer должен создать:

```text
WORD("|")
```

Parser вообще не должен увидеть:

```text
PIPE
```

---

# Ошибка 3 — Забыть проверять target после redirection

Некорректно:

```bash
echo hello >
```

Потому что:

```text
REDIR_OUT
```

требует:

```text
WORD
```

после себя.

---

# Ошибка 4 — Запрещать несколько redirections

Это:

```bash
cat < input > output
```

синтаксически корректно.

Не нужно автоматически отклонять несколько redirections.

---

# Ошибка 5 — Считать отсутствие пробелов ошибкой

Это:

```bash
echo hello>file
```

корректно.

Lexer должен создать:

```text
WORD
WORD
REDIR_OUT
WORD
```

---

# Ошибка 6 — Путать Syntax Error и Execution Error

Рассмотрим:

```bash
cat < missing_file
```

Синтаксис корректен.

Файл просто не существует.

Это не Syntax Error.

А:

```bash
cat <
```

синтаксически некорректно.

Разница:

```text
cat < missing_file
        |
        +-- Syntax OK
        +-- Execution/Redirection error

cat <
    |
    +-- Syntax Error
```

Также:

```bash
some_nonexistent_command
```

может быть синтаксически корректной командой, но привести к:

```text
command not found
```

Это уже Execution Error.

---

# 20. Структура реализации

Хорошая структура проекта:

```text
src/
├── lexer/
│   ├── lexer.c
│   ├── token.c
│   ├── quotes.c
│   └── operators.c
│
├── parser/
│   ├── parser.c
│   ├── syntax.c
│   ├── command.c
│   └── redirection.c
│
├── expansion/
│   └── ...
│
├── execution/
│   └── ...
│
└── main.c
```

Полезные функции:

```c
int     validate_syntax(t_token *tokens);
int     is_redirection(int type);
int     is_pipe(int type);
int     expect_word(t_token *token);
int     syntax_error(t_token *token);
```

---

# 20.1 Helper для Redirections

Можно сделать:

```c
int is_redirection(int type)
{
    return (
        type == TOKEN_REDIR_IN
        || type == TOKEN_REDIR_OUT
        || type == TOKEN_APPEND
        || type == TOKEN_HEREDOC
    );
}
```

Тогда Parser становится проще:

```c
if (is_redirection(current->type))
{
    if (!current->next
        || current->next->type != TOKEN_WORD)
        return (syntax_error(current));
}
```

---

# 21. Стратегия тестирования

Не стоит тестировать только:

```bash
echo hello
```

Создайте группы тестов.

---

## Group 1 — Basic Commands

```bash
echo
echo hello
ls
pwd
```

---

## Group 2 — Pipes

```bash
ls | cat
ls | grep txt
cat file | grep hello | wc -l
```

---

## Group 3 — Invalid Pipes

```bash
|
| ls
ls |
ls | |
ls | | cat
```

---

## Group 4 — Redirections

```bash
cat < file
echo hello > file
echo hello >> file
cat << EOF
```

---

## Group 5 — Invalid Redirections

```bash
cat <
echo >
echo >>
cat <<
```

---

## Group 6 — Redirection + Pipe

```bash
cat < file | grep hello
cat file | grep hello > result
cat < input | grep hello >> output
```

---

## Group 7 — Quotes

```bash
echo "|"
echo "<"
echo ">"
echo "hello | world"
echo 'hello > world'
```

Эти операторы не должны превращаться в operator tokens.

---

## Group 8 — Operators Without Spaces

```bash
cat<file
cat>file
cat>>file
cat<<EOF
echo hello|grep hello
```

---

# 21.1 Сравнение с Bash

Очень полезная техника:

```text
Input
  ↓
Bash
  ↓
Expected

Input
  ↓
Minishell
  ↓
Actual
```

Сравнивайте:

* синтаксические ошибки;
* exit status;
* выполнение команды;
* redirections;
* pipes;
* quotes;
* поведение `$?`.

Особенно важно проверять:

```bash
echo $?
```

после Syntax Error.

---

# 22. Checklist

Перед тем как считать тему **Syntax Validation** изученной, убедитесь, что вы понимаете:

## Basic

* [ ] Что такое Syntax Validation.
* [ ] Зачем она нужна.
* [ ] Почему она относится к Parser.
* [ ] Разницу между Lexer и Parser.
* [ ] Что такое valid token sequence.
* [ ] Что такое invalid token sequence.

---

## Pipes

* [ ] Pipe не может находиться в начале команды.
* [ ] Pipe не может находиться в конце команды.
* [ ] Два Pipe подряд являются ошибкой для базовой грамматики Minishell.
* [ ] Pipe соединяет две команды.
* [ ] Несколько Pipe подряд в корректной цепочке допустимы, если между ними находятся команды.

---

## Redirections

* [ ] `<` требует `WORD` после себя.
* [ ] `>` требует `WORD`.
* [ ] `>>` требует `WORD`.
* [ ] `<<` требует `WORD` delimiter.
* [ ] Несколько redirections могут быть допустимыми.
* [ ] Redirections не требуют пробелов.

---

## Quotes

* [ ] Operators внутри quotes становятся частью WORD.
* [ ] Quotes учитываются во время lexical analysis.
* [ ] Незакрытые quotes должны обнаруживаться.
* [ ] Пробелы внутри quotes не разделяют tokens.

---

## Errors

* [ ] Что такое Syntax Error.
* [ ] Что такое Execution Error.
* [ ] Что такое Command Not Found.
* [ ] Как работает exit status.
* [ ] Почему после Syntax Error команда не должна выполняться.

---

# 23. Вопросы для проверки знаний

Попробуйте ответить на вопросы без подсказки.

## Basic

1. Что такое Syntax Validation?
2. Зачем она нужна в Minishell?
3. Чем Lexer отличается от Parser?
4. Кто должен обнаружить:

```bash
echo hello >
```

5. Почему?

---

## Pipes

6. Корректна ли команда:

```bash
ls | cat
```

7. Корректна ли:

```bash
| ls
```

8. Корректна ли:

```bash
ls |
```

9. Корректна ли:

```bash
ls | | cat
```

10. Почему?

---

## Redirections

11. Корректно ли:

```bash
cat < input
```

12. Корректно ли:

```bash
cat <
```

13. Корректно ли:

```bash
echo hello > output
```

14. Корректно ли:

```bash
echo hello >
```

15. Корректно ли:

```bash
echo hello >> output
```

16. Корректно ли:

```bash
cat << EOF
```

17. Что такое `EOF` в этом случае?

---

## Quotes

18. Какие токены должны получиться из:

```bash
echo "|"
```

19. Является ли `|` Pipe?

20. Что получится из:

```bash
echo "hello > world"
```

21. Почему `>` не становится `REDIR_OUT`?

---

## Syntax vs Execution

22. Является ли это Syntax Error?

```bash
cat < missing_file
```

23. А это?

```bash
cat <
```

24. В чём разница между этими двумя случаями?

---

# 24. Главная модель

Запомните этот pipeline:

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
                        v
              +-------------------+
              | SYNTAX VALIDATION |
              +-------------------+
                        |
                Valid? | Invalid?
                    /        \
                   /          \
                  v            v
              PARSER       SYNTAX ERROR
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

Главное различие:

```text
LEXER
"Какие элементы находятся в input?"

PARSER
"Образуют ли эти элементы корректную структуру?"

EXECUTOR
"Как выполнить эту структуру?"
```

---

# Самые важные правила

## Rule 1 — Command начинается с WORD

Базовое начало команды:

```text
WORD
```

---

## Rule 2 — PIPE соединяет команды

```text
COMMAND PIPE COMMAND
```

Поэтому:

```text
PIPE
```

в начале или конце является ошибкой.

---

## Rule 3 — Redirection требует WORD

```text
REDIR_IN  WORD
REDIR_OUT WORD
APPEND    WORD
HEREDOC   WORD
```

Поэтому:

```bash
echo >
```

некорректно.

---

## Rule 4 — Syntax ≠ Execution

```bash
cat < missing_file
```

имеет корректный синтаксис.

Но файл может отсутствовать.

А:

```bash
cat <
```

имеет некорректный синтаксис.

Разница:

```text
Syntax Error
    ↓
Parser

Redirection/File Error
    ↓
Execution / Redirection

Command Not Found
    ↓
Execution
```

---

# Итоговая архитектура Minishell

```text
USER INPUT
    |
    v
+----------+
|  LEXER   |
+----------+
    |
    | creates
    v
+----------+
|  TOKENS  |
+----------+
    |
    | validates
    v
+----------+
|  PARSER  |
+----------+
    |
    | creates
    v
+------------------+
| COMMAND STRUCTURE|
+------------------+
    |
    v
+-------------+
| EXPANSION   |
+-------------+
    |
    v
+-------------+
| REDIRECTIONS|
+-------------+
    |
    v
+-------------+
| EXECUTION   |
+-------------+
```

Главная идея:

> **Syntax Validation не позволяет некорректной последовательности токенов дойти до Execution.**

Например:

```bash
echo hello |
```

должно пройти:

```text
Lexer
  ↓
Tokens
  ↓
Parser
  ↓
SYNTAX ERROR
  ↓
STOP
```

а не:

```text
Lexer
  ↓
Tokens
  ↓
Parser
  ↓
Execution
```

Если вы понимаете, как проверять `PIPE`, `<`, `>`, `<<`, `>>`, отсутствие аргументов после redirection, неправильное расположение Pipe, quotes и разницу между Syntax Error и Execution Error — вы освоили основу **Syntax Validation** в Minishell.

Следующий логичный этап:

```text
TOKENS
   ↓
SYNTAX VALIDATION
   ↓
PARSER
   ↓
COMMAND STRUCTURE / AST
```

То есть теперь нужно научиться **превращать валидный список токенов в структуру команд, которую затем сможет выполнить Minishell**.
