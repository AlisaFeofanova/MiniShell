# Распознавание операторов Shell

## Содержание

* [1. Цель задачи](#1-цель-задачи)
* [2. Что такое оператор Shell](#2-что-такое-оператор-shell)
* [3. Операторы, необходимые для Minishell](#3-операторы-необходимые-для-minishell)
* [4. Основные правила распознавания](#4-основные-правила-распознавания)
* [5. Односимвольные операторы](#5-односимвольные-операторы)
* [6. Двухсимвольные операторы](#6-двухсимвольные-операторы)
* [7. Pipe `|`](#7-pipe-)
* [8. Перенаправление ввода `<`](#8-перенаправление-ввода-)
* [9. Перенаправление вывода `>`](#9-перенаправление-вывода-)
* [10. Here-document `<<`](#10-here-document-)
* [11. Append `>>`](#11-append-)
* [12. Операторы без пробелов](#12-операторы-без-пробелов)
* [13. Операторы внутри кавычек](#13-операторы-внутри-кавычек)
* [14. Приоритет при работе Lexer](#14-приоритет-при-работе-lexer)
* [15. Реализация](#15-реализация)
* [16. Типичные ошибки](#16-типичные-ошибки)
* [17. Тестирование](#17-тестирование)
* [18. Финальный чек-лист](#18-финальный-чек-лист)

---

# 1. Цель задачи

Задача:

> **Recognize shell operators**

означает, что Lexer должен находить специальные символы Shell и превращать их в соответствующие типы токенов.

Для стандартного проекта Minishell основными операторами являются:

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

должно превратиться в:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

# 2. Что такое оператор Shell

Оператор Shell — это специальный символ или последовательность символов, которые имеют особое значение для Shell.

Оператор — это не обычный текст.

Например:

```bash
echo hello > output.txt
```

содержит:

```text
echo
hello
>
output.txt
```

Символ:

```text
>
```

не является частью предыдущего слова.

Он говорит Shell:

> Перенаправить стандартный вывод в файл.

Поэтому Lexer должен создать для него специальный токен.

---

# 3. Операторы, необходимые для Minishell

Для стандартного 42 Minishell необходимо поддерживать:

| Оператор | Значение                 | Тип токена        |
| -------- | ------------------------ | ----------------- |
| `\|`     | Pipe                     | `TOKEN_PIPE`      |
| `<`      | Перенаправление ввода    | `TOKEN_REDIR_IN`  |
| `>`      | Перенаправление вывода   | `TOKEN_REDIR_OUT` |
| `<<`     | Here-document            | `TOKEN_HEREDOC`   |
| `>>`     | Добавление в конец файла | `TOKEN_APPEND`    |

Например, enum может выглядеть так:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_HEREDOC,
    TOKEN_APPEND
}   t_token_type;
```

---

# 4. Основные правила распознавания

Lexer должен определить:

> "Является ли текущий символ началом оператора?"

Например:

```bash
echo hello|wc
```

Когда Lexer доходит до:

```text
|
```

он должен закончить текущий `WORD`.

Результат:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

# 4.1 Оператор является разделителем

Оператор завершает текущий `WORD`.

Например:

```bash
hello>file
```

должно превратиться в:

```text
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

# 4.2 Пробелы вокруг оператора необязательны

Все эти варианты должны работать:

```bash
echo hello | wc
```

```bash
echo hello|wc
```

```bash
echo hello |wc
```

```bash
echo hello| wc
```

Все они должны дать:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

Следовательно:

> **Наличие пробелов не определяет, является ли символ оператором.**

---

# 5. Односимвольные операторы

Самые простые операторы:

```text
|
<
>
```

Каждый из них представляет отдельный токен.

Например:

```bash
cat < input.txt
```

получаем:

```text
WORD("cat")
TOKEN_REDIR_IN("<")
WORD("input.txt")
```

---

# 5.1 Распознавание `|`

Если:

```c
input[i] == '|'
```

создаём:

```text
TOKEN_PIPE
```

Затем перемещаемся на следующий символ:

```c
i++;
```

Например:

```bash
ls|wc
```

получаем:

```text
WORD("ls")
PIPE("|")
WORD("wc")
```

---

# 5.2 Распознавание `<`

Если:

```c
input[i] == '<'
```

нужно сначала проверить следующий символ.

Если следующий символ тоже `<`:

```text
<<
```

то это `HEREDOC`.

Если нет:

```text
<
```

то это обычное перенаправление ввода.

---

# 5.3 Распознавание `>`

Аналогично:

```c
input[i] == '>'
```

нужно сначала проверить следующий символ.

Если:

```text
>>
```

то это `APPEND`.

Если:

```text
>
```

то это `REDIR_OUT`.

---

# 6. Двухсимвольные операторы

В Minishell есть два оператора, состоящие из двух символов:

```text
<<
>>
```

Это очень важно, потому что Lexer должен воспринимать их как **один оператор**, а не как два отдельных.

Например:

```bash
cat << EOF
```

должно стать:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

# 6.1 Почему порядок проверки важен

Рассмотрим:

```text
<<
```

Если сначала проверить:

```c
if (input[i] == '<')
```

то Lexer может создать:

```text
TOKEN_REDIR_IN("<")
TOKEN_REDIR_IN("<")
```

Это неправильно.

Нужно сначала проверять:

```text
<<
```

и только если его нет — проверять:

```text
<
```

Правильный порядок:

```text
1. <<
2. <
```

А для `>`:

```text
1. >>
2. >
```

---

# 6.2 Принцип Longest Match

Общее правило Lexer:

> **Если несколько операторов начинаются с одного символа, сначала необходимо проверить самый длинный оператор.**

Для `<`:

```text
<<
<
```

Для `>`:

```text
>>
>
```

Это называется:

> **Longest Match — правило самого длинного совпадения.**

---

# 7. Pipe `|`

Pipe используется для передачи вывода одной команды на вход другой.

Например:

```bash
ls | wc
```

Lexer создаёт:

```text
WORD("ls")
PIPE("|")
WORD("wc")
```

---

# 7.1 Несколько Pipe

Команда:

```bash
ls | grep txt | wc
```

превращается в:

```text
WORD("ls")
PIPE("|")
WORD("grep")
WORD("txt")
PIPE("|")
WORD("wc")
```

Обрати внимание:

Lexer только распознаёт операторы.

Он ещё не решает, является ли последовательность синтаксически правильной.

Проверка:

```bash
ls || wc
```

или:

```bash
| ls
```

относится уже к Syntax Validation / Parser.

---

# 7.2 Pipe без пробелов

```bash
ls|grep|wc
```

Результат:

```text
WORD("ls")
PIPE("|")
WORD("grep")
PIPE("|")
WORD("wc")
```

---

# 8. Перенаправление ввода `<`

Пример:

```bash
cat < input.txt
```

Lexer:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

Lexer только определяет:

```text
<
```

как оператор.

Его реальное значение будет использовать Parser/Executor.

---

# 8.1 Без пробелов

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

# 8.2 С несколькими пробелами

```bash
cat     <     input.txt
```

Результат:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

# 9. Перенаправление вывода `>`

Пример:

```bash
echo hello > output.txt
```

Результат:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

# 9.1 Без пробелов

```bash
echo hello>output.txt
```

Результат:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

# 9.2 С несколькими пробелами

```bash
echo hello     >     output.txt
```

Результат:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

# 10. Here-document `<<`

Пример:

```bash
cat << EOF
hello
EOF
```

На этапе Lexer команда:

```bash
cat << EOF
```

превращается в:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

Важно:

Lexer **не должен реализовывать сам Here-document**.

Он только распознаёт:

```text
<<
```

Дальнейшая обработка выполняется на следующих этапах.

---

# 10.1 Алгоритм распознавания `<<`

```text
current == '<'
        |
        v
next == '<' ?
     /      \
   yes       no
    |         |
    v         v
 HEREDOC   REDIR_IN
```

Пример C:

```c
if (input[i] == '<' && input[i + 1] == '<')
{
    add_token(tokens, "<<", TOKEN_HEREDOC);
    i += 2;
}
else if (input[i] == '<')
{
    add_token(tokens, "<", TOKEN_REDIR_IN);
    i++;
}
```

---

# 11. Append `>>`

Append используется для добавления вывода в конец файла:

```bash
echo hello >> output.txt
```

Результат:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("output.txt")
```

---

# 11.1 Распознавание `>>`

Алгоритм:

```text
current == '>'
        |
        v
next == '>' ?
     /      \
   yes       no
    |         |
    v         v
 APPEND   REDIR_OUT
```

C:

```c
if (input[i] == '>' && input[i + 1] == '>')
{
    add_token(tokens, ">>", TOKEN_APPEND);
    i += 2;
}
else if (input[i] == '>')
{
    add_token(tokens, ">", TOKEN_REDIR_OUT);
    i++;
}
```

---

# 12. Операторы без пробелов

Это один из самых важных моментов.

Lexer должен распознавать операторы даже тогда, когда они непосредственно соединены с WORD.

Например:

```bash
echo hello|wc
```

```bash
cat<input.txt
```

```bash
echo hello>file
```

```bash
cat<<EOF
```

```bash
echo hello>>file
```

Результаты:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("file")
```

---

# 12.1 Оператор завершает WORD

Рассмотрим:

```bash
hello>world
```

Lexer начинает читать:

```text
hello
```

Затем встречает:

```text
>
```

Поэтому:

```text
WORD("hello")
```

завершается.

Затем создаётся:

```text
REDIR_OUT(">")
```

После этого Lexer продолжает:

```text
world
```

и создаёт:

```text
WORD("world")
```

Итог:

```text
WORD("hello")
REDIR_OUT(">")
WORD("world")
```

---

# 13. Операторы внутри кавычек

Это **критически важно**.

Оператор является оператором только тогда, когда он находится **вне кавычек**.

Например:

```bash
echo "|"
```

Символ:

```text
|
```

находится внутри двойных кавычек.

Поэтому это:

```text
WORD("echo")
WORD("|")
```

а НЕ:

```text
WORD("echo")
PIPE("|")
```

---

# 13.1 `<` внутри кавычек

```bash
echo "<"
```

Результат:

```text
WORD("echo")
WORD("<")
```

Не:

```text
REDIR_IN("<")
```

---

# 13.2 `>` внутри кавычек

```bash
echo ">"
```

Результат:

```text
WORD("echo")
WORD(">")
```

---

# 13.3 `<<` внутри кавычек

```bash
echo "<<"
```

Результат:

```text
WORD("echo")
WORD("<<")
```

---

# 13.4 `>>` внутри кавычек

```bash
echo ">>"
```

Результат:

```text
WORD("echo")
WORD(">>")
```

---

# 13.5 Операторы внутри одинарных кавычек

То же самое:

```bash
echo '|'
```

получаем:

```text
WORD("echo")
WORD("|")
```

И:

```bash
echo '<<'
```

получаем:

```text
WORD("echo")
WORD("<<")
```

---

# 14. Приоритет при работе Lexer

Хорошая структура основного цикла Lexer:

```text
1. Пропустить пробелы
        ↓
2. Проверить оператор
        ↓
3. Если это не оператор → прочитать WORD
```

Но во время чтения WORD необходимо снова проверять:

* пробел;
* оператор;
* кавычки.

---

# 14.1 Основное дерево решений

```text
                 input[i]
                    |
                    v
               пробел?
              /       \
            YES        NO
             |          |
          пропустить    v
                    оператор?
                    /       \
                  YES        NO
                   |          |
              прочитать     прочитать
              оператор        WORD
```

---

# 14.2 Проверка оператора

```text
                 input[i]
                    |
                    v
                '<' или '>'?
                 /       \
               YES        NO
                |          |
                v          v
          следующий      '|'
          такой же?        |
          /       \        |
        YES        NO      PIPE
         |          |
         v          v
      << / >>     < / >
```

---

# 15. Реализация

## 15.1 `is_operator()`

Создай функцию:

```c
int is_operator(char c)
{
    return (c == '|' || c == '<' || c == '>');
}
```

Она отвечает только на вопрос:

> Может ли этот символ начинать оператор?

---

# 15.2 Типы токенов

Например:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_HEREDOC,
    TOKEN_APPEND
}   t_token_type;
```

---

# 15.3 Обработчик операторов

Простой вариант:

```c
void handle_operator(char *input, int *i, t_token **tokens)
{
    if (input[*i] == '|')
    {
        add_token(tokens, "|", TOKEN_PIPE);
        (*i)++;
    }
    else if (input[*i] == '<' && input[*i + 1] == '<')
    {
        add_token(tokens, "<<", TOKEN_HEREDOC);
        (*i) += 2;
    }
    else if (input[*i] == '>' && input[*i + 1] == '>')
    {
        add_token(tokens, ">>", TOKEN_APPEND);
        (*i) += 2;
    }
    else if (input[*i] == '<')
    {
        add_token(tokens, "<", TOKEN_REDIR_IN);
        (*i)++;
    }
    else if (input[*i] == '>')
    {
        add_token(tokens, ">", TOKEN_REDIR_OUT);
        (*i)++;
    }
}
```

---

# 15.4 Почему `i += 2`

Для:

```text
<<
```

есть два символа.

Поэтому:

```c
i += 2;
```

Для:

```text
>>
```

тоже:

```c
i += 2;
```

Для:

```text
|
<
>
```

только один:

```c
i++;
```

---

# 15.5 Основной Lexer

Базовая структура:

```c
t_token *lexer(char *input)
{
    t_token *tokens;
    int     i;

    tokens = NULL;
    i = 0;

    while (input[i])
    {
        while (input[i] && is_space(input[i]))
            i++;

        if (!input[i])
            break;

        if (is_operator(input[i]))
            handle_operator(input, &i, &tokens);
        else
            handle_word(input, &i, &tokens);
    }

    return (tokens);
}
```

---

# 15.6 `handle_word()`

Оператор должен останавливать чтение WORD:

```c
void handle_word(char *input, int *i, t_token **tokens)
{
    int start;

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

    add_word_token(tokens, input, start, *i);
}
```

Но важно:

`skip_single_quote()` и `skip_double_quote()` должны пропускать операторные символы внутри кавычек.

---

# 16. Типичные ошибки

## Ошибка 1 — Проверять `<` раньше `<<`

Неправильно:

```c
if (input[i] == '<')
    TOKEN_REDIR_IN;
else if (input[i] == '<' && input[i + 1] == '<')
    TOKEN_HEREDOC;
```

Вторая проверка никогда не выполнится.

Правильно:

```c
if (input[i] == '<' && input[i + 1] == '<')
    TOKEN_HEREDOC;
else if (input[i] == '<')
    TOKEN_REDIR_IN;
```

---

# 16.1 Ошибка 2 — Такая же проблема с `>>`

Неправильно:

```c
if (input[i] == '>')
    TOKEN_REDIR_OUT;
else if (input[i] == '>' && input[i + 1] == '>')
    TOKEN_APPEND;
```

Правильно:

```c
if (input[i] == '>' && input[i + 1] == '>')
    TOKEN_APPEND;
else if (input[i] == '>')
    TOKEN_REDIR_OUT;
```

---

# 16.2 Ошибка 3 — Считать оператором символ внутри кавычек

Ввод:

```bash
echo "hello|world"
```

Неправильно:

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

# 16.3 Ошибка 4 — Требовать пробелы вокруг операторов

Неправильное предположение:

```bash
echo hello | wc
```

работает, а:

```bash
echo hello|wc
```

нет.

Это неправильно.

Операторы не требуют пробелов.

---

# 16.4 Ошибка 5 — Разделить `<<` на два токена

Ввод:

```bash
cat << EOF
```

Неправильно:

```text
WORD("cat")
REDIR_IN("<")
REDIR_IN("<")
WORD("EOF")
```

Правильно:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

# 16.5 Ошибка 6 — Включить оператор в WORD

Ввод:

```bash
hello>world
```

Неправильно:

```text
WORD("hello>world")
```

Правильно:

```text
WORD("hello")
REDIR_OUT(">")
WORD("world")
```

---

# 17. Тестирование

Необходимо тестировать операторы как с пробелами, так и без них.

---

## Test 1 — Pipe

```bash
echo hello | wc
```

Ожидаемый результат:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

## Test 2 — Pipe без пробелов

```bash
echo hello|wc
```

Ожидаемый результат:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

## Test 3 — Input Redirection

```bash
cat < input.txt
```

Результат:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

## Test 4 — Input Redirection без пробелов

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

## Test 5 — Output Redirection

```bash
echo hello > file
```

Результат:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

## Test 6 — Output Redirection без пробелов

```bash
echo hello>file
```

Результат:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

## Test 7 — Here-document

```bash
cat << EOF
```

Результат:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

## Test 8 — Here-document без пробелов

```bash
cat<<EOF
```

Результат:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

## Test 9 — Append

```bash
echo hello >> file
```

Результат:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("file")
```

---

## Test 10 — Append без пробелов

```bash
echo hello>>file
```

Результат:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("file")
```

---

## Test 11 — Операторы внутри двойных кавычек

```bash
echo "| < > << >>"
```

Результат:

```text
WORD("echo")
WORD("| < > << >>")
```

Никаких операторных токенов создавать нельзя.

---

## Test 12 — Операторы внутри одинарных кавычек

```bash
echo '| < > << >>'
```

Результат:

```text
WORD("echo")
WORD("| < > << >>")
```

---

## Test 13 — Несколько операторов

```bash
cat < input | grep hello > output
```

Результат:

```text
WORD("cat")
REDIR_IN("<")
WORD("input")
PIPE("|")
WORD("grep")
WORD("hello")
REDIR_OUT(">")
WORD("output")
```

---

## Test 14 — Сложная команда

```bash
cat<<EOF|grep hello>>output
```

Результат:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
PIPE("|")
WORD("grep")
WORD("hello")
APPEND(">>")
WORD("output")
```

---

# 17.1 Таблица тестов

| Ввод                | Ожидаемый оператор |
| ------------------- | ------------------ |
| `echo hello \| wc`  | `PIPE`             |
| `echo hello\|wc`    | `PIPE`             |
| `cat < file`        | `REDIR_IN`         |
| `cat<file`          | `REDIR_IN`         |
| `echo hello > file` | `REDIR_OUT`        |
| `echo hello>file`   | `REDIR_OUT`        |
| `cat << EOF`        | `HEREDOC`          |
| `cat<<EOF`          | `HEREDOC`          |
| `echo hi >> file`   | `APPEND`           |
| `echo hi>>file`     | `APPEND`           |
| `echo "a\|b"`       | Нет оператора      |
| `echo "< >"`        | Нет оператора      |
| `echo '<< >>'`      | Нет оператора      |

---

# 18. Финальный чек-лист

## Основные операторы

* [ ] `|` распознаётся.
* [ ] `<` распознаётся.
* [ ] `>` распознаётся.
* [ ] `<<` распознаётся.
* [ ] `>>` распознаётся.

## Longest Match

* [ ] `<<` проверяется раньше `<`.
* [ ] `>>` проверяется раньше `>`.
* [ ] `<<` создаёт один токен.
* [ ] `>>` создаёт один токен.

## Пробелы

* [ ] Операторы работают с пробелами.
* [ ] Операторы работают без пробелов.
* [ ] Несколько пробелов вокруг оператора обрабатываются правильно.

## Границы WORD

* [ ] Оператор завершает текущий WORD.
* [ ] Оператор не становится частью WORD.
* [ ] WORD после оператора корректно распознаётся.

## Кавычки

* [ ] `|` внутри кавычек не является оператором.
* [ ] `<` внутри кавычек не является оператором.
* [ ] `>` внутри кавычек не является оператором.
* [ ] `<<` внутри кавычек не является оператором.
* [ ] `>>` внутри кавычек не является оператором.
* [ ] Одинарные кавычки учитываются.
* [ ] Двойные кавычки учитываются.

## Память

* [ ] Значения операторных токенов корректно выделяются.
* [ ] Все токены можно освободить.
* [ ] Нет memory leaks.
* [ ] Нет invalid memory access.
* [ ] Нет double free.

---

# Главное, что нужно запомнить

Lexer должен различать:

```text
ОБЫЧНЫЙ ТЕКСТ
     |
     +---- WORD
     |
     +---- OPERATOR
```

Но при этом учитывать кавычки:

```text
                    INPUT
                      |
                      v
                Внутри кавычек?
                  /       \
                YES        NO
                 |          |
                 v          v
       | < > << >>       оператор?
       обычный текст      /       \
                         YES       NO
                          |         |
                          v         v
                      OPERATOR    WORD
```

Главное правило:

> **Оператор является специальным только тогда, когда он находится вне кавычек.**

А если несколько операторов начинаются с одного символа:

```text
<<
<
```

или:

```text
>>
>
```

всегда сначала проверяй **самый длинный вариант**.

---

# Общая схема

Например:

```bash
cat < input | grep hello >> output
```

Lexer должен получить:

```text
WORD("cat")
REDIR_IN("<")
WORD("input")
PIPE("|")
WORD("grep")
WORD("hello")
APPEND(">>")
WORD("output")
```

На этом этапе Lexer **не должен выполнять операторы** и не должен решать их полную семантику.

Его задача:

```text
RAW INPUT
    ↓
LEXER
    ↓
распознать WORD
распознать OPERATOR
учесть QUOTES
    ↓
TOKEN LIST
    ↓
PARSER
    ↓
EXPANSION
    ↓
EXECUTOR
```

То есть задача `Recognize shell operators` — научить Lexer правильно отличать:

```text
WORD
```

от:

```text
|
<
>
<<
>>
```

и при этом **не распознавать эти символы как операторы, если они находятся внутри кавычек**.
