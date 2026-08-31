# Lexical Analysis — Лексический анализ Shell

## Содержание

* [1. Что такое Lexical Analysis](#1-что-такое-lexical-analysis)
* [2. Задача Lexer в Minishell](#2-задача-lexer-в-minishell)
* [3. Lexer Pipeline](#3-lexer-pipeline)
* [4. Что такое Token](#4-что-такое-token)
* [5. Типы Tokens](#5-типы-tokens)
* [6. Words](#6-words)
* [7. Operators](#7-operators)
* [8. Pipes](#8-pipes)
* [9. Redirections](#9-redirections)
* [10. Quotes](#10-quotes)
* [11. Single Quotes](#11-single-quotes)
* [12. Double Quotes](#12-double-quotes)
* [13. Смешивание Quotes и Text](#13-смешивание-quotes-и-text)
* [14. Spaces](#14-spaces)
* [15. Environment Variables](#15-environment-variables)
* [16. Lexer не выполняет Expansion](#16-lexer-не-выполняет-expansion)
* [17. Syntax Errors](#17-syntax-errors)
* [18. Lexer State Machine](#18-lexer-state-machine)
* [19. Примеры Tokenization](#19-примеры-tokenization)
* [20. Структура Token](#20-структура-token)
* [21. Алгоритм Lexer](#21-алгоритм-lexer)
* [22. Псевдокод](#22-псевдокод)
* [23. Типичная архитектура Minishell](#23-типичная-архитектура-minishell)
* [24. Частые ошибки](#24-частые-ошибки)
* [25. Checklist для команды](#25-checklist-для-команды)
* [26. Контрольные вопросы](#26-контрольные-вопросы)
* [27. Главная идея](#27-главная-идея)

---

# 1. Что такое Lexical Analysis

**Lexical Analysis** — это процесс преобразования обычной строки, которую ввёл пользователь, в набор структурированных **tokens**.

Например, пользователь вводит:

```bash
echo hello | grep hello
```

Lexer превращает это в:

```text
WORD      "echo"
WORD      "hello"
PIPE      "|"
WORD      "grep"
WORD      "hello"
```

То есть:

```text
Raw Input
    |
    v
"echo hello | grep hello"
    |
    v
   LEXER
    |
    v
TOKENS
```

Lexer отвечает на вопрос:

> **Из каких элементов состоит команда?**

Он **не отвечает** на вопрос:

> Что делать с этими элементами?

Это уже задача Parser и Execution.

---

# 2. Задача Lexer в Minishell

В `minishell` Lexer должен распознавать специальные элементы Shell.

Например:

```text
|
<
>
<<
>>
'
"
```

а также обычные слова:

```text
ls
hello
file.txt
/usr/bin
$USER
```

Основная задача:

```text
Input string
     |
     v
Recognize tokens
     |
     v
Create token list
```

Например:

```bash
cat < input.txt | grep hello > output.txt
```

становится:

```text
WORD       cat
REDIR_IN   <
WORD       input.txt
PIPE       |
WORD       grep
WORD       hello
REDIR_OUT  >
WORD       output.txt
```

---

# 3. Lexer Pipeline

Общий процесс:

```text
                INPUT
                  |
                  v
        +-------------------+
        |   Character #0    |
        +-------------------+
                  |
                  v
        +-------------------+
        | Determine token   |
        +-------------------+
                  |
                  v
        +-------------------+
        | Read token        |
        +-------------------+
                  |
                  v
        +-------------------+
        | Save token        |
        +-------------------+
                  |
                  v
             Next char
                  |
                  v
              ...
                  |
                  v
             TOKEN LIST
```

После Lexer:

```text
TOKEN LIST
    |
    v
Parser
```

---

# 4. Что такое Token

**Token** — это отдельный логический элемент команды.

Например:

```bash
echo hello > file.txt
```

содержит:

```text
echo
hello
>
file.txt
```

Но Shell должен знать не только значение, но и **тип** каждого элемента.

Например:

```text
"echo"      -> WORD
"hello"     -> WORD
">"         -> REDIR_OUT
"file.txt"  -> WORD
```

---

# 4.1 Пример структуры

Можно создать:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

Например:

```text
+---------+----------+
| value   | type     |
+---------+----------+
| echo    | WORD     |
| hello   | WORD     |
| |       | PIPE     |
| grep    | WORD     |
| hello   | WORD     |
+---------+----------+
```

---

# 5. Типы Tokens

Обычно для `minishell` достаточно выделить:

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

Соответствие:

| Символ        | Token Type  | Значение           |          |
| ------------- | ----------- | ------------------ | -------- |
| обычный текст | `WORD`      | слово/аргумент     |          |
| `             | `           | `PIPE`             | pipeline |
| `<`           | `REDIR_IN`  | input redirection  |          |
| `>`           | `REDIR_OUT` | output redirection |          |
| `<<`          | `HEREDOC`   | heredoc            |          |
| `>>`          | `APPEND`    | append             |          |

---

# 6. Words

Самый распространённый token — `WORD`.

Например:

```bash
ls
```

получаем:

```text
WORD "ls"
```

---

## 6.1 Несколько слов

```bash
ls -la file.txt
```

получаем:

```text
WORD "ls"
WORD "-la"
WORD "file.txt"
```

---

## 6.2 Word не обязательно означает одно физическое слово

Это очень важно.

Например:

```bash
echo "hello world"
```

должно создать:

```text
WORD "echo"
WORD "hello world"
```

а не:

```text
WORD "echo"
WORD "hello"
WORD "world"
```

Quotes влияют на tokenization.

---

# 7. Operators

Shell имеет специальные операторы.

Основные:

```text
|
<
>
<<
>>
```

Они не являются обычными `WORD`.

Например:

```bash
cat < file
```

получаем:

```text
WORD      "cat"
REDIR_IN  "<"
WORD      "file"
```

---

# 8. Pipes

Pipe:

```bash
command1 | command2
```

создаёт два command tokens:

```text
WORD "command1"
PIPE "|"
WORD "command2"
```

Например:

```bash
ls | grep txt
```

Lexer создаёт:

```text
WORD "ls"
PIPE "|"
WORD "grep"
WORD "txt"
```

---

# 8.1 Pipe без пробелов

Важно:

```bash
ls|grep txt
```

и:

```bash
ls | grep txt
```

должны дать одинаковые tokens:

```text
WORD "ls"
PIPE "|"
WORD "grep"
WORD "txt"
```

Поэтому lexer должен распознавать `|` независимо от пробелов.

---

# 9. Redirections

## `<`

```bash
cat < input.txt
```

Tokens:

```text
WORD      "cat"
REDIR_IN  "<"
WORD      "input.txt"
```

---

## `>`

```bash
echo hello > output.txt
```

Tokens:

```text
WORD       "echo"
WORD       "hello"
REDIR_OUT  ">"
WORD       "output.txt"
```

---

## `>>`

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD       "echo"
WORD       "hello"
APPEND     ">>"
WORD       "output.txt"
```

---

## `<<`

```bash
cat << EOF
```

Tokens:

```text
WORD       "cat"
HEREDOC    "<<"
WORD       "EOF"
```

---

# 10. Quotes

Quotes — одна из самых важных частей Lexer.

Есть два основных типа:

```text
'
"
```

Они работают по-разному.

---

# 11. Single Quotes

Single quotes:

```bash
'text'
```

означают:

> воспринимать содержимое буквально.

Например:

```bash
echo '$USER'
```

Lexer должен сохранить содержимое как часть одного `WORD`.

Концептуально:

```text
WORD "$USER"
```

Expansion позже должен понять, что `$USER` находится внутри single quotes и не должен расширяться.

---

# 11.1 Важный момент

Lexer и Expansion — разные этапы.

Lexer должен определить:

```text
где находятся quotes
```

а Expansion позже решает:

```text
нужно ли расширять $USER
```

---

# 12. Double Quotes

Double quotes:

```bash
"hello world"
```

объединяют содержимое в один word.

Например:

```bash
echo "hello world"
```

получаем:

```text
WORD "echo"
WORD "hello world"
```

---

# 12.1 Variable внутри Double Quotes

```bash
echo "$USER"
```

Lexer должен понимать, что:

```text
"$USER"
```

является одним логическим словом.

Позже Expansion может превратить:

```text
"$USER"
```

в:

```text
"alice"
```

если:

```text
USER=alice
```

---

# 13. Смешивание Quotes и Text

Очень важный пример:

```bash
echo hello"world"
```

Это не два аргумента.

Это:

```text
helloworld
```

То есть:

```text
WORD "helloworld"
```

---

## Другой пример

```bash
echo "hello"'world'
```

также:

```text
WORD "helloworld"
```

---

## Ещё пример

```bash
echo abc"def"ghi
```

результат:

```text
WORD "abcdefghi"
```

Это означает:

> Quotes не обязательно отделяют отдельный token.

Они могут быть частью одного `WORD`.

---

# 14. Spaces

Пробел обычно разделяет words.

Например:

```bash
echo hello world
```

становится:

```text
WORD "echo"
WORD "hello"
WORD "world"
```

---

# 14.1 Несколько пробелов

```bash
echo      hello
```

всё равно:

```text
WORD "echo"
WORD "hello"
```

---

# 14.2 Spaces внутри Quotes

```bash
echo "hello     world"
```

получаем:

```text
WORD "echo"
WORD "hello     world"
```

Пробелы внутри quotes не разделяют word.

---

# 15. Environment Variables

Lexer должен уметь встретить:

```bash
$USER
```

но не обязательно сразу выполнять expansion.

Например:

```bash
echo $USER
```

можно представить как:

```text
WORD "echo"
WORD "$USER"
```

Затем Expansion:

```text
"$USER"
    |
    v
"alice"
```

---

# 15.1 Почему нельзя делать Expansion прямо в Lexer?

Потому что Lexer отвечает за структуру текста.

Например:

```bash
echo '$USER'
```

и:

```bash
echo "$USER"
```

на этапе Lexer должны сохранять информацию о quotes.

Если выполнить expansion слишком рано, можно потерять информацию:

```text
'$USER'
```

и:

```text
"$USER"
```

нельзя обрабатывать одинаково.

Поэтому лучше концептуально разделять:

```text
Lexer
   |
   v
Tokens + quote information
   |
   v
Parser
   |
   v
Expansion
```

---

# 16. Lexer не выполняет Expansion

Это важный принцип архитектуры.

### Lexer:

```text
"echo $USER"
```

↓

```text
WORD "echo"
WORD "$USER"
```

### Expansion:

```text
$USER
```

↓

```text
alice
```

---

# 17. Syntax Errors

Lexer должен уметь обнаруживать некоторые некорректные ситуации.

Например, незакрытые quotes:

```bash
echo "hello
```

Здесь:

```text
"
```

не имеет закрывающей пары.

Shell должен это корректно обработать.

---

# 17.1 Unclosed Single Quote

```bash
echo 'hello
```

Также проблема:

```text
'
```

не закрыта.

---

# 17.2 Что с Pipe?

Например:

```bash
echo hello |
```

Сам tokenization может получить:

```text
WORD "echo"
WORD "hello"
PIPE "|"
```

Но это уже потенциальная **syntax error**, которую должен определить следующий этап — parser/syntax checker.

Важно не смешивать:

```text
lexical error
```

и:

```text
syntax error
```

---

# 18. Lexer State Machine

Один из лучших способов понять Lexer — представить его как **Finite State Machine**.

Основные состояния:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

---

# 18.1 NORMAL

В обычном состоянии:

```text
NORMAL
```

Shell может встретить:

```text
space
'
"
|
<
>
```

Например:

```text
echo "hello"
     ^
```

Lexer переходит:

```text
NORMAL
   |
   | "
   v
DOUBLE_QUOTE
```

---

# 18.2 SINGLE_QUOTE

Внутри:

```text
'hello world'
```

Lexer находится:

```text
SINGLE_QUOTE
```

Пока не встретит:

```text
'
```

Он воспринимает содержимое как обычный текст.

---

# 18.3 DOUBLE_QUOTE

Внутри:

```text
"hello $USER"
```

Lexer находится:

```text
DOUBLE_QUOTE
```

и выходит из него при следующей:

```text
"
```

---

# 18.4 State Diagram

```text
                 "
        +-------------------+
        |                   v
    +--------+          +----------+
    | NORMAL |          |  DOUBLE  |
    +--------+          |  QUOTE   |
        |               +----------+
        | '                  |
        v                    | "
 +-------------+             |
 | SINGLE      |-------------+
 | QUOTE       |
 +-------------+
       |
       | '
       |
       v
    NORMAL
```

Более подробно:

```text
NORMAL
  |
  +-- ' --> SINGLE_QUOTE
  |             |
  |             +-- ' --> NORMAL
  |
  +-- " --> DOUBLE_QUOTE
                |
                +-- " --> NORMAL
```

---

# 19. Примеры Tokenization

## Example 1

Input:

```bash
echo hello
```

Tokens:

```text
WORD "echo"
WORD "hello"
```

---

## Example 2

Input:

```bash
echo hello world
```

Tokens:

```text
WORD "echo"
WORD "hello"
WORD "world"
```

---

## Example 3

Input:

```bash
ls | grep txt
```

Tokens:

```text
WORD "ls"
PIPE "|"
WORD "grep"
WORD "txt"
```

---

## Example 4

Input:

```bash
cat < input.txt
```

Tokens:

```text
WORD "cat"
REDIR_IN "<"
WORD "input.txt"
```

---

## Example 5

Input:

```bash
echo hello > output.txt
```

Tokens:

```text
WORD "echo"
WORD "hello"
REDIR_OUT ">"
WORD "output.txt"
```

---

## Example 6

Input:

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD "echo"
WORD "hello"
APPEND ">>"
WORD "output.txt"
```

---

## Example 7

Input:

```bash
cat << EOF
```

Tokens:

```text
WORD "cat"
HEREDOC "<<"
WORD "EOF"
```

---

## Example 8

Input:

```bash
echo "hello world"
```

Tokens:

```text
WORD "echo"
WORD "hello world"
```

---

## Example 9

Input:

```bash
echo '$USER'
```

Tokens:

```text
WORD "echo"
WORD "$USER"
```

Но token должен сохранить информацию о single quotes.

---

## Example 10

Input:

```bash
echo "$USER"
```

Tokens:

```text
WORD "echo"
WORD "$USER"
```

Но здесь quote context другой.

Поэтому:

> одинаковое текстовое значение не означает одинаковое семантическое значение.

---

## Example 11

Input:

```bash
echo hello"world"
```

Tokens:

```text
WORD "helloworld"
```

---

## Example 12

Input:

```bash
echo "hello"'world'
```

Tokens:

```text
WORD "helloworld"
```

---

## Example 13

Input:

```bash
echo abc"123"def
```

Tokens:

```text
WORD "abc123def"
```

---

# 20. Структура Token

Простейший вариант:

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

Затем:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

---

# 20.1 Нужно ли хранить Quotes?

Есть несколько вариантов архитектуры.

### Вариант 1

Хранить исходные quotes:

```text
value = "\"hello world\""
```

и обрабатывать их позже.

### Вариант 2

Удалять quotes во время lexer и хранить дополнительную информацию.

### Вариант 3

Создать более подробную структуру, которая хранит части word и их quote context.

Например концептуально:

```text
WORD
 |
 +-- TEXT: hello
 |
 +-- DOUBLE_QUOTED: world
 |
 +-- TEXT: test
```

Для сложного Shell это может быть удобнее.

Главное:

> Нельзя потерять информацию, которая понадобится для правильного Expansion и удаления quotes.

---

# 21. Алгоритм Lexer

Общий алгоритм:

```text
1. Начать с первого символа.
2. Пропускать пробелы.
3. Проверить специальные операторы.
4. Если найден pipe — создать PIPE token.
5. Если найден < или > — определить одинарный или двойной оператор.
6. Если найден quote — войти в соответствующий quote state.
7. Если найден обычный текст — продолжать собирать WORD.
8. Остановиться на пробеле или operator.
9. Создать WORD token.
10. Перейти к следующему символу.
11. Повторять до конца строки.
```

---

# 21.1 Главный принцип

Lexer читает:

```text
character by character
```

Например:

```bash
echo "hello world" | grep hello
```

По символам:

```text
e
c
h
o
space
"
h
e
l
l
o
space
w
o
r
l
d
"
space
|
space
g
r
e
p
...
```

Но группирует их в tokens:

```text
echo
hello world
|
grep
hello
```

---

# 22. Псевдокод

Общий псевдокод:

```text
while current character exists:

    skip spaces

    if current == '|':
        create PIPE token
        move forward

    else if current == '<':
        if next == '<':
            create HEREDOC token
            move 2 characters
        else:
            create REDIR_IN token
            move 1 character

    else if current == '>':
        if next == '>':
            create APPEND token
            move 2 characters
        else:
            create REDIR_OUT token
            move 1 character

    else:
        read WORD

        while current is not:
            space
            pipe
            redirection
            end of string

            if current == '\'':
                process single quote

            else if current == '"':
                process double quote

            else:
                add character to word

        create WORD token
```

---

# 22.1 Чтение WORD

Представим:

```bash
echo abc"hello"xyz
```

Lexer начинает:

```text
a
b
c
```

затем встречает:

```text
"
```

переходит в:

```text
DOUBLE_QUOTE
```

читает:

```text
hello
```

выходит из quotes:

```text
"
```

затем продолжает:

```text
xyz
```

В результате:

```text
WORD "abchelloxyz"
```

---

# 23. Типичная архитектура Minishell

Хорошая архитектура может выглядеть так:

```text
                     readline()
                         |
                         v
                     raw string
                         |
                         v
                     +-------+
                     | Lexer |
                     +-------+
                         |
                         v
                      tokens
                         |
                         v
                     +--------+
                     | Parser |
                     +--------+
                         |
                         v
                 command structure
                         |
                         v
                    Expansion
                         |
                         v
                    Execution
```

---

# 23.1 Lexer отвечает только за Lexing

Не стоит помещать в Lexer всё подряд.

Lexer:

```text
✔ распознаёт tokens
✔ понимает quotes
✔ распознаёт operators
✔ создаёт token list
```

Lexer не должен заниматься:

```text
✘ fork()
✘ execve()
✘ pipe()
✘ waitpid()
✘ запуском commands
```

---

# 24. Частые ошибки

## Ошибка 1 — Использовать `split()` по пробелам

Например:

```c
split(line, ' ');
```

не работает корректно для Shell.

Потому что:

```bash
echo "hello world"
```

должно дать:

```text
echo
hello world
```

а не:

```text
echo
"hello
world"
```

---

# Ошибка 2 — Игнорировать operators без spaces

Нужно правильно обработать:

```bash
ls|grep txt
```

а не только:

```bash
ls | grep txt
```

---

# Ошибка 3 — Неправильно обрабатывать `>>`

Нельзя получить:

```text
REDIR_OUT ">"
REDIR_OUT ">"
```

если Shell должен распознать:

```text
APPEND ">>"
```

---

# Ошибка 4 — Неправильно обрабатывать `<<`

Аналогично:

```text
<<
```

должен быть одним operator:

```text
HEREDOC
```

---

# Ошибка 5 — Разделять word на quotes

Например:

```bash
echo hello"world"
```

не должно стать:

```text
WORD "hello"
WORD "world"
```

Это:

```text
WORD "helloworld"
```

---

# Ошибка 6 — Удалить quotes слишком рано

Если удалить всю информацию о quotes в Lexer, Expansion может не понять разницу между:

```bash
'$USER'
```

и:

```bash
"$USER"
```

---

# Ошибка 7 — Делать Expansion в Lexer

Lexer должен определить структуру:

```text
$USER
```

но не обязательно заменять её сразу.

Лучше:

```text
Lexer
  |
  v
"$USER"
  |
  v
Expansion
  |
  v
"alice"
```

---

# Ошибка 8 — Не учитывать пустые quotes

Например:

```bash
echo ""
```

`""` представляет пустой аргумент.

Это отличается от отсутствия аргумента.

То есть:

```bash
echo ""
```

имеет:

```text
argv[0] = "echo"
argv[1] = ""
```

Это очень важный edge case.

---

# Ошибка 9 — Не различать quote states

Нужно отдельно отслеживать:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

Например:

```bash
echo "'"
```

и:

```bash
echo '"'
```

не должны приводить к одинаковому поведению.

---

# 25. Checklist для команды

Каждый участник команды должен понимать:

## Основы

* [ ] Что такое lexical analysis.
* [ ] Что такое lexer.
* [ ] Что такое token.
* [ ] Чем lexer отличается от parser.
* [ ] Почему нельзя просто использовать `split()`.

---

## Tokens

* [ ] `WORD`
* [ ] `PIPE`
* [ ] `REDIR_IN`
* [ ] `REDIR_OUT`
* [ ] `APPEND`
* [ ] `HEREDOC`

---

## Operators

* [ ] `|`
* [ ] `<`
* [ ] `>`
* [ ] `<<`
* [ ] `>>`

---

## Quotes

* [ ] Single quotes `'`.
* [ ] Double quotes `"`.
* [ ] Quotes внутри word.
* [ ] Несколько quotes подряд.
* [ ] Empty quotes.
* [ ] Unclosed quotes.

---

## Words

Понимать почему:

```bash
hello"world"
```

становится:

```text
helloworld
```

и почему:

```bash
"hello world"
```

остаётся одним word.

---

## State Machine

Понимать состояния:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

и переходы:

```text
NORMAL
  |
  +-- ' --> SINGLE_QUOTE
  |
  +-- " --> DOUBLE_QUOTE
```

---

# 26. Контрольные вопросы

Перед тем как считать Lexer завершённым, каждый участник должен уметь ответить:

### Общие вопросы

1. Что такое lexical analysis?
2. Что делает lexer?
3. Что такое token?
4. Чем lexer отличается от parser?
5. Почему нельзя использовать `split()` по пробелам?

### Operators

6. Какие операторы должен распознавать minishell?
7. Чем `<` отличается от `<<`?
8. Чем `>` отличается от `>>`?
9. Почему `>>` должен быть одним token?
10. Почему `ls|grep` и `ls | grep` должны давать одинаковые tokens?

### Quotes

11. Зачем нужны quote states?
12. Чем `'...'` отличается от `"..."`?
13. Почему:

```bash
echo "hello world"
```

имеет только один argument после `echo`?

14. Почему:

```bash
echo hello"world"
```

становится:

```text
helloworld
```

15. Что должно произойти с:

```bash
echo ""
```

16. Что происходит при:

```bash
echo "hello
```

### Expansion

17. Должен ли Lexer выполнять `$USER` expansion?
18. Почему нельзя потерять quote information?
19. Чем отличаются:

```bash
'$USER'
```

и:

```bash
"$USER"
```

---

# 27. Главная идея

Главное, что нужно запомнить:

> **Lexer не выполняет команду. Lexer понимает структуру текста.**

Например, пользователь вводит:

```bash
cat < input.txt | grep "$USER" >> result.txt
```

Lexer должен увидеть:

```text
WORD       "cat"
REDIR_IN   "<"
WORD       "input.txt"
PIPE       "|"
WORD       "grep"
WORD       "$USER"
APPEND     ">>"
WORD       "result.txt"
```

Дальше:

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
                     PARSER
                        |
                        v
                COMMAND STRUCTURE
                        |
                        v
                    EXPANSION
                        |
                        v
                  REDIRECTIONS
                        |
                        v
                     EXECUTION
```

---

# Ключевая ментальная модель

При написании Lexer постоянно задавайте себе вопрос:

> **"Что сейчас означает этот символ в текущем контексте?"**

Например:

```text
NORMAL:
    " -> начало DOUBLE_QUOTE
    ' -> начало SINGLE_QUOTE
    | -> PIPE
    < -> REDIR_IN / HEREDOC
    > -> REDIR_OUT / APPEND
    space -> конец WORD


SINGLE_QUOTE:
    всё обычный текст
    ' -> конец SINGLE_QUOTE


DOUBLE_QUOTE:
    всё обычный текст
    " -> конец DOUBLE_QUOTE
```

Именно **контекст + состояние Lexer** позволяют правильно обработать Shell syntax.

Если команда:

```bash
echo abc"hello world"def | grep 'hello world' > result.txt
```

правильно превращается в:

```text
WORD       "echo"
WORD       "abchello worlddef"
PIPE       "|"
WORD       "grep"
WORD       "hello world"
REDIR_OUT  ">"
WORD       "result.txt"
```

то Lexer уже выполняет свою основную работу правильно.

После этого Parser, Expansion и Execution могут работать с понятной структурой данных вместо сырой строки.
