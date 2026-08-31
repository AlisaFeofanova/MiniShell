# Parameter Expansion в Minishell

## Содержание

* [1. Что такое Parameter Expansion](#1-что-такое-parameter-expansion)
* [2. Зачем нужен Parameter Expansion](#2-зачем-нужен-parameter-expansion)
* [3. Что такое Parameter](#3-что-такое-parameter)
* [4. Основной синтаксис `$VAR`](#4-основной-синтаксис-var)
* [5. Переменные окружения](#5-переменные-окружения)
* [6. Переменная не существует](#6-переменная-не-существует)
* [7. Пустая переменная](#7-пустая-переменная)
* [8. `$?` — Exit Status](#8---exit-status)
* [9. `$` перед обычным символом](#9--перед-обычным-символом)
* [10. Expansion внутри и вне Quotes](#10-expansion-внутри-и-вне-quotes)
* [11. Double Quotes](#11-double-quotes)
* [12. Single Quotes](#12-single-quotes)
* [13. Expansion и Spaces](#13-expansion-и-spaces)
* [14. Expansion и Word Splitting](#14-expansion-и-word-splitting)
* [15. Expansion и Empty Variables](#15-expansion-и-empty-variables)
* [16. Expansion и Command Arguments](#16-expansion-и-command-arguments)
* [17. Expansion после Redirection](#17-expansion-после-redirection)
* [18. Expansion в Heredoc](#18-expansion-в-heredoc)
* [19. Порядок Shell Processing](#19-порядок-shell-processing)
* [20. Алгоритм Parameter Expansion](#20-алгоритм-parameter-expansion)
* [21. Pseudocode](#21-pseudocode)
* [22. Структура данных](#22-структура-данных)
* [23. Типичные ошибки](#23-типичные-ошибки)
* [24. Тестирование](#24-тестирование)
* [25. Checklist](#25-checklist)
* [26. Вопросы для проверки знаний](#26-вопросы-для-проверки-знаний)
* [27. Главная модель](#27-главная-модель)

---

# 1. Что такое Parameter Expansion

**Parameter Expansion** — это процесс, во время которого Shell заменяет специальную запись, например:

```bash
$USER
```

значением соответствующего параметра.

Например:

```bash
echo $USER
```

Если:

```text
USER=alice
```

результат:

```text
alice
```

То есть:

```text
$USER
   ↓
значение переменной USER
```

---

# 2. Зачем нужен Parameter Expansion

Shell позволяет хранить информацию в переменных.

Например:

```bash
NAME="Alice"
```

После этого:

```bash
echo $NAME
```

становится:

```bash
echo Alice
```

Перед выполнением команды Shell должен заменить:

```text
$NAME
```

на:

```text
Alice
```

Общий процесс:

```text
INPUT
  |
  v
echo $NAME
  |
  v
Parameter Expansion
  |
  v
echo Alice
  |
  v
Execution
```

---

# 3. Что такое Parameter

В Shell parameter — это значение, к которому можно обратиться через специальный синтаксис.

В Minishell наиболее важны:

```bash
$NAME
```

и:

```bash
$?
```

Например:

```bash
USER=Alice
```

Здесь:

```text
NAME / USER
```

— имя параметра.

А:

```text
Alice
```

— его значение.

---

# 4. Основной синтаксис `$VAR`

Основная форма:

```bash
$VARIABLE
```

Например:

```bash
echo $HOME
```

Если:

```text
HOME=/home/alice
```

получаем:

```text
echo /home/alice
```

---

## 4.1 Как определить имя переменной

После `$` Shell ищет допустимые символы имени переменной.

Например:

```bash
$USER
```

имя:

```text
USER
```

```bash
$HOME
```

имя:

```text
HOME
```

```bash
$PATH
```

имя:

```text
PATH
```

---

## 4.2 Какие символы входят в имя

Обычно имя переменной состоит из:

```text
A-Z
a-z
0-9
_
```

Но имя переменной не должно начинаться с цифры при её создании.

Например:

```bash
USER
HOME
PATH
MY_VAR
VAR123
```

---

# 5. Переменные окружения

Minishell получает environment примерно в таком виде:

```c
char **envp
```

Например:

```text
USER=alice
HOME=/home/alice
PATH=/usr/bin:/bin
SHELL=/bin/bash
```

Внутри Minishell можно хранить environment в linked list:

```c
typedef struct s_env
{
    char            *key;
    char            *value;
    struct s_env     *next;
} t_env;
```

Например:

```text
key       value
-------------------------
USER      alice
HOME      /home/alice
PATH      /usr/bin:/bin
```

Когда встречается:

```bash
$USER
```

Minishell ищет:

```text
key = "USER"
```

и получает:

```text
value = "alice"
```

---

# 5.1 Поиск переменной

Упрощённая функция:

```c
char *get_env_value(t_env *env, char *key)
{
    while (env)
    {
        if (strcmp(env->key, key) == 0)
            return (env->value);
        env = env->next;
    }
    return (NULL);
}
```

Например:

```text
get_env_value(env, "HOME")
```

может вернуть:

```text
/home/alice
```

---

# 6. Переменная не существует

Что происходит:

```bash
echo $NOT_EXIST
```

Если:

```text
NOT_EXIST
```

не существует, результат обычно:

```text
""
```

То есть:

```bash
echo $NOT_EXIST
```

становится примерно:

```bash
echo
```

---

## Важно

Отсутствующая переменная не означает:

```text
"NOT_EXIST"
```

и не означает:

```text
"$NOT_EXIST"
```

Она расширяется в пустую строку.

```text
$NOT_EXIST
     ↓
""
```

---

# 7. Пустая переменная

Есть разница между:

```text
переменная отсутствует
```

и:

```text
переменная существует, но пустая
```

Например:

```bash
export NAME=
```

Теперь:

```text
NAME=""
```

И:

```bash
echo $NAME
```

даёт пустой результат.

Для базового Parameter Expansion результат в обоих случаях часто выглядит одинаково:

```text
""
```

Но внутри environment это разные состояния.

---

# 8. `$?` — Exit Status

Особый параметр:

```bash
$?
```

означает exit status предыдущей команды.

Например:

```bash
echo hello
```

успешно завершается:

```text
exit status = 0
```

После этого:

```bash
echo $?
```

получим:

```text
0
```

---

## 8.1 Пример с ошибкой

```bash
ls nonexistent_file
```

Команда завершается с ненулевым статусом.

После:

```bash
echo $?
```

получим ненулевое значение.

---

## 8.2 Почему `$?` особенный

Для:

```bash
$USER
```

нужно искать переменную:

```text
USER
```

Для:

```bash
$?
```

искать переменную `?` в environment нельзя.

Это специальный shell parameter.

Поэтому Expansion должен иметь отдельную обработку:

```c
if (input[i] == '$' && input[i + 1] == '?')
{
    expand_exit_status();
}
```

---

# 8.3 `$?` должен обновляться

Представим:

```bash
false
echo $?
```

После `false`:

```text
$? = 1
```

Затем выполняется:

```bash
echo 1
```

`echo` успешен:

```text
$? = 0
```

Если затем:

```bash
echo $?
```

получим:

```text
0
```

То есть `$?` всегда относится к **последнему выполненному pipeline/command status**, согласно семантике Shell.

---

# 9. `$` перед обычным символом

Не каждый `$` означает переменную.

Например:

```bash
echo $
```

`$` здесь не за чем расширять.

Также:

```bash
echo $!
```

```bash
echo $#
```

```bash
echo $-
```

В полном Bash существуют дополнительные special parameters, но базовый Minishell обычно должен реализовывать прежде всего:

```text
$VAR
$?
```

В рамках требований конкретного проекта нужно ориентироваться на subject.

---

# 9.1 `$` перед цифрой

Например:

```bash
echo $1
```

В полноценном Shell это positional parameter.

Но Minishell 42 обычно не реализует shell scripts/positional parameters как Bash.

Поэтому необходимо следовать именно требованиям вашего subject.

Не стоит автоматически реализовывать весь Bash.

---

# 10. Expansion внутри и вне Quotes

Это одна из самых важных частей.

Рассмотрим:

```bash
echo $USER
```

и:

```bash
echo "$USER"
```

и:

```bash
echo '$USER'
```

Они ведут себя по-разному.

---

# 10.1 Без кавычек

```bash
echo $USER
```

Parameter Expansion выполняется.

Если:

```text
USER=Alice
```

получаем:

```text
Alice
```

---

# 10.2 Double Quotes

```bash
echo "$USER"
```

Parameter Expansion также выполняется.

Результат:

```text
Alice
```

То есть:

```text
"$USER"
   ↓
"Alice"
```

---

# 10.3 Single Quotes

```bash
echo '$USER'
```

Parameter Expansion **не выполняется**.

Результат:

```text
$USER
```

То есть:

```text
'$USER'
    ↓
'$USER'
```

---

# 10.4 Главное правило

Запомните:

```text
Unquoted:
$VAR → expansion

Double quotes:
"$VAR" → expansion

Single quotes:
'$VAR' → NO expansion
```

---

# 11. Double Quotes

Double quotes:

```bash
"..."
```

не отключают Parameter Expansion.

Например:

```bash
NAME="Alice"

echo "Hello $NAME"
```

результат:

```text
Hello Alice
```

---

## Пример

```bash
echo "User: $USER"
```

может дать:

```text
User: alice
```

---

# 11.1 Несколько переменных

```bash
echo "$USER lives in $HOME"
```

может превратиться в:

```text
alice lives in /home/alice
```

---

# 12. Single Quotes

Single quotes:

```bash
'...'
```

защищают содержимое от Expansion.

Например:

```bash
echo '$HOME'
```

вывод:

```text
$HOME
```

---

## Сравнение

```bash
echo "$HOME"
```

→

```text
/home/alice
```

А:

```bash
echo '$HOME'
```

→

```text
$HOME
```

Это фундаментальное правило для Minishell.

---

# 13. Expansion и Spaces

Это важная часть реализации.

Предположим:

```bash
NAME="Alice Bob"
```

Что происходит:

```bash
echo $NAME
```

и:

```bash
echo "$NAME"
```

Они могут вести себя по-разному из-за word splitting.

---

## Unquoted

```bash
echo $NAME
```

значение:

```text
Alice Bob
```

может быть разделено на два слова:

```text
Alice
Bob
```

То есть аргументы:

```text
argv[0] = "echo"
argv[1] = "Alice"
argv[2] = "Bob"
```

---

## Double Quotes

```bash
echo "$NAME"
```

значение сохраняет пробел:

```text
Alice Bob
```

и становится одним аргументом:

```text
argv[0] = "echo"
argv[1] = "Alice Bob"
```

---

# 13.1 Почему это важно

Если сделать Expansion слишком просто:

```c
replace("$NAME", value);
```

можно неправильно обработать пробелы.

Нужно помнить о контексте:

```text
unquoted
double quoted
single quoted
```

---

# 14. Expansion и Word Splitting

Упрощённая модель:

```text
$VAR
  |
  v
value
  |
  v
если unquoted → возможно разделение на words
```

Но:

```text
"$VAR"
   |
   v
value
   |
   v
один word
```

---

## Пример

Environment:

```text
NAME="Alice Bob"
```

Команда:

```bash
printf '<%s>\n' $NAME
```

может работать как:

```text
printf '<%s>\n' Alice Bob
```

а:

```bash
printf '<%s>\n' "$NAME"
```

как:

```text
printf '<%s>\n' "Alice Bob"
```

---

# 15. Expansion и Empty Variables

Рассмотрим:

```bash
export NAME=
```

Затем:

```bash
echo $NAME
```

Получаем:

```text
empty
```

Но:

```bash
echo "$NAME"
```

также является одним пустым аргументом.

Это особенно важно для Parser/Executor.

---

## Разница

```bash
echo $NAME
```

может привести к отсутствию аргумента после удаления пустого unquoted expansion.

А:

```bash
echo "$NAME"
```

создаёт пустой аргумент.

Концептуально:

```text
$NAME
 ↓
empty
 ↓
может исчезнуть

"$NAME"
 ↓
empty string
 ↓
остаётся как argument
```

Это одна из причин, почему нельзя выполнять Expansion простым глобальным `replace()`.

---

# 16. Expansion и Command Arguments

Рассмотрим:

```bash
NAME=Alice
echo Hello $NAME
```

До Expansion:

```text
echo
Hello
$NAME
```

После:

```text
echo
Hello
Alice
```

Executor получает:

```c
argv[0] = "echo";
argv[1] = "Hello";
argv[2] = "Alice";
```

---

# 16.1 Несколько переменных

```bash
FIRST=Alice
LAST=Feofanova
```

Команда:

```bash
echo $FIRST $LAST
```

После Expansion:

```bash
echo Alice Feofanova
```

---

# 16.2 Переменная внутри слова

Например:

```bash
echo hello$USER
```

Если:

```text
USER=alice
```

результат:

```text
helloalice
```

Не:

```text
hello alice
```

То есть Expansion может происходить внутри одного WORD.

---

# 16.3 Prefix и suffix

```bash
echo /home/$USER/file
```

становится:

```text
/home/alice/file
```

И:

```bash
echo ${USER}123
```

в полноценном Shell позволяет явно отделить имя переменной от `123`.

Если ваш Minishell не реализует `${...}`, не нужно добавлять эту функциональность без необходимости.

---

# 17. Expansion после Redirection

Рассмотрим:

```bash
OUTPUT=result.txt
echo hello > $OUTPUT
```

Parameter Expansion:

```text
$OUTPUT
   ↓
result.txt
```

Получаем:

```bash
echo hello > result.txt
```

Затем redirection создаёт/открывает:

```text
result.txt
```

---

## Очень важно

Parser сначала должен определить:

```text
REDIR_OUT
WORD("$OUTPUT")
```

После этого Expansion может изменить:

```text
"$OUTPUT"
```

на:

```text
"result.txt"
```

То есть:

```text
Lexer
  ↓
Parser
  ↓
Redirection structure
  ↓
Expansion
  ↓
Open file
```

---

# 17.1 Если переменная пустая

Например:

```bash
OUTPUT=
echo hello > $OUTPUT
```

Это может привести к проблемному случаю: после expansion target может оказаться пустым.

Это нужно корректно обрабатывать согласно shell semantics и требованиям subject.

Не следует автоматически создавать файл с именем:

```text
""
```

---

# 18. Expansion в Heredoc

Heredoc:

```bash
cat << EOF
$USER
EOF
```

имеет отдельные правила.

В обычном heredoc delimiter:

```text
EOF
```

не quoted.

Тогда переменные внутри heredoc могут расширяться.

Например:

```bash
USER=Alice

cat << EOF
Hello $USER
EOF
```

может дать:

```text
Hello Alice
```

---

## Quoted delimiter

Например:

```bash
cat << 'EOF'
Hello $USER
EOF
```

Здесь delimiter quoted.

В этом случае expansion внутри heredoc отключается.

Результат:

```text
Hello $USER
```

---

# 18.1 Почему это важно для Minishell

Heredoc нельзя рассматривать просто как обычный WORD.

Нужно сохранить информацию:

```text
delimiter
quoted/unquoted
```

Например:

```c
typedef struct s_redir
{
    int     type;
    char    *file;
    int     heredoc_quoted;
} t_redir;
```

И затем:

```text
HEREDOC
   |
   +-- delimiter
   |
   +-- quoted?
         |
         +-- yes → no expansion
         |
         +-- no → expansion
```

---

# 19. Порядок Shell Processing

Parameter Expansion нельзя рассматривать изолированно.

Общий pipeline:

```text
Raw Input
    |
    v
Lexical Analysis
    |
    v
Tokens
    |
    v
Syntax Validation
    |
    v
Parsing
    |
    v
Parameter Expansion
    |
    v
Word Splitting
    |
    v
Pathname Expansion
    |
    v
Redirections
    |
    v
Execution
```

Точная внутренняя архитектура Minishell может отличаться, но концептуально важно понимать:

> `$VAR` не должен расширяться до того, как Shell понял структуру input и контекст quotes.

---

# 19.1 Почему нельзя делать Expansion в Lexer

Плохая идея:

```text
Input
 ↓
Lexer
 ↓
сразу заменить $VAR
 ↓
Tokens
```

Почему?

Потому что Lexer должен понимать quotes.

Например:

```bash
echo '$USER'
```

Если Lexer сразу заменит:

```text
$USER → Alice
```

получится:

```bash
echo 'Alice'
```

что неправильно.

Правильное поведение:

```text
Lexer:
'$USER'
   ↓
WORD with single quote context

Expansion:
single quote
   ↓
do NOT expand
```

---

# 20. Алгоритм Parameter Expansion

Упрощённый алгоритм:

```text
1. Получить WORD.

2. Пройти по символам.

3. Если встретили single quote:
       перейти в SINGLE_QUOTE context.

4. Если встретили double quote:
       перейти в DOUBLE_QUOTE context.

5. Если встретили '$':

       Если current context == SINGLE_QUOTE:
           оставить '$' как обычный символ.

       Иначе:

           Если следующий символ == '?':
               заменить на last_exit_status.

           Иначе если следующий символ
           является частью variable name:
               прочитать имя variable.
               найти variable в environment.
               заменить на её value.

           Иначе:
               '$' остаётся обычным символом.

6. После Expansion обработать word splitting,
   если это необходимо.

7. Передать результат дальше.
```

---

# 20.1 Context State

Можно использовать:

```c
typedef enum e_quote
{
    NO_QUOTE,
    SINGLE_QUOTE,
    DOUBLE_QUOTE
} t_quote;
```

И при обходе:

```text
NO_QUOTE
   |
   +-- ' → SINGLE_QUOTE
   |
   +-- " → DOUBLE_QUOTE
```

---

# 20.2 Пример прохода

Input:

```bash
echo "Hello $USER"
```

Lexer/Expansion видит:

```text
"
↓
DOUBLE_QUOTE

Hello
↓
обычный текст

$USER
↓
expansion разрешён

"
↓
NO_QUOTE
```

Если:

```text
USER=Alice
```

получаем:

```text
Hello Alice
```

---

# 21. Pseudocode

Упрощённый вариант:

```c
char *expand_word(char *word, t_shell *shell)
{
    char    *result;
    int     i;
    int     quote;

    result = init_string();
    i = 0;
    quote = NO_QUOTE;

    while (word[i])
    {
        update_quote_state(word[i], &quote);

        if (word[i] == '$' && quote != SINGLE_QUOTE)
        {
            if (word[i + 1] == '?')
            {
                append_exit_status(&result, shell->exit_status);
                i += 2;
                continue;
            }

            if (is_valid_var_start(word[i + 1]))
            {
                append_variable_value(
                    &result,
                    word,
                    &i,
                    shell->env
                );
                continue;
            }
        }

        append_char(&result, word[i]);
        i++;
    }

    return (result);
}
```

Это концептуальный пример.

Конкретная реализация зависит от вашей структуры Tokens и того, удаляете ли вы quotes во время Lexer или позже.

---

# 22. Структура данных

Хорошо иметь Shell context:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
} t_shell;
```

Тогда:

```text
shell->env
```

содержит environment.

А:

```text
shell->exit_status
```

содержит значение `$?`.

---

## Пример

```text
shell
 |
 +-- env
 |    |
 |    +-- USER=alice
 |    +-- HOME=/home/alice
 |    +-- PATH=/usr/bin:/bin
 |
 +-- exit_status = 0
```

Когда встречается:

```bash
$USER
```

ищем:

```text
env → USER
```

Когда:

```bash
$?
```

используем:

```text
shell->exit_status
```

---

# 23. Типичные ошибки

## Ошибка 1 — Расширять `$VAR` внутри single quotes

Неправильно:

```bash
echo '$USER'
```

→

```text
Alice
```

Правильно:

```text
$USER
```

---

# Ошибка 2 — Не расширять внутри double quotes

Неправильно:

```bash
echo "$USER"
```

→

```text
$USER
```

Правильно:

```text
Alice
```

---

# Ошибка 3 — Обрабатывать `$?` как обычную переменную

Неправильно:

```text
search_env("?")
```

Правильно:

```text
$?
 ↓
shell->exit_status
```

---

# Ошибка 4 — Использовать простой replace

Например:

```c
str_replace(input, "$USER", value);
```

Такой подход быстро ломается на:

```bash
echo '$USER'
```

```bash
echo "$USER"
```

```bash
echo hello$USER
```

```bash
echo $USER.txt
```

```bash
echo "$USER $HOME"
```

и heredoc.

Нужно учитывать context.

---

# Ошибка 5 — Игнорировать пустые значения

```bash
export NAME=
echo "$NAME"
```

должно создавать пустой аргумент.

---

# Ошибка 6 — Путать Parameter Expansion и Command Substitution

Это:

```bash
$USER
```

Parameter Expansion.

А:

```bash
$(whoami)
```

Command Substitution.

Это разные механизмы.

Если `$(...)` не поддерживается вашим Minishell subject, не нужно смешивать эти темы.

---

# Ошибка 7 — Пытаться реализовать весь Bash

Bash поддерживает множество возможностей:

```bash
${VAR}
${VAR:-default}
${VAR:=default}
${VAR:+value}
${VAR:?error}
$1
$2
$#
$@
$*
$!
$$
```

Но Minishell 42 имеет ограниченный scope.

Нужно реализовывать **то, что требует subject**, а не весь Bash.

---

# 24. Тестирование

Для Parameter Expansion необходимо создать отдельную группу тестов.

---

## Test 1 — Basic Variable

```bash
echo $USER
```

---

## Test 2 — HOME

```bash
echo $HOME
```

---

## Test 3 — PATH

```bash
echo $PATH
```

---

## Test 4 — Unknown Variable

```bash
echo $DOES_NOT_EXIST
```

Ожидается пустой результат.

---

## Test 5 — Empty Variable

```bash
export TEST=
echo $TEST
```

---

## Test 6 — Double Quotes

```bash
echo "$USER"
```

---

## Test 7 — Single Quotes

```bash
echo '$USER'
```

Ожидается:

```text
$USER
```

---

## Test 8 — Text + Variable

```bash
echo hello$USER
```

---

## Test 9 — Variable + Text

```bash
echo $USER-hello
```

---

## Test 10 — Multiple Variables

```bash
echo $USER $HOME
```

---

## Test 11 — `$?`

```bash
true
echo $?
```

Ожидается:

```text
0
```

---

## Test 12 — `$?` after error

```bash
false
echo $?
```

Ожидается ненулевой status.

---

## Test 13 — Variable inside quotes

```bash
echo "Hello $USER"
```

---

## Test 14 — Variable inside single quotes

```bash
echo 'Hello $USER'
```

---

## Test 15 — Spaces

Создайте:

```bash
export NAME="Alice Bob"
```

Проверьте:

```bash
echo $NAME
```

и:

```bash
echo "$NAME"
```

Они должны демонстрировать разницу между unquoted и quoted expansion.

---

## Test 16 — Redirection

```bash
export FILE=test.txt
echo hello > $FILE
```

---

## Test 17 — Heredoc

```bash
export NAME=Alice

cat << EOF
Hello $NAME
EOF
```

---

## Test 18 — Quoted Heredoc

```bash
export NAME=Alice

cat << 'EOF'
Hello $NAME
EOF
```

---

# 24.1 Таблица тестов

| Input                           | Expansion? | Expected                   |
| ------------------------------- | ---------- | -------------------------- |
| `$USER`                         | Yes        | value                      |
| `"$USER"`                       | Yes        | value                      |
| `'$USER'`                       | No         | `$USER`                    |
| `$HOME`                         | Yes        | home path                  |
| `$NOT_EXIST`                    | Yes        | empty                      |
| `$?`                            | Yes        | exit status                |
| `hello$USER`                    | Yes        | `hello` + value            |
| `$USER-hello`                   | Yes        | value + `-hello`           |
| `$NAME` where NAME has spaces   | Yes        | word splitting if unquoted |
| `"$NAME"` where NAME has spaces | Yes        | one word                   |

---

# 25. Checklist

Перед тем как считать Parameter Expansion изученным:

## Basic

* [ ] Понимаю, что такое Parameter Expansion.
* [ ] Понимаю `$VAR`.
* [ ] Понимаю, как искать переменную в environment.
* [ ] Понимаю, что происходит с неизвестной переменной.
* [ ] Понимаю разницу между unset и empty variable.

---

## `$?`

* [ ] Понимаю, что такое `$?`.
* [ ] Знаю, где хранить exit status.
* [ ] Понимаю, почему `$?` нельзя искать в env.
* [ ] Понимаю, когда обновляется `$?`.

---

## Quotes

* [ ] `$VAR` расширяется без quotes.
* [ ] `"$VAR"` расширяется.
* [ ] `'$VAR'` НЕ расширяется.
* [ ] Понимаю quote context.

---

## Word Splitting

* [ ] Понимаю разницу `$VAR` и `"$VAR"`.
* [ ] Понимаю влияние spaces.
* [ ] Понимаю empty expansion.

---

## Architecture

* [ ] Знаю, почему Expansion не должен ломать Lexer.
* [ ] Понимаю связь Lexer → Parser → Expansion → Execution.
* [ ] Знаю, где хранить environment.
* [ ] Знаю, где хранить `$?`.

---

## Redirection / Heredoc

* [ ] Понимаю expansion в redirection target.
* [ ] Понимаю expansion в heredoc.
* [ ] Понимаю влияние quoted heredoc delimiter.

---

# 26. Вопросы для проверки знаний

Попробуйте ответить без подсказки.

### Basic

1. Что такое Parameter Expansion?
2. Что означает `$USER`?
3. Где Minishell хранит `USER`?
4. Что произойдёт с `$ABC`, если `ABC` не существует?
5. Чем отличается unset variable от variable со значением `""`?

---

### Quotes

6. Что выведет:

```bash
echo "$USER"
```

7. Что выведет:

```bash
echo '$USER'
```

8. Почему результаты разные?

9. Расширяется ли `$USER` внутри double quotes?

10. Расширяется ли `$USER` внутри single quotes?

---

### Exit Status

11. Что означает:

```bash
$?
```

12. Где хранить его значение?

13. Почему нельзя искать `?` в environment?

14. Что произойдёт:

```bash
true
echo $?
```

15. Что произойдёт:

```bash
false
echo $?
```

---

### Spaces

16. Если:

```bash
NAME="Alice Bob"
```

чем отличаются:

```bash
echo $NAME
```

и:

```bash
echo "$NAME"
```

17. Почему quotes влияют на количество arguments?

---

### Architecture

18. Почему нельзя просто сделать:

```c
replace("$USER", value);
```

для всего input?

19. На каком этапе нужно учитывать quote context?

20. Почему:

```bash
echo '$USER'
```

является хорошим тестом для вашей реализации?

---

# 27. Главная модель

Запомните основной pipeline:

```text
                    USER INPUT
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
                 +--------------+
                 |    PARSER    |
                 +--------------+
                        |
                        v
                COMMAND STRUCTURE
                        |
                        v
              +-------------------+
              | PARAMETER EXPANSION|
              +-------------------+
                        |
             +----------+----------+
             |                     |
             v                     v
         $VARIABLE               $?
             |                     |
             v                     v
          ENV VALUE          EXIT STATUS
             |                     |
             +----------+----------+
                        |
                        v
                  WORD SPLITTING
                        |
                        v
                   REDIRECTIONS
                        |
                        v
                    EXECUTION
```

---

# Самые важные правила

## Rule 1

```bash
$VAR
```

означает:

```text
найти VAR
    ↓
получить value
    ↓
заменить $VAR на value
```

---

## Rule 2

```bash
"$VAR"
```

тоже выполняет Expansion.

---

## Rule 3

```bash
'$VAR'
```

НЕ выполняет Expansion.

---

## Rule 4

```bash
$?
```

— специальный параметр, содержащий exit status предыдущей команды.

---

## Rule 5

Неизвестная переменная:

```bash
echo $UNKNOWN
```

обычно расширяется в:

```text
empty string
```

---

## Rule 6

Context имеет значение:

```text
unquoted
    ↓
expansion + possible word splitting

double quoted
    ↓
expansion, но сохраняется как один word

single quoted
    ↓
no parameter expansion
```

---

# Итог

Parameter Expansion в Minishell — это не просто:

```text
найти "$"
↓
заменить текст
```

Это процесс, который должен учитывать:

```text
             $VAR
               |
               v
        Is it inside quotes?
          /           \
         /             \
    single quote    other context
         |               |
         v               v
      NO EXPANSION    EXPANSION
                         |
              +----------+----------+
              |                     |
             $VAR                  $?
              |                     |
              v                     v
           env value          exit status
              |
              v
       word splitting
        if applicable
```

Главная мысль:

> **Parameter Expansion заменяет `$VAR` значением параметра, но результат зависит от контекста quotes и от того, является ли параметр обычной переменной или специальным параметром `$?`.**

Для Minishell особенно важно уметь объяснить эту цепочку:

```text
$USER
  ↓
Lexer сохраняет контекст
  ↓
Parser создаёт WORD
  ↓
Expansion видит $USER
  ↓
ищет USER в env
  ↓
получает value
  ↓
учитывает quotes
  ↓
получает final argument
  ↓
Executor запускает command
```

Следующая логичная тема после этого:

```text
Parameter Expansion
        ↓
Word Splitting
        ↓
Pathname Expansion
        ↓
Redirections
        ↓
Execution
```

Именно эти этапы вместе формируют значительную часть **shell processing pipeline**.
