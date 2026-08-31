# Handle Spaces Correctly

## Содержание

* [1. Цель задачи](#1-цель-задачи)
* [2. Почему обработка пробелов важна](#2-почему-обработка-пробелов-важна)
* [3. Основное правило](#3-основное-правило)
* [4. Пробелы вне кавычек](#4-пробелы-вне-кавычек)
* [5. Несколько пробелов](#5-несколько-пробелов)
* [6. Табуляция](#6-табуляция)
* [7. Пробелы внутри кавычек](#7-пробелы-внутри-кавычек)
* [8. Смешанные quoted и unquoted части](#8-смешанные-quoted-и-unquoted-части)
* [9. Пробелы рядом с операторами](#9-пробелы-рядом-с-операторами)
* [10. Пустые quoted words](#10-пустые-quoted-words)
* [11. Алгоритм обработки spaces](#11-алгоритм-обработки-spaces)
* [12. Реализация](#12-реализация)
* [13. Типичная ошибка](#13-типичная-ошибка)
* [14. Тестирование](#14-тестирование)
* [15. Финальный чеклист](#15-финальный-чеклист)

---

# 1. Цель задачи

Задача:

> **Handle spaces correctly**

означает, что Lexer должен правильно определять, где пробел:

1. разделяет два token;
2. является частью WORD;
3. находится внутри кавычек и поэтому не разделяет WORD.

Например:

```bash
echo hello world
```

должно стать:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

Но:

```bash
echo "hello world"
```

должно стать:

```text
WORD("echo")
WORD("hello world")
```

---

# 2. Почему обработка пробелов важна

Пробелы — один из основных механизмов, с помощью которого Shell разделяет команды на отдельные слова.

Например:

```bash
ls -la file.txt
```

Lexer должен получить:

```text
WORD("ls")
WORD("-la")
WORD("file.txt")
```

Если обработать пробелы неправильно, Parser получит неправильный список tokens.

---

# 3. Основное правило

Главное правило:

> **Пробел вне кавычек разделяет tokens. Пробел внутри кавычек является частью текущего WORD.**

Сравним:

```bash
echo hello world
```

и:

```bash
echo "hello world"
```

Первый вариант:

```text
echo
hello
world
```

Второй:

```text
echo
hello world
```

---

# 4. Пробелы вне кавычек

В обычном состоянии:

```text
NORMAL
```

пробел является разделителем.

Например:

```bash
echo hello
```

Lexer читает:

```text
echo
```

Затем встречает:

```text
' '
```

и завершает текущий WORD.

После этого начинает искать следующий token:

```text
hello
```

Результат:

```text
WORD("echo")
WORD("hello")
```

---

# 4.1 Базовая функция `is_space()`

Можно создать helper:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

Тогда:

```c
if (is_space(input[i]))
    i++;
```

---

# 5. Несколько пробелов

Shell может содержать любое количество пробелов между словами.

Например:

```bash
echo     hello
```

Результат должен быть:

```text
WORD("echo")
WORD("hello")
```

Не должно появиться:

```text
WORD("")
WORD("")
WORD("")
```

---

## Правильный алгоритм

Когда Lexer встречает пробел:

```text
skip current space
     |
     v
skip next space
     |
     v
skip next space
     |
     v
continue
```

То есть лучше пропускать все последовательные whitespace:

```c
while (is_space(input[i]))
    i++;
```

---

# 5.1 Пробелы в начале строки

Input:

```bash
   echo hello
```

Результат:

```text
WORD("echo")
WORD("hello")
```

Начальные пробелы не создают token.

---

# 5.2 Пробелы в конце строки

Input:

```bash
echo hello   
```

Результат:

```text
WORD("echo")
WORD("hello")
```

Пробелы в конце также не создают token.

---

# 5.3 Только пробелы

Input:

```bash
     
```

Lexer должен вернуть:

```text
NULL
```

или пустой token list.

Не нужно создавать:

```text
WORD("")
```

---

# 6. Табуляция

Кроме обычного пробела:

```text
' '
```

Shell использует также tab:

```text
'\t'
```

Например:

```bash
echo	hello
```

должно работать как:

```bash
echo hello
```

Результат:

```text
WORD("echo")
WORD("hello")
```

Поэтому:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

---

# 6.1 Расширенная проверка whitespace

Если ваша реализация требует обработки всех стандартных whitespace, можно использовать:

```c
int is_space(char c)
{
    return (c == ' '
        || c == '\t'
        || c == '\n'
        || c == '\v'
        || c == '\f'
        || c == '\r');
}
```

Но для Minishell важно учитывать требования вашей конкретной реализации и то, какие символы могут попасть в строку от readline.

---

# 7. Пробелы внутри кавычек

Это самая важная часть.

Рассмотрим:

```bash
echo "hello world"
```

Здесь пробел между:

```text
hello world
```

не является разделителем.

Почему?

Потому что Lexer находится в:

```text
DOUBLE_QUOTE
```

состоянии.

Поэтому:

```text
"hello world"
```

должно рассматриваться как один WORD.

Результат:

```text
WORD("echo")
WORD("hello world")
```

---

# 7.1 Single quotes

То же самое происходит с:

```bash
echo 'hello world'
```

Результат:

```text
WORD("echo")
WORD("hello world")
```

Пробел внутри:

```text
'hello world'
```

не разделяет token.

---

# 7.2 Почему нельзя просто искать пробел?

Неправильно:

```c
while (input[i] != ' ')
    i++;
```

Потому что:

```bash
echo "hello world"
```

будет ошибочно разбито:

```text
WORD("echo")
WORD("hello)
WORD(world")
```

Правильно учитывать состояние кавычек.

---

# 8. Смешанные quoted и unquoted части

Очень важный случай:

```bash
echo hello"world"
```

Shell воспринимает это как **один WORD**:

```text
helloworld
```

Lexer должен продолжать читать WORD после окончания quoted section.

---

## Другой пример

```bash
echo "hello"world
```

Это также один WORD:

```text
helloworld
```

---

## Ещё пример

```bash
echo hello" "world
```

Получается:

```text
hello world
```

Но это всё ещё один WORD.

Концептуально:

```text
hello
+
" "
+
world
=
hello world
```

Поэтому пробел внутри quoted section не завершает WORD.

---

# 8.1 Несколько частей WORD

Можно представить WORD как последовательность частей:

```text
WORD
 |
 +-- unquoted: hello
 |
 +-- quoted: " "
 |
 +-- unquoted: world
```

Итог:

```text
hello world
```

---

# 9. Пробелы рядом с операторами

Пробелы могут находиться рядом с:

```text
|
<
>
<<
>>
```

Например:

```bash
echo hello | wc
```

Результат:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

# 9.1 Оператор без пробелов

Пробел вокруг оператора не обязателен.

Например:

```bash
echo hello|wc
```

должно стать:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

Поэтому Lexer должен рассматривать оператор как отдельный разделитель независимо от spaces.

---

# 9.2 Redirection

Input:

```bash
cat < input.txt
```

Результат:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

То же самое без пробелов:

```bash
cat<input.txt
```

Результат:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

# 9.3 Несколько пробелов вокруг оператора

Input:

```bash
echo hello     |     wc
```

Результат:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

Количество пробелов значения не имеет.

---

# 10. Пустые quoted words

Это важный edge case:

```bash
echo ""
```

Здесь после `echo` существует пустой WORD.

Результат концептуально:

```text
WORD("echo")
WORD("")
```

То есть:

```text
""
```

не равно отсутствию token.

---

## Single quotes

То же самое:

```bash
echo ''
```

должно дать:

```text
WORD("echo")
WORD("")
```

Это особенно важно для Expansion и дальнейшего Parser.

---

# 11. Алгоритм обработки spaces

Основная логика Lexer:

```text
while input[i] != '\0'

    if current character is whitespace
        skip all whitespace

    else if current character is operator
        create operator token

    else
        read word

end
```

---

# 11.1 Но внутри WORD

При чтении WORD логика должна быть:

```text
start WORD

while input[i] exists

    if current character is whitespace
        stop WORD

    if current character is operator
        stop WORD

    if current character is single quote
        read single quoted section
        continue WORD

    if current character is double quote
        read double quoted section
        continue WORD

    otherwise
        add character
        continue

end
```

Ключевой момент:

> **Whitespace останавливает WORD только тогда, когда мы находимся вне кавычек.**

---

# 12. Реализация

## `is_space()`

Минимальная реализация:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

---

# 12.1 Пропуск whitespace

В основном Lexer:

```c
while (input[i])
{
    if (is_space(input[i]))
    {
        i++;
        continue;
    }

    /*
     * Handle token.
     */
}
```

Лучше пропускать сразу все spaces:

```c
while (input[i] && is_space(input[i]))
    i++;
```

---

# 12.2 Пример основной функции

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
        {
            i++;
            continue;
        }

        if (is_operator(input[i]))
        {
            handle_operator(input, &i, &tokens);
            continue;
        }

        handle_word(input, &i, &tokens);
    }

    return (tokens);
}
```

---

# 12.3 Важный момент

Не нужно делать:

```c
if (input[i] == ' ')
```

внутри `handle_word()` без учёта кавычек.

Потому что:

```bash
echo "hello world"
```

должно сохранить:

```text
hello world
```

как один WORD.

---

# 12.4 Пример `handle_word()`

Концептуально:

```c
void handle_word(char *input, int *i, t_token **tokens)
{
    int     start;
    char    *value;

    start = *i;

    while (input[*i])
    {
        if (is_space(input[*i]))
            break;

        if (is_operator(input[*i]))
            break;

        if (input[*i] == '\'')
            skip_single_quote(input, i);

        else if (input[*i] == '"')
            skip_double_quote(input, i);

        else
            (*i)++;
    }

    value = extract_word(input, start, *i);

    add_token(tokens, value, TOKEN_WORD);
}
```

Это skeleton.

На следующем этапе нужно будет решить, сохраняете ли вы кавычки в `value` или удаляете их позже.

---

# 12.5 Важная архитектурная идея

Не нужно удалять quotes только ради того, чтобы правильно обработать spaces.

Лучше разделить этапы:

```text
INPUT
  |
  v
LEXER
  |
  | определяет границы WORD
  v
TOKENS
  |
  v
EXPANSION / QUOTE HANDLING
```

Так проще контролировать поведение:

```bash
echo "hello world"
```

Lexer понимает:

```text
один WORD
```

а последующие стадии решают:

```text
что делать с кавычками
что делать с $
что делать с wildcard
```

---

# 13. Типичная ошибка

## Ошибка №1

Разделять WORD только по пробелам:

```c
while (input[i] != ' ')
    i++;
```

Это сломает:

```bash
echo "hello world"
```

---

## Ошибка №2

Создавать token для каждого пробела:

```text
WORD("echo")
SPACE
SPACE
WORD("hello")
```

Для обычной Token list Minishell это обычно не нужно.

Spaces используются как разделители, а не как самостоятельные tokens.

---

## Ошибка №3

Создавать пустые WORD между пробелами

Input:

```bash
echo     hello
```

Неправильно:

```text
WORD("echo")
WORD("")
WORD("")
WORD("")
WORD("hello")
```

Правильно:

```text
WORD("echo")
WORD("hello")
```

---

## Ошибка №4

Не учитывать quotes

Input:

```bash
echo "hello world"
```

Неправильно:

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

---

# 14. Тестирование

Создай debug функцию:

```c
void print_tokens(t_token *tokens)
{
    while (tokens)
    {
        printf("TYPE: %d | VALUE: [%s]\n",
            tokens->type,
            tokens->value);
        tokens = tokens->next;
    }
}
```

---

## Test 1 — Один пробел

```bash
echo hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 2 — Несколько пробелов

```bash
echo     hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 3 — Начальные пробелы

```bash
     echo hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 4 — Пробелы в конце

```bash
echo hello     
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 5 — Только пробелы

```bash
       
```

Expected:

```text
NULL
```

---

## Test 6 — Tab

```bash
echo	hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 7 — Пробел внутри double quotes

```bash
echo "hello world"
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 8 — Пробел внутри single quotes

```bash
echo 'hello world'
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 9 — Mixed word

```bash
echo hello" world"
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 10 — Quote + word

```bash
echo "hello"world
```

Expected:

```text
WORD("echo")
WORD("helloworld")
```

---

## Test 11 — Operator с пробелами

```bash
echo hello | wc
```

Expected:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

## Test 12 — Operator без пробелов

```bash
echo hello|wc
```

Expected:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

## Test 13 — Redirection с пробелами

```bash
cat < input.txt
```

Expected:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

## Test 14 — Redirection без пробелов

```bash
cat<input.txt
```

Expected:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

## Test 15 — Пустая double quote

```bash
echo ""
```

Expected:

```text
WORD("echo")
WORD("")
```

---

## Test 16 — Пустая single quote

```bash
echo ''
```

Expected:

```text
WORD("echo")
WORD("")
```

---

# 14.1 Таблица тестов

| Input                | Expected tokens             |
| -------------------- | --------------------------- |
| `echo hello`         | `echo`, `hello`             |
| `echo     hello`     | `echo`, `hello`             |
| `   echo hello`      | `echo`, `hello`             |
| `echo hello   `      | `echo`, `hello`             |
| `echo "hello world"` | `echo`, `hello world`       |
| `echo 'hello world'` | `echo`, `hello world`       |
| `echo hello"world"`  | `echo`, `helloworld`        |
| `echo "hello"world`  | `echo`, `helloworld`        |
| `echo hello \| wc`   | `echo`, `hello`, `\|`, `wc` |
| `echo hello\|wc`     | `echo`, `hello`, `\|`, `wc` |
| `cat < file`         | `cat`, `<`, `file`          |
| `cat<file`           | `cat`, `<`, `file`          |
| `echo ""`            | `echo`, `""`                |
| `echo ''`            | `echo`, `''`                |

---

# 15. Финальный чеклист

## Spaces

* [ ] Пробел вне quotes разделяет WORD.
* [ ] Несколько пробелов пропускаются.
* [ ] Начальные пробелы пропускаются.
* [ ] Конечные пробелы пропускаются.
* [ ] Только пробелы дают пустой token list.
* [ ] Tab обрабатывается как whitespace.

## Quotes

* [ ] Пробел внутри `'...'` не разделяет WORD.
* [ ] Пробел внутри `"..."` не разделяет WORD.
* [ ] WORD может состоять из quoted и unquoted частей.
* [ ] `hello" world"` остаётся одним WORD.
* [ ] `"hello"world` остаётся одним WORD.
* [ ] Пустые quotes создают пустой WORD.

## Operators

* [ ] Spaces вокруг `|` работают.
* [ ] `|` без spaces работает.
* [ ] Spaces вокруг `<` работают.
* [ ] `<` без spaces работает.
* [ ] Spaces вокруг `>` работают.
* [ ] `>` без spaces работает.
* [ ] `<<` работает.
* [ ] `>>` работает.

## Memory

* [ ] Каждый созданный token освобождается.
* [ ] Каждый `value` освобождается.
* [ ] Нет memory leaks.
* [ ] Нет double free.
* [ ] Нет invalid memory access.

---

# Главная идея

Необходимо запомнить одно ключевое правило:

```text
                    SPACE
                      |
              +-------+-------+
              |               |
          OUTSIDE           INSIDE
          QUOTES            QUOTES
              |               |
              v               v
        разделяет WORD     часть WORD
```

Например:

```bash
echo hello world
```

получаем:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

Но:

```bash
echo "hello world"
```

получаем:

```text
WORD("echo")
WORD("hello world")
```

А:

```bash
echo hello" "world
```

получаем один WORD:

```text
WORD("hello world")
```

### Главная цель задачи

> **Lexer должен рассматривать whitespace как разделитель только вне кавычек и игнорировать его как разделитель внутри quoted sections.**

Если это правило реализовано правильно, обработка пробелов станет надёжной основой для дальнейшей работы с **quotes, operators, expansion и Parser**.
