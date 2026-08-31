# Preserve Quoted Strings

## Содержание

* [1. Цель задачи](#1-цель-задачи)
* [2. Что означает Preserve Quoted Strings](#2-что-означает-preserve-quoted-strings)
* [3. Зачем Shell нужны кавычки](#3-зачем-shell-нужны-кавычки)
* [4. Два типа кавычек](#4-два-типа-кавычек)
* [5. Одинарные кавычки `'...'`](#5-одинарные-кавычки-)
* [6. Двойные кавычки `"..."`](#6-двойные-кавычки-)
* [7. Кавычки должны сохранять содержимое](#7-кавычки-должны-сохранять-содержимое)
* [8. Кавычки и пробелы](#8-кавычки-и-пробелы)
* [9. Кавычки и операторы](#9-кавычки-и-операторы)
* [10. Кавычки внутри WORD](#10-кавычки-внутри-word)
* [11. Пустые кавычки](#11-пустые-кавычки)
* [12. Смешивание кавычек](#12-смешивание-кавычек)
* [13. Незакрытые кавычки](#13-незакрытые-кавычки)
* [14. Алгоритм Lexer](#14-алгоритм-lexer)
* [15. Структура кода](#15-структура-кода)
* [16. Типичные ошибки](#16-типичные-ошибки)
* [17. Тестирование](#17-тестирование)
* [18. Финальный чек-лист](#18-финальный-чек-лист)

---

# 1. Цель задачи

Задача:

> **Preserve quoted strings**

означает:

> Lexer должен правильно обрабатывать строки внутри `'...'` и `"..."`, сохраняя их как часть одного WORD и не позволяя пробелам или операторам внутри кавычек разделять токен.

Например:

```bash
echo "hello world"
```

не должно превращаться в:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

Правильный результат:

```text
WORD("echo")
WORD("hello world")
```

---

# 2. Что означает Preserve Quoted Strings

Рассмотрим:

```bash
echo "hello world"
```

Для обычного текста пробел является разделителем:

```bash
hello world
```

может стать:

```text
WORD("hello")
WORD("world")
```

Но внутри кавычек:

```bash
"hello world"
```

пробел является обычным символом содержимого.

Поэтому:

```text
"hello world"
```

должно сохраняться как единый элемент:

```text
WORD("hello world")
```

---

# 3. Зачем Shell нужны кавычки

Кавычки позволяют изменить стандартное поведение Shell.

Без кавычек:

```bash
echo hello world
```

получается два аргумента:

```text
hello
world
```

С кавычками:

```bash
echo "hello world"
```

получается один аргумент:

```text
hello world
```

Поэтому Lexer должен понимать:

```text
hello
```

и:

```text
"hello world"
```

как разные ситуации.

---

# 4. Два типа кавычек

Shell использует два основных типа:

### Одинарные кавычки

```text
'...'
```

### Двойные кавычки

```text
"..."
```

Они имеют разные правила обработки.

---

# 5. Одинарные кавычки `'...'`

Одинарные кавычки сохраняют содержимое практически буквально.

Например:

```bash
echo 'hello world'
```

Lexer должен сохранить:

```text
WORD("hello world")
```

---

# 5.1 Пробел внутри одинарных кавычек

```bash
echo 'hello     world'
```

Результат:

```text
WORD("echo")
WORD("hello     world")
```

Количество пробелов внутри кавычек сохраняется.

---

# 5.2 Операторы внутри одинарных кавычек

Например:

```bash
echo 'hello|world'
```

`|` здесь не является Pipe.

Результат:

```text
WORD("echo")
WORD("hello|world")
```

А не:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("world")
```

---

# 5.3 Перенаправление внутри одинарных кавычек

```bash
echo 'hello>world'
```

Результат:

```text
WORD("echo")
WORD("hello>world")
```

`>` не является оператором.

---

# 5.4 `<<` внутри одинарных кавычек

```bash
echo 'hello<<EOF'
```

Результат:

```text
WORD("echo")
WORD("hello<<EOF")
```

`<<` здесь обычный текст.

---

# 6. Двойные кавычки `"..."`

Двойные кавычки также позволяют сохранять пробелы и специальные символы как часть WORD.

Например:

```bash
echo "hello world"
```

Результат:

```text
WORD("echo")
WORD("hello world")
```

---

# 6.1 Операторы внутри двойных кавычек

```bash
echo "hello | world"
```

Результат:

```text
WORD("echo")
WORD("hello | world")
```

Pipe не распознаётся.

---

# 6.2 Перенаправление внутри двойных кавычек

```bash
echo "hello > world"
```

Результат:

```text
WORD("echo")
WORD("hello > world")
```

---

# 6.3 Несколько операторов

```bash
echo "a | b < c > d << e >> f"
```

Результат:

```text
WORD("echo")
WORD("a | b < c > d << e >> f")
```

Все операторы внутри кавычек являются частью WORD.

---

# 7. Кавычки должны сохранять содержимое

Очень важно различать:

### Исходный input

```bash
echo "hello world"
```

### Лексическая структура

```text
WORD
WORD
```

### Значение второго WORD

```text
hello world
```

Кавычки сами по себе обычно являются синтаксическими элементами Shell, а не частью итогового аргумента.

То есть:

```bash
echo "hello world"
```

в конечном результате должен передать `echo`:

```text
hello world
```

а не:

```text
"hello world"
```

---

# 7.1 Почему нельзя просто удалить кавычки сразу

На этапе Lexer не всегда стоит просто механически удалять все кавычки.

Например:

```bash
echo "hello"world
```

Это один WORD:

```text
helloworld
```

А:

```bash
echo "hello world"
```

это:

```text
hello world
```

Поэтому Lexer должен сначала правильно определить границы WORD и сохранить информацию, необходимую следующим этапам.

В зависимости от архитектуры Minishell можно:

1. сохранить исходное содержимое вместе с кавычками;
2. удалить кавычки во время отдельного quote-removal этапа;
3. хранить дополнительную информацию о частях токена.

Главное — **не потерять структуру строки слишком рано**.

---

# 8. Кавычки и пробелы

Это один из главных случаев.

Без кавычек:

```bash
echo hello world
```

получаем:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

С кавычками:

```bash
echo "hello world"
```

получаем:

```text
WORD("echo")
WORD("hello world")
```

---

# 8.1 Кавычки внутри WORD

Рассмотрим:

```bash
echo hello" world"
```

Это всё ещё один WORD.

Результат:

```text
WORD("echo")
WORD("hello world")
```

Lexer не должен создать:

```text
WORD("hello")
WORD(" world")
```

---

# 8.2 WORD может состоять из нескольких частей

Например:

```bash
echo abc"def"ghi
```

Все три части:

```text
abc
"def"
ghi
```

образуют один WORD:

```text
WORD("abcdefghi")
```

Это очень важная концепция.

> Кавычки не обязательно окружают весь WORD.

---

# 8.3 Ещё один пример

```bash
echo "hello"world
```

Результат:

```text
WORD("helloworld")
```

И:

```bash
echo hello"world"
```

результат:

```text
WORD("helloworld")
```

---

# 9. Кавычки и операторы

Операторы должны распознаваться только вне кавычек.

Например:

```bash
echo "hello|world"
```

Результат:

```text
WORD("echo")
WORD("hello|world")
```

Но:

```bash
echo hello|world
```

результат:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("world")
```

---

# 9.1 Сравнение

| Input                 | Результат                               |
| --------------------- | --------------------------------------- |
| `echo hello\|world`   | `WORD("hello") PIPE WORD("world")`      |
| `echo "hello\|world"` | `WORD("hello\|world")`                  |
| `echo 'hello\|world'` | `WORD("hello\|world")`                  |
| `echo hello\>world`   | `WORD("hello") REDIR_OUT WORD("world")` |
| `echo "hello>world"`  | `WORD("hello>world")`                   |

---

# 10. Кавычки внутри WORD

Кавычки могут находиться в середине WORD.

Например:

```bash
echo hello" world"
```

Lexer должен получить:

```text
WORD("hello world")
```

Необходимо продолжать текущий WORD после закрытия кавычек.

---

# 10.1 Правильный алгоритм

Если Lexer читает:

```text
hello" world"test
```

то он должен обработать:

```text
hello
```

затем:

```text
" world"
```

затем:

```text
test
```

Но итог:

```text
WORD("hello worldtest")
```

Это **один токен**.

---

# 10.2 Почему это важно

Нельзя считать закрытие кавычек концом WORD.

Закрытие кавычек означает только:

> "Мы снова находимся вне quote mode."

Но если следующий символ не является пробелом или оператором, WORD продолжается.

---

# 11. Пустые кавычки

Пустые кавычки:

```bash
echo ""
```

создают пустую строку.

Результат концептуально:

```text
WORD("echo")
WORD("")
```

А:

```bash
echo ''
```

тоже:

```text
WORD("echo")
WORD("")
```

---

# 11.1 Почему пустой WORD важен

Сравни:

```bash
echo ""
```

и:

```bash
echo
```

В первом случае `echo` получает пустой аргумент.

Во втором — аргумента нет.

Поэтому Lexer не должен просто игнорировать:

```text
""
```

или:

```text
''
```

---

# 11.2 Пустые кавычки внутри WORD

Например:

```bash
echo ab""cd
```

Результат:

```text
WORD("echo")
WORD("abcd")
```

Пустая строка всё равно является частью WORD.

---

# 12. Смешивание кавычек

Одинарные и двойные кавычки могут использоваться в одном WORD.

Например:

```bash
echo "hello"'world'
```

Это один WORD:

```text
WORD("helloworld")
```

---

# 12.1 Пример

```bash
echo 'hello'"world"
```

Результат:

```text
WORD("helloworld")
```

---

# 12.2 Кавычки внутри разных quote modes

Например:

```bash
echo "hello 'world'"
```

Внутри двойных кавычек одинарная кавычка не открывает новый quote mode.

Весь текст:

```text
hello 'world'
```

остаётся внутри двойных кавычек.

Результат:

```text
WORD("hello 'world'")
```

---

# 12.3 Обратный пример

```bash
echo 'hello "world"'
```

Двойные кавычки внутри одинарных также являются обычным текстом.

Результат:

```text
WORD("hello \"world\"")
```

Концептуально значение:

```text
hello "world"
```

---

# 13. Незакрытые кавычки

Очень важный случай:

```bash
echo "hello
```

или:

```bash
echo 'hello
```

В Bash интерактивный Shell обычно ожидает продолжение строки.

Для Minishell необходимо определить поведение в соответствии с требованиями проекта.

Lexer должен как минимум определить:

> Quote был открыт, но не закрыт.

---

# 13.1 Как определить незакрытую кавычку

При чтении:

```text
"hello
```

Lexer находит:

```text
"
```

и переходит в:

```text
DOUBLE_QUOTE_MODE
```

Затем ищет следующую:

```text
"
```

Если строка закончилась раньше:

```text
EOF
```

то quote не закрыта.

---

# 13.2 Возможная функция

```c
int find_closing_quote(char *input, int *i, char quote)
{
    (*i)++;

    while (input[*i])
    {
        if (input[*i] == quote)
        {
            (*i)++;
            return (1);
        }
        (*i)++;
    }

    return (0);
}
```

Если возвращается:

```text
0
```

кавычка не закрыта.

---

# 14. Алгоритм Lexer

Главная идея:

```text
                INPUT
                  |
                  v
              current char
                  |
          +-------+-------+
          |               |
        quote           normal
          |               |
          v               v
     enter quote       check:
        mode           space/operator
          |               |
          v               v
    read until        continue WORD
    closing quote
```

---

# 14.1 Основной цикл

Псевдокод:

```text
while input[i]:

    if space:
        skip spaces

    else if operator outside quotes:
        create operator token

    else:
        start WORD

        while current character belongs to WORD:

            if single quote:
                read until closing single quote

            else if double quote:
                read until closing double quote

            else if space:
                stop WORD

            else if operator:
                stop WORD

            else:
                add character

        create WORD token
```

---

# 14.2 Самое важное правило

При чтении WORD необходимо знать:

```text
outside quotes
```

или:

```text
inside quotes
```

Если мы внутри кавычек:

```text
space
|
<
>
```

не должны разделять WORD.

---

# 15. Структура кода

Один из возможных вариантов:

```c
void handle_word(char *input, int *i, t_token **tokens)
{
    int start;

    start = *i;

    while (input[*i])
    {
        if (input[*i] == '\'')
            handle_single_quote(input, i);
        else if (input[*i] == '"')
            handle_double_quote(input, i);
        else if (is_space(input[*i]) || is_operator(input[*i]))
            break;
        else
            (*i)++;
    }

    add_word_token(tokens, input, start, *i);
}
```

---

# 15.1 Обработка одинарных кавычек

Например:

```c
void handle_single_quote(char *input, int *i)
{
    (*i)++;

    while (input[*i] && input[*i] != '\'')
        (*i)++;

    if (input[*i] == '\'')
        (*i)++;
}
```

Здесь важно:

```text
```

Пока мы внутри `'...'`, операторные символы игнорируются.

---

# 15.2 Обработка двойных кавычек

```c
void handle_double_quote(char *input, int *i)
{
    (*i)++;

    while (input[*i] && input[*i] != '"')
        (*i)++;

    if (input[*i] == '"')
        (*i)++;
}
```

На более позднем этапе можно добавить обработку `$`, поскольку внутри двойных кавычек работает parameter expansion.

---

# 15.3 Более правильная концепция

Лучше рассматривать WORD как последовательность частей:

```text
WORD
 |
 +-- normal text
 |
 +-- single quoted text
 |
 +-- normal text
 |
 +-- double quoted text
 |
 +-- normal text
```

Например:

```bash
abc"hello"'world'xyz
```

структурно:

```text
WORD
 ├── abc
 ├── "hello"
 ├── 'world'
 └── xyz
```

Но итоговое значение:

```text
abchelloworldxyz
```

---

# 15.4 Почему это полезно для Minishell

Такой подход сильно упрощает следующие этапы:

```text
Lexer
   ↓
Parser
   ↓
Parameter Expansion
   ↓
Quote Removal
   ↓
Execution
```

Например:

```bash
echo "Hello $USER"
```

Lexer должен понимать границы:

```text
WORD("Hello $USER")
```

а затем Expansion сможет обработать:

```text
$USER
```

---

# 16. Типичные ошибки

## Ошибка 1 — Разделять WORD по пробелам внутри кавычек

Неправильно:

```bash
echo "hello world"
```

получается:

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

# 16.1 Ошибка 2 — Распознавать `|` внутри кавычек

Неправильно:

```bash
echo "hello|world"
```

как:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("world")
```

Правильно:

```text
WORD("echo")
WORD("hello|world")
```

---

# 16.2 Ошибка 3 — Считать закрытие кавычек концом WORD

Например:

```bash
echo "hello"world
```

Неправильно:

```text
WORD("hello")
WORD("world")
```

Правильно:

```text
WORD("helloworld")
```

---

# 16.3 Ошибка 4 — Игнорировать пустые кавычки

Неправильно:

```bash
echo ""
```

превращать в:

```text
WORD("echo")
```

Правильно:

```text
WORD("echo")
WORD("")
```

---

# 16.4 Ошибка 5 — Не проверять незакрытые кавычки

Например:

```bash
echo "hello
```

нельзя просто продолжить обработку как обычного текста.

Необходимо определить:

```text
UNCLOSED QUOTE
```

и обработать ситуацию согласно архитектуре Minishell.

---

# 16.5 Ошибка 6 — Считать кавычки отдельными WORD

Неправильно:

```bash
echo "hello world"
```

как:

```text
WORD("echo")
QUOTE
WORD("hello world")
QUOTE
```

Если ваша архитектура не предусматривает отдельные quote tokens, кавычки должны обрабатываться как часть построения WORD.

---

# 17. Тестирование

## Test 1 — Простые двойные кавычки

```bash
echo "hello world"
```

Ожидается:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 2 — Простые одинарные кавычки

```bash
echo 'hello world'
```

Ожидается:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 3 — Много пробелов

```bash
echo "hello     world"
```

Ожидается:

```text
WORD("echo")
WORD("hello     world")
```

---

## Test 4 — Pipe внутри кавычек

```bash
echo "hello|world"
```

Ожидается:

```text
WORD("echo")
WORD("hello|world")
```

---

## Test 5 — Redirection внутри кавычек

```bash
echo "hello>world"
```

Ожидается:

```text
WORD("echo")
WORD("hello>world")
```

---

## Test 6 — Here-document внутри кавычек

```bash
echo "hello<<EOF"
```

Ожидается:

```text
WORD("echo")
WORD("hello<<EOF")
```

---

## Test 7 — Кавычки в середине WORD

```bash
echo hello" world"
```

Ожидается:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 8 — Кавычки между обычными символами

```bash
echo abc"def"ghi
```

Ожидается:

```text
WORD("echo")
WORD("abcdefghi")
```

---

## Test 9 — Смешанные кавычки

```bash
echo "hello"'world'
```

Ожидается:

```text
WORD("echo")
WORD("helloworld")
```

---

## Test 10 — Пустые двойные кавычки

```bash
echo ""
```

Ожидается:

```text
WORD("echo")
WORD("")
```

---

## Test 11 — Пустые одинарные кавычки

```bash
echo ''
```

Ожидается:

```text
WORD("echo")
WORD("")
```

---

## Test 12 — Пустые кавычки внутри WORD

```bash
echo ab""cd
```

Ожидается:

```text
WORD("echo")
WORD("abcd")
```

---

## Test 13 — Одинарные внутри двойных

```bash
echo "hello 'world'"
```

Ожидается:

```text
WORD("echo")
WORD("hello 'world'")
```

---

## Test 14 — Двойные внутри одинарных

```bash
echo 'hello "world"'
```

Ожидается:

```text
WORD("echo")
WORD("hello \"world\"")
```

Значение:

```text
hello "world"
```

---

## Test 15 — Незакрытая двойная кавычка

```bash
echo "hello
```

Проверить:

```text
UNCLOSED_DOUBLE_QUOTE
```

---

## Test 16 — Незакрытая одинарная кавычка

```bash
echo 'hello
```

Проверить:

```text
UNCLOSED_SINGLE_QUOTE
```

---

## Test 17 — Кавычки + оператор

```bash
echo "hello" > file
```

Ожидается:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

## Test 18 — Оператор внутри кавычек + настоящий оператор

```bash
echo "hello > world" > file
```

Ожидается:

```text
WORD("echo")
WORD("hello > world")
REDIR_OUT(">")
WORD("file")
```

---

# 17.1 Таблица тестов

| Input                         | Ожидаемый результат             |
| ----------------------------- | ------------------------------- |
| `echo "hello world"`          | Один WORD для строки            |
| `echo 'hello world'`          | Один WORD для строки            |
| `echo "hello\|world"`         | `\|` не оператор                |
| `echo 'hello\|world'`         | `\|` не оператор                |
| `echo "hello>world"`          | `>` не оператор                 |
| `echo hello" world"`          | Один WORD                       |
| `echo abc"def"ghi`            | Один WORD                       |
| `echo "hello"'world'`         | Один WORD                       |
| `echo ""`                     | Пустой WORD                     |
| `echo ''`                     | Пустой WORD                     |
| `echo ab""cd`                 | `WORD("abcd")`                  |
| `echo "hello" > file`         | `WORD + REDIR + WORD`           |
| `echo "hello > world" > file` | Только последний `>` — оператор |

---

# 18. Финальный чек-лист

## Одинарные кавычки

* [ ] `'...'` распознаются.
* [ ] Пробелы внутри `'...'` сохраняются.
* [ ] `|` внутри `'...'` не является оператором.
* [ ] `<` внутри `'...'` не является оператором.
* [ ] `>` внутри `'...'` не является оператором.
* [ ] `<<` внутри `'...'` не является оператором.
* [ ] `>>` внутри `'...'` не является оператором.

## Двойные кавычки

* [ ] `"..."` распознаются.
* [ ] Пробелы внутри `"..."` сохраняются.
* [ ] `|` внутри `"..."` не является оператором.
* [ ] `<` внутри `"..."` не является оператором.
* [ ] `>` внутри `"..."` не является оператором.
* [ ] `<<` внутри `"..."` не является оператором.
* [ ] `>>` внутри `"..."` не является оператором.

## WORD

* [ ] Кавычки могут находиться внутри WORD.
* [ ] WORD продолжается после закрытия кавычек.
* [ ] `abc"def"ghi` является одним WORD.
* [ ] `"abc"'def'` является одним WORD.
* [ ] Пустые кавычки создают пустую часть WORD.
* [ ] `" "` создаёт WORD с пробелом.

## Ошибки

* [ ] Незакрытые `'` обнаруживаются.
* [ ] Незакрытые `"` обнаруживаются.
* [ ] Нет неправильного разделения WORD.
* [ ] Нет неправильного распознавания операторов внутри кавычек.

## Память

* [ ] Все строки токенов корректно выделяются.
* [ ] Все токены освобождаются.
* [ ] Нет memory leaks.
* [ ] Нет invalid reads.
* [ ] Нет buffer overflow.

---

# Главная концепция

Для Lexer очень важно понимать разницу между:

```text
OUTSIDE QUOTES
```

и:

```text
INSIDE QUOTES
```

Вне кавычек:

```text
space → разделитель
|     → оператор
<     → оператор
>     → оператор
```

Внутри кавычек:

```text
space → обычный символ
|     → обычный символ
<     → обычный символ
>     → обычный символ
```

Например:

```bash
echo hello|wc
```

становится:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

Но:

```bash
echo "hello|wc"
```

становится:

```text
WORD("echo")
WORD("hello|wc")
```

---

# Самая важная идея

Кавычки **не обязательно окружают весь WORD**.

Например:

```bash
echo abc"hello world"xyz
```

структурно:

```text
abc
"hello world"
xyz
```

но это:

```text
ONE WORD
```

с итоговым значением:

```text
abchello worldxyz
```

Поэтому Lexer должен рассматривать WORD как последовательность частей:

```text
WORD
 ├── normal text
 ├── quoted text
 ├── normal text
 └── quoted text
```

---

# Общая архитектура

После выполнения этого этапа у тебя должна быть примерно такая цепочка:

```text
                 RAW INPUT
                     |
                     v
                  LEXER
                     |
        +------------+------------+
        |            |            |
      spaces       quotes       operators
        |            |            |
        v            v            v
      skip       preserve       tokenize
                     |
                     v
                 WORD TOKENS
                     |
                     v
                  PARSER
                     |
                     v
              EXPANSION / QUOTE
                 PROCESSING
                     |
                     v
                 EXECUTION
```

Главное правило:

> **Lexer должен сохранять quoted strings как часть одного WORD и не позволять пробелам или операторам внутри кавычек разделять этот WORD.**

Это является основой корректной обработки Shell-команд в Minishell.
