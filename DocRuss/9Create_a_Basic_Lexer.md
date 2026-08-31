# Создание базового Lexer

## Содержание

* [1. Что такое Lexer?](#1-что-такое-lexer)
* [2. Где Lexer находится в архитектуре Shell?](#2-где-lexer-находится-в-архитектуре-shell)
* [3. Основная задача Lexer](#3-основная-задача-lexer)
* [4. Вход и выход Lexer](#4-вход-и-выход-lexer)
* [5. Какие символы должен распознавать Lexer](#5-какие-символы-должен-распознавать-lexer)
* [6. Состояния Lexer](#6-состояния-lexer)
* [7. Базовый алгоритм Lexer](#7-базовый-алгоритм-lexer)
* [8. Обработка пробелов](#8-обработка-пробелов)
* [9. Чтение WORD](#9-чтение-word)
* [10. Обработка операторов](#10-обработка-операторов)
* [11. Обработка одинарных кавычек](#11-обработка-одинарных-кавычек)
* [12. Обработка двойных кавычек](#12-обработка-двойных-кавычек)
* [13. Создание Token](#13-создание-token)
* [14. Полная структура базового Lexer](#14-полная-структура-базового-lexer)
* [15. Тестирование](#15-тестирование)
* [16. Ошибки](#16-ошибки)
* [17. Что Lexer НЕ должен делать](#17-что-lexer-не-должен-делать)
* [18. План реализации](#18-план-реализации)
* [19. Финальный чеклист](#19-финальный-чеклист)

---

# 1. Что такое Lexer?

**Lexer (лексер)** — это часть Minishell, которая получает исходную строку, введённую пользователем, и разбивает её на отдельные смысловые элементы — **tokens**.

Например:

```bash
echo hello | wc -l
```

Lexer превращает это в:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
WORD("-l")
```

Lexer **не выполняет команду**.

Он отвечает только на вопрос:

> «Какие синтаксические элементы находятся в этой строке?»

---

# 2. Где Lexer находится в архитектуре Shell?

Общий pipeline Minishell:

```text
                    USER INPUT
                         |
                         v
                  +--------------+
                  |    LEXER     |
                  +------+-------+
                         |
                         v
                    TOKEN LIST
                         |
                         v
                  +--------------+
                  |    PARSER    |
                  +------+-------+
                         |
                         v
                 COMMAND STRUCTURE
                         |
                         v
                  +--------------+
                  |   EXPANSION  |
                  +------+-------+
                         |
                         v
                  +--------------+
                  |   EXECUTOR   |
                  +--------------+
```

Lexer — это первая основная стадия обработки команды после получения строки от пользователя.

---

# 3. Основная задача Lexer

Базовый Lexer должен:

* читать input посимвольно;
* пропускать пробелы вне кавычек;
* распознавать слова;
* распознавать операторы;
* обрабатывать одинарные кавычки;
* обрабатывать двойные кавычки;
* создавать tokens;
* добавлять tokens в linked list;
* обнаруживать незакрытые кавычки;
* сохранять информацию, необходимую следующим этапам.

Lexer **не должен выполнять команды**.

---

# 4. Вход и выход Lexer

## Вход

Например:

```bash
echo hello > output.txt
```

Lexer получает:

```c
char *input;
```

где:

```text
echo hello > output.txt
```

---

## Выход

Lexer создаёт:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

Внутренне это может выглядеть так:

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
| >           |
| REDIR_OUT   |
+------+------+
       |
       v
+-------------+
| output.txt  |
| WORD        |
+-------------+
```

То есть получается linked list:

```text
token1 -> token2 -> token3 -> token4 -> NULL
```

---

# 5. Какие символы должен распознавать Lexer

Для Minishell базовый Lexer должен распознавать несколько групп символов.

## Пробелы

Например:

```text
' '
'\t'
```

Пробелы обычно разделяют слова.

Например:

```bash
echo hello
```

становится:

```text
WORD("echo")
WORD("hello")
```

---

## Операторы

Основные операторы Minishell:

```text
|
<
>
<<
>>
```

Каждый оператор должен становиться отдельным Token.

---

## Кавычки

Lexer должен распознавать:

```text
'
"
```

Кавычки влияют на то, как Shell воспринимает содержимое строки.

---

# 6. Состояния Lexer

Удобно представлять Lexer как конечный автомат с несколькими состояниями.

Минимально нужны:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

---

## NORMAL

Обычное состояние.

Например:

```text
echo hello
```

В этом состоянии:

* пробел разделяет слова;
* `|` — оператор;
* `<` — оператор;
* `>` — оператор;
* кавычки изменяют состояние.

---

## SINGLE_QUOTE

Мы находимся внутри:

```bash
'...'
```

Например:

```bash
echo '$USER'
```

Всё между:

```text
'
```

и:

```text
'
```

считается литеральным текстом.

То есть:

```text
$USER
```

не должен интерпретироваться как переменная внутри одинарных кавычек.

---

## DOUBLE_QUOTE

Мы находимся внутри:

```bash
"..."
```

Например:

```bash
echo "hello world"
```

Пробел внутри кавычек **не разделяет слово**.

Поэтому результат:

```text
WORD("echo")
WORD("hello world")
```

а не:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

---

# 7. Базовый алгоритм Lexer

Основной цикл можно представить следующим образом:

```text
while input[i] != '\0'

    если текущий символ — пробел
        пропустить пробел

    иначе если текущий символ — '|'
        создать PIPE

    иначе если текущий символ — '<'
        проверить следующий символ
        если это '<'
            создать HEREDOC
        иначе
            создать REDIR_IN

    иначе если текущий символ — '>'
        проверить следующий символ
        если это '>'
            создать APPEND
        иначе
            создать REDIR_OUT

    иначе
        прочитать WORD

конец
```

Схематически:

```text
                  input[i]
                     |
          +----------+----------+
          |          |          |
        space     operator      word
          |          |          |
          v          v          v
        skip       token      read word
                                |
                                v
                              token
```

---

# 8. Обработка пробелов

Вне кавычек пробелы разделяют tokens.

Например:

```bash
echo     hello
```

должно дать:

```text
WORD("echo")
WORD("hello")
```

Несколько пробелов подряд не должны создавать пустые tokens.

Простейшая реализация:

```c
while (input[i] == ' ' || input[i] == '\t')
    i++;
```

Но это нужно делать только в обычном состоянии.

---

# 9. Чтение WORD

WORD — это не просто «всё до следующего пробела».

Например:

```bash
echo "hello world"
```

Второй token:

```text
WORD("hello world")
```

несмотря на пробел внутри.

---

## Другой пример

```bash
echo hello"world"
```

Shell воспринимает это как одно слово:

```text
helloworld
```

То есть Lexer должен продолжать собирать один WORD даже через quoted sections.

---

## Базовый алгоритм чтения WORD

Когда текущий символ не является:

* пробелом;
* оператором;
* концом строки;

начинаем читать слово.

```text
start WORD

while:
    не пробел
    И не оператор
    И не конец input

    если символ == '
        прочитать содержимое до следующей '

    иначе если символ == "
        прочитать содержимое до следующей "

    иначе
        добавить символ

конец WORD
```

---

# 10. Обработка операторов

Основные операторы:

```text
|
<
>
<<
>>
```

Важно понимать, что:

```text
<
<<
```

— разные операторы.

И:

```text
>
>>
```

— тоже разные операторы.

Поэтому при обнаружении `<` нужно посмотреть на следующий символ.

---

## `<`

Input:

```bash
cat < file
```

Tokens:

```text
WORD("cat")
REDIR_IN("<")
WORD("file")
```

---

## `<<`

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

## `>`

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

---

## `>>`

Input:

```bash
echo hello >> file
```

Tokens:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("file")
```

---

# 10.1 Распознавание операторов

Например:

```c
if (input[i] == '<')
{
    if (input[i + 1] == '<')
    {
        create_token("<<", TOKEN_HEREDOC);
        i += 2;
    }
    else
    {
        create_token("<", TOKEN_REDIR_IN);
        i++;
    }
}
```

Для `>`:

```c
if (input[i] == '>')
{
    if (input[i + 1] == '>')
    {
        create_token(">>", TOKEN_APPEND);
        i += 2;
    }
    else
    {
        create_token(">", TOKEN_REDIR_OUT);
        i++;
    }
}
```

Для `|`:

```c
if (input[i] == '|')
{
    create_token("|", TOKEN_PIPE);
    i++;
}
```

---

# 11. Обработка одинарных кавычек

Одинарные кавычки:

```bash
'
```

сохраняют содержимое буквально.

Например:

```bash
echo '$USER'
```

содержит:

```text
$USER
```

Но `$USER` внутри `'...'` не должен расширяться.

Для Lexer главное:

```text
'
|
+-- всё до следующей '
```

---

## Пример

```bash
echo 'hello world'
```

Результат:

```text
WORD("echo")
WORD("hello world")
```

Пробел между:

```text
hello world
```

не разделяет token.

---

# 11.1 Незакрытая одинарная кавычка

Input:

```bash
echo 'hello
```

Закрывающей:

```text
'
```

нет.

Lexer должен обнаружить ошибку.

Например:

```text
Unclosed single quote
```

Не следует просто продолжать обработку строки как будто всё нормально.

---

# 12. Обработка двойных кавычек

Двойные кавычки:

```bash
"
```

также объединяют содержимое в один WORD.

Например:

```bash
echo "hello world"
```

результат:

```text
WORD("echo")
WORD("hello world")
```

---

## Двойные кавычки и `$`

Внутри двойных кавычек parameter expansion всё ещё может работать.

Например:

```bash
echo "$USER"
```

Lexer не должен самостоятельно выполнять `$USER`.

Он должен правильно определить границы WORD и сохранить необходимую информацию для следующего этапа.

То есть:

```text
Lexer
  |
  v
определяет WORD "$USER"
  |
  v
Expansion
  |
  v
обрабатывает $USER
```

---

# 12.1 Незакрытая двойная кавычка

Input:

```bash
echo "hello
```

Закрывающей:

```text
"
```

нет.

Lexer должен вернуть ошибку:

```text
Unclosed double quote
```

---

# 13. Создание Token

Если структура Token уже реализована:

```c
t_token *token_new(char *value, t_token_type type);
```

то Lexer может создавать token:

```c
token = token_new(value, type);
```

и добавлять его:

```c
token_add_back(&tokens, token);
```

Например:

```c
token = token_new(ft_strdup("echo"), TOKEN_WORD);
token_add_back(&tokens, token);
```

---

# 13.1 Управление памятью

Очень важно заранее определить ownership.

Простой вариант:

```text
Token владеет value
```

Например:

```c
char *value;

value = ft_strdup("echo");
token = token_new(value, TOKEN_WORD);
```

После этого Token отвечает за освобождение:

```c
free(token->value);
free(token);
```

---

# 14. Полная структура базового Lexer

Хорошая архитектура:

```c
t_token *lexer(char *input)
{
    t_token *tokens;
    int     i;

    tokens = NULL;
    i = 0;

    while (input[i])
    {
        if (is_space(input[i]))
            i++;

        else if (input[i] == '|')
            handle_pipe(input, &i, &tokens);

        else if (input[i] == '<')
            handle_input_redirection(input, &i, &tokens);

        else if (input[i] == '>')
            handle_output_redirection(input, &i, &tokens);

        else
            handle_word(input, &i, &tokens);
    }

    return (tokens);
}
```

Такой подход лучше, чем одна огромная функция.

---

# 14.1 Вспомогательные функции

Например:

```c
int     is_space(char c);
int     is_operator(char c);

void    handle_pipe(
            char *input,
            int *i,
            t_token **tokens
        );

void    handle_input_redirection(
            char *input,
            int *i,
            t_token **tokens
        );

void    handle_output_redirection(
            char *input,
            int *i,
            t_token **tokens
        );

int     handle_word(
            char *input,
            int *i,
            t_token **tokens
        );
```

Названия можно адаптировать под структуру вашего проекта.

---

# 14.2 `is_operator()`

Простейший вариант:

```c
int is_operator(char c)
{
    return (c == '|' || c == '<' || c == '>');
}
```

Обрати внимание:

```text
<<
>>
```

не являются отдельными символами.

Они определяются комбинацией двух символов:

```text
<
<
```

или:

```text
>
>
```

---

# 14.3 `is_space()`

Базовый вариант:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

При необходимости можно расширить обработку whitespace согласно требованиям проекта.

---

# 14.4 Чтение WORD

Концептуально:

```c
char *read_word(char *input, int *i)
{
    int     start;
    char    *word;

    start = *i;

    while (input[*i]
        && !is_space(input[*i])
        && !is_operator(input[*i]))
    {
        if (input[*i] == '\'' || input[*i] == '"')
        {
            /*
             * Обработка quoted section.
             */
        }
        else
            (*i)++;
    }

    word = substring_from_input(input, start, *i);

    return (word);
}
```

Это только skeleton.

Обработку кавычек необходимо реализовать отдельно и аккуратно.

---

# 15. Тестирование

Тестирование Lexer — одна из самых важных частей работы.

Создай временную debug-функцию, которая выводит для каждого token:

```text
TYPE
VALUE
```

Например:

```text
TYPE=WORD VALUE=echo
TYPE=WORD VALUE=hello
TYPE=PIPE VALUE=|
TYPE=WORD VALUE=wc
```

---

# 15.1 Простая команда

Input:

```bash
echo hello
```

Expected:

```text
[WORD] echo
[WORD] hello
```

---

# 15.2 Несколько пробелов

Input:

```bash
echo     hello
```

Expected:

```text
[WORD] echo
[WORD] hello
```

---

# 15.3 Pipe

Input:

```bash
echo hello | wc
```

Expected:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
```

---

# 15.4 Input redirection

Input:

```bash
cat < input.txt
```

Expected:

```text
[WORD] cat
[REDIR_IN] <
[WORD] input.txt
```

---

# 15.5 Output redirection

Input:

```bash
echo hello > output.txt
```

Expected:

```text
[WORD] echo
[WORD] hello
[REDIR_OUT] >
[WORD] output.txt
```

---

# 15.6 Append

Input:

```bash
echo hello >> output.txt
```

Expected:

```text
[WORD] echo
[WORD] hello
[APPEND] >>
[WORD] output.txt
```

---

# 15.7 Heredoc

Input:

```bash
cat << EOF
```

Expected:

```text
[WORD] cat
[HEREDOC] <<
[WORD] EOF
```

---

# 15.8 Одинарные кавычки

Input:

```bash
echo 'hello world'
```

Expected:

```text
[WORD] echo
[WORD] hello world
```

---

# 15.9 Двойные кавычки

Input:

```bash
echo "hello world"
```

Expected:

```text
[WORD] echo
[WORD] hello world
```

---

# 15.10 Смешанные кавычки

Input:

```bash
echo 'hello'"world"
```

Это должно быть одним WORD:

```text
helloworld
```

То есть концептуально:

```text
'hello'
+
"world"
=
helloworld
```

Важно понимать, что Shell воспринимает эти части как один word.

---

# 15.11 Пустые кавычки

Проверь:

```bash
echo ""
```

и:

```bash
echo ''
```

Это должны быть **пустые WORD**, а не отсутствие token.

То есть:

```text
WORD("")
```

отличается от:

```text
NO TOKEN
```

Это важный edge case для дальнейшей реализации Parser и Expansion.

---

# 15.12 Операторы без пробелов

Lexer не должен требовать пробелы вокруг операторов.

Например:

```bash
echo hello>file
```

должно стать:

```text
[WORD] echo
[WORD] hello
[REDIR_OUT] >
[WORD] file
```

И:

```bash
echo hello|wc
```

должно стать:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
```

Это очень важно.

---

# 16. Ошибки

Lexer должен обнаруживать как минимум незакрытые кавычки.

## Незакрытая `'`

```bash
echo 'hello
```

Ошибка:

```text
unclosed single quote
```

---

## Незакрытая `"`

```bash
echo "hello
```

Ошибка:

```text
unclosed double quote
```

---

# 16.1 Lexer Error vs Parser Error

Важно не смешивать эти две категории.

Например:

```bash
echo hello |
```

Lexer вполне может успешно создать:

```text
WORD("echo")
WORD("hello")
PIPE("|")
```

С токенизацией всё нормально.

Проблема в синтаксисе.

Её должен обнаружить Parser:

```text
PIPE
  |
  v
nothing after PIPE
```

и вернуть:

```text
syntax error
```

---

# 16.2 Ещё один пример

Input:

```bash
echo >
```

Lexer:

```text
WORD("echo")
REDIR_OUT(">")
```

Lexer сделал свою работу правильно.

Но Parser увидит:

```text
REDIR_OUT
    |
    v
  EOF
```

После redirection должен находиться WORD.

Поэтому Parser должен сообщить:

```text
syntax error
```

---

# 17. Что Lexer НЕ должен делать

Очень важно правильно разделить обязанности между компонентами.

## Lexer не должен выполнять команды

Он не должен вызывать:

```c
execve()
```

---

## Lexer не должен создавать pipes

Он не должен заниматься:

```c
pipe()
```

---

## Lexer не должен создавать процессы

Он не должен вызывать:

```c
fork()
```

---

## Lexer не должен выполнять builtins

Например:

```text
cd
echo
pwd
export
unset
env
exit
```

не выполняются Lexer.

---

## Lexer не должен выполнять redirections

Lexer только распознаёт:

```text
<
>
<<
>>
```

Actual file descriptor operations выполняются позже.

---

## Lexer не должен строить финальную структуру команд

Lexer создаёт:

```text
TOKEN LIST
```

Parser превращает её в:

```text
COMMAND STRUCTURE
```

---

# 17.1 А что насчёт Expansion?

Не стоит смешивать Lexer и Expansion.

Например:

```bash
echo $USER
```

Lexer создаёт:

```text
WORD("echo")
WORD("$USER")
```

Затем Expansion обрабатывает:

```text
$USER
```

Например:

```text
$USER
   |
   v
alice
```

Архитектурно:

```text
Input
  |
  v
Lexer
  |
  v
WORD("$USER")
  |
  v
Expansion
  |
  v
WORD("alice")
```

---

# 18. План реализации

## Шаг 1 — Создать файлы Lexer

Например:

```text
src/
├── lexer/
│   ├── lexer.c
│   ├── lexer_utils.c
│   ├── lexer_word.c
│   └── lexer_quotes.c
│
├── token/
│   ├── token_new.c
│   ├── token_add_back.c
│   └── token_free.c
│
├── parser/
│
└── executor/
```

Структуру можно изменить в соответствии с архитектурой вашей команды.

---

# Шаг 2 — Реализовать helpers

Создать:

```c
int is_space(char c);
int is_operator(char c);
```

И протестировать их отдельно.

---

# Шаг 3 — Реализовать operators

Поддержать:

```text
|
<
>
<<
>>
```

Проверить отдельно:

```text
<
<<
>
>>
|
```

---

# Шаг 4 — Реализовать WORD

Начать с простых команд:

```bash
echo hello
```

Затем:

```bash
echo hello world
ls -la
pwd
```

---

# Шаг 5 — Добавить quotes

Реализовать:

```text
'
"
```

Проверить:

```bash
echo 'hello world'
echo "hello world"
```

После этого:

```bash
echo hello"world"
echo "hello"'world'
```

---

# Шаг 6 — Добавить ошибки

Проверить:

```bash
echo 'hello
```

и:

```bash
echo "hello
```

Lexer должен обнаружить незакрытые кавычки.

---

# Шаг 7 — Создать debug output

Например:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
```

Это сильно упростит debugging Lexer.

---

# 18.1 Рекомендуемая последовательность разработки

Не пытайся сразу реализовать весь Lexer.

Лучше двигаться постепенно:

```text
1. Простые WORD
       ↓
2. Spaces
       ↓
3. |
       ↓
4. <
       ↓
5. >
       ↓
6. <<
       ↓
7. >>
       ↓
8. Single quotes
       ↓
9. Double quotes
       ↓
10. Mixed quotes
       ↓
11. Empty quotes
       ↓
12. Error handling
```

После каждого шага запускай тесты.

---

# 19. Пример полного процесса

Input:

```bash
echo "hello world" > output.txt | cat
```

Lexer начинает читать:

```text
e
```

Распознаёт:

```text
WORD("echo")
```

Пропускает пробел.

Встречает:

```text
"
```

Переходит в состояние:

```text
DOUBLE_QUOTE
```

Читает:

```text
hello world
```

до следующей:

```text
"
```

Создаёт:

```text
WORD("hello world")
```

Затем встречает:

```text
>
```

Создаёт:

```text
REDIR_OUT(">")
```

Читает:

```text
output.txt
```

Создаёт:

```text
WORD("output.txt")
```

Затем встречает:

```text
|
```

Создаёт:

```text
PIPE("|")
```

И наконец:

```text
cat
```

становится:

```text
WORD("cat")
```

---

# Финальный Token List

```text
WORD("echo")
    |
    v
WORD("hello world")
    |
    v
REDIR_OUT(">")
    |
    v
WORD("output.txt")
    |
    v
PIPE("|")
    |
    v
WORD("cat")
    |
    v
NULL
```

---

# 19. Финальный чеклист

## Basic Lexer

* [ ] Lexer получает raw input.
* [ ] Lexer проходит строку посимвольно.
* [ ] Пробелы вне кавычек пропускаются.
* [ ] WORD корректно распознаются.
* [ ] `|` распознаётся.
* [ ] `<` распознаётся.
* [ ] `>` распознаётся.
* [ ] `<<` распознаётся.
* [ ] `>>` распознаётся.

## Quotes

* [ ] Single quotes распознаются.
* [ ] Double quotes распознаются.
* [ ] Пробелы внутри кавычек не разделяют WORD.
* [ ] Содержимое single quotes остаётся literal.
* [ ] Double-quoted section остаётся одним WORD.
* [ ] Незакрытая `'` обнаруживается.
* [ ] Незакрытая `"` обнаруживается.
* [ ] Пустые кавычки обрабатываются.

## Tokens

* [ ] `TOKEN_WORD`
* [ ] `TOKEN_PIPE`
* [ ] `TOKEN_REDIR_IN`
* [ ] `TOKEN_REDIR_OUT`
* [ ] `TOKEN_APPEND`
* [ ] `TOKEN_HEREDOC`

## Functions

* [ ] `token_new()`
* [ ] `token_add_back()`
* [ ] `free_tokens()`
* [ ] `print_tokens()`
* [ ] `is_space()`
* [ ] `is_operator()`
* [ ] `handle_word()`
* [ ] Обработчики operators

## Memory

* [ ] Каждый token освобождается.
* [ ] Каждый `value` освобождается.
* [ ] Нет memory leaks.
* [ ] Нет double free.
* [ ] Нет use-after-free.
* [ ] Ownership `value` определён.

## Tests

* [ ] `echo hello`
* [ ] `echo hello world`
* [ ] `echo hello | wc`
* [ ] `cat < file`
* [ ] `echo hello > file`
* [ ] `echo hello >> file`
* [ ] `cat << EOF`
* [ ] `echo "hello world"`
* [ ] `echo 'hello world'`
* [ ] `echo hello"world"`
* [ ] `echo ""`
* [ ] `echo ''`
* [ ] `echo hello>file`
* [ ] `echo hello|wc`
* [ ] `echo 'hello`
* [ ] `echo "hello`

---

# Главная идея

Lexer превращает:

```bash
echo "hello world" > file | cat
```

в:

```text
WORD("echo")
WORD("hello world")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("cat")
```

Он **не выполняет команду**.

Обязанности разделены:

```text
                RAW INPUT
                    |
                    v
                  LEXER
                    |
                    | "Какие tokens здесь есть?"
                    v
                TOKEN LIST
                    |
                    v
                  PARSER
                    |
                    | "Что означает эта структура?"
                    v
             COMMAND STRUCTURE
                    |
                    v
                EXPANSION
                    |
                    v
                EXECUTOR
                    |
                    | "Выполнить"
                    v
                PROCESSES
```

### Главная цель этой задачи

> **Создать базовый Lexer, который надёжно преобразует исходную строку Shell в упорядоченный список типизированных tokens, правильно обрабатывая слова, пробелы, операторы и кавычки.**

Lexer является фундаментом Parser.

Если Lexer неправильно разбивает input на tokens, все следующие этапы — Parser, Expansion и Executor — будут работать с неправильными данными.
