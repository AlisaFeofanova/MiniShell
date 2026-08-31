# Понимание `$?` в Minishell

## Содержание

* [1. Что такое `$?`](#1-что-такое-)
* [2. Что такое Exit Status](#2-что-такое-exit-status)
* [3. Простые примеры](#3-простые-примеры)
* [4. `$?` — специальный параметр Shell](#4---специальный-параметр-shell)
* [5. Как работает `$?`](#5-как-работает-)
* [6. Переменная `exit_status`](#6-переменная-exit_status)
* [7. Когда нужно обновлять `$?`](#7-когда-нужно-обновлять-)
* [8. Builtins и `$?`](#8-builtins-и-)
* [9. Внешние команды и `$?`](#9-внешние-команды-и-)
* [10. Pipeline и `$?`](#10-pipeline-и-)
* [11. `$?` внутри кавычек](#11--внутри-кавычек)
* [12. `$?` внутри одинарных кавычек](#12--внутри-одинарных-кавычек)
* [13. `$?` внутри двойных кавычек](#13--внутри-двойных-кавычек)
* [14. `$?` вместе с другим текстом](#14--вместе-с-другим-текстом)
* [15. `$?` и выполнение команд](#15--и-выполнение-команд)
* [16. `$?` после `echo`](#16--после-echo)
* [17. `$?` после `cd`](#17--после-cd)
* [18. `$?` после `export`](#18--после-export)
* [19. `$?` после `unset`](#19--после-unset)
* [20. `$?` после `exit`](#20--после-exit)
* [21. `$?` и ошибки](#21--и-ошибки)
* [22. `$?` и сигналы](#22--и-сигналы)
* [23. `$?` в архитектуре Minishell](#23--в-архитектуре-minishell)
* [24. Стратегия реализации](#24-стратегия-реализации)
* [25. Псевдокод](#25-псевдокод)
* [26. Частые ошибки](#26-частые-ошибки)
* [27. Тестирование](#27-тестирование)
* [28. Checklist](#28-checklist)
* [29. Проверка знаний](#29-проверка-знаний)
* [30. Главная модель понимания](#30-главная-модель-понимания)

---

# 1. Что такое `$?`

`$?` — это **специальный параметр Shell**.

Он содержит **exit status последней выполненной команды или pipeline**.

Например:

```bash
true
echo $?
```

Результат:

```text
0
```

Почему?

Команда:

```bash
true
```

завершилась успешно.

Её exit status:

```text
0
```

Поэтому:

```text
$?
 ↓
0
```

---

# 2. Что такое Exit Status

Каждая команда после выполнения возвращает числовой статус.

Он сообщает Shell, была ли команда выполнена успешно.

Главное правило:

```text
0       → успех
non-zero → ошибка / неудача
```

Например:

```bash
true
```

возвращает:

```text
0
```

А:

```bash
false
```

возвращает:

```text
1
```

Поэтому:

```bash
false
echo $?
```

даст:

```text
1
```

---

## 2.1 Exit Status — это число

Внутри Minishell удобно хранить его как:

```c
int exit_status;
```

Например:

```c
exit_status = 0;
```

или:

```c
exit_status = 1;
```

или:

```c
exit_status = 127;
```

Shell хранит это значение и предоставляет его пользователю через:

```bash
$?
```

---

# 3. Простые примеры

## Успешная команда

```bash
echo hello
echo $?
```

`echo` обычно завершается успешно:

```text
0
```

---

## Неуспешная команда

```bash
ls /does/not/exist
echo $?
```

`ls` не смог найти указанный путь.

Поэтому exit status будет ненулевым.

---

## `true`

```bash
true
echo $?
```

Результат:

```text
0
```

---

## `false`

```bash
false
echo $?
```

Результат:

```text
1
```

---

# 4. `$?` — специальный параметр Shell

Очень важно понимать:

```bash
$?
```

**не является обычной environment variable**.

Например:

```bash
$USER
```

означает:

```text
найти переменную USER
        ↓
получить её значение
```

А:

```bash
$?
```

означает:

```text
получить текущий exit status Shell
```

Поэтому в Minishell нельзя просто сделать:

```c
get_env_value(env, "?");
```

Вместо этого нужно получить значение из состояния Shell:

```c
shell->exit_status;
```

---

# 4.1 Обычная переменная vs `$?`

### Environment variable

```bash
echo $USER
```

Логика:

```text
$USER
  ↓
environment
  ↓
найти USER
  ↓
получить value
```

### `$?`

```bash
echo $?
```

Логика:

```text
$?
 ↓
Shell state
 ↓
exit_status
```

Это два разных механизма.

---

# 5. Как работает `$?`

Представим структуру:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
} t_shell;
```

В начале:

```text
exit_status = 0
```

Пользователь запускает:

```bash
false
```

`false` возвращает:

```text
1
```

Minishell сохраняет:

```text
shell->exit_status = 1;
```

Теперь пользователь пишет:

```bash
echo $?
```

Во время Parameter Expansion:

```text
$?
 ↓
shell->exit_status
 ↓
1
```

Поэтому команда становится концептуально:

```bash
echo 1
```

`echo` выполняется успешно.

После этого:

```text
shell->exit_status = 0
```

---

# 6. Переменная `exit_status`

Один из самых удобных вариантов архитектуры:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
    int     last_pid;
} t_shell;
```

Главное поле:

```c
int exit_status;
```

Оно хранит значение, которое должно быть доступно через:

```bash
$?
```

---

## Пример

После:

```bash
false
```

состояние:

```text
shell
 |
 +-- exit_status = 1
```

После:

```bash
echo $?
```

происходит:

```text
$?
 ↓
1
 ↓
echo 1
```

После успешного `echo`:

```text
shell
 |
 +-- exit_status = 0
```

---

# 7. Когда нужно обновлять `$?`

Exit status должен обновляться **после выполнения команды**.

Например:

```bash
true
```

→

```text
exit_status = 0
```

Затем:

```bash
false
```

→

```text
exit_status = 1
```

Затем:

```bash
echo hello
```

→

```text
exit_status = 0
```

То есть значение постоянно меняется.

---

## Очень важное правило

Следующая команда может изменить `$?`.

Например:

```bash
false
echo hello
echo $?
```

Результат:

```text
0
```

а не:

```text
1
```

Потому что:

```text
false
 ↓
exit_status = 1

echo hello
 ↓
exit_status = 0

echo $?
 ↓
0
```

---

# 8. Builtins и `$?`

Builtins также имеют exit status.

В Minishell основные builtins:

```text
echo
cd
pwd
export
unset
env
exit
```

Каждый builtin должен вернуть соответствующий статус.

---

## `echo`

```bash
echo hello
echo $?
```

Обычно:

```text
0
```

---

## `cd`

Успешный:

```bash
cd /tmp
echo $?
```

Обычно:

```text
0
```

Неуспешный:

```bash
cd /does/not/exist
echo $?
```

Должен быть ненулевой статус.

---

## `export`

Успешный:

```bash
export NAME=Alice
echo $?
```

Обычно:

```text
0
```

Некорректный:

```bash
export 123=hello
echo $?
```

Должен дать ненулевой status.

---

# 9. Внешние команды и `$?`

Внешние программы также возвращают exit status.

Например:

```bash
/bin/true
echo $?
```

Результат:

```text
0
```

И:

```bash
/bin/false
echo $?
```

Результат:

```text
1
```

Minishell получает информацию о завершении дочернего процесса через:

```c
waitpid()
```

Например:

```c
int status;

waitpid(pid, &status, 0);
```

Затем используются macros:

```c
WIFEXITED(status)
```

и:

```c
WEXITSTATUS(status)
```

Логика:

```text
child process
      |
      v
    exit
      |
      v
 parent: waitpid()
      |
      v
получить status
      |
      v
shell->exit_status
```

---

# 10. Pipeline и `$?`

Pipeline особенно важен для Minishell.

Например:

```bash
false | true
echo $?
```

Pipeline:

```text
false
  |
  v
true
```

Для стандартного Shell `$?` после pipeline соответствует статусу **последней команды pipeline**.

Последняя команда:

```text
true
```

возвращает:

```text
0
```

Поэтому:

```text
$? → 0
```

---

## Другой пример

```bash
true | false
echo $?
```

Последняя команда:

```text
false
```

возвращает:

```text
1
```

Поэтому:

```text
$? → 1
```

---

# 10.1 Модель Pipeline

Для:

```bash
cmd1 | cmd2 | cmd3
```

можно мыслить так:

```text
cmd1 → status1
cmd2 → status2
cmd3 → status3

             ↓

      pipeline status

             ↓

          status3
```

Для базовой реализации Minishell:

```text
status pipeline
        =
status последней команды
```

---

# 11. `$?` внутри кавычек

`$?` подчиняется правилам Parameter Expansion.

---

## Без кавычек

```bash
echo $?
```

Expansion происходит.

---

## Двойные кавычки

```bash
echo "$?"
```

Expansion также происходит.

Например:

```text
exit_status = 42
```

тогда:

```bash
echo "$?"
```

становится концептуально:

```bash
echo "42"
```

---

## Одинарные кавычки

```bash
echo '$?'
```

Expansion **не происходит**.

Результат:

```text
$?
```

---

# 12. `$?` внутри одинарных кавычек

Например:

```bash
false
echo '$?'
```

Результат:

```text
$?
```

Почему?

Потому что всё внутри:

```text
'...'
```

рассматривается буквально.

То есть:

```text
'$?'
 ↓
literal text
 ↓
$?
```

---

# 13. `$?` внутри двойных кавычек

Например:

```bash
false
echo "$?"
```

Expansion происходит.

Если:

```text
exit_status = 1
```

то:

```text
"$?"
```

превращается в:

```text
"1"
```

Результат:

```text
1
```

---

# 14. `$?` вместе с другим текстом

Parameter Expansion может находиться внутри слова.

Например:

```bash
false
echo status=$?
```

Если:

```text
$? = 1
```

результат:

```text
status=1
```

---

## Текст до и после

```bash
echo xxx$?yyy
```

Если:

```text
$? = 1
```

результат:

```text
xxx1yyy
```

Логика:

```text
xxx + exit_status + yyy
```

---

# 15. `$?` и выполнение команд

Весь pipeline Minishell можно представить так:

```text
USER INPUT
    |
    v
  LEXER
    |
    v
  PARSER
    |
    v
 EXPANSION
    |
    +------ $?
    |        |
    |        v
    |   exit_status
    |
    v
FINAL ARGUMENTS
    |
    v
 EXECUTION
    |
    v
NEW EXIT STATUS
    |
    v
shell->exit_status
```

Главный цикл:

```text
старый exit status
        |
        v
    expand $?
        |
        v
  execute command
        |
        v
новый exit status
        |
        v
 сохранить его
```

---

# 16. `$?` после `echo`

Рассмотрим:

```bash
false
echo $?
```

До `echo`:

```text
exit_status = 1
```

Expansion:

```text
$?
 ↓
1
```

Команда:

```bash
echo 1
```

`echo` выполняется успешно.

Поэтому после него:

```text
exit_status = 0
```

---

# 16.1 Несколько `$?`

Очень полезный тест:

```bash
false
echo $?
echo $?
```

После `false`:

```text
exit_status = 1
```

Первый `echo` получает:

```text
1
```

Но сам `echo` успешно завершается.

Поэтому:

```text
exit_status = 0
```

Второй `echo $?` получает:

```text
0
```

Результат:

```text
1
0
```

---

# 17. `$?` после `cd`

Успешный:

```bash
cd /tmp
echo $?
```

Ожидаемо:

```text
0
```

Неуспешный:

```bash
cd /does/not/exist
echo $?
```

Результат должен быть ненулевым.

Логика:

```text
cd
 |
 +-- выполнить
 |
 +-- ошибка?
 |      |
 |      v
 |   вывести error
 |
 +-- вернуть status
          |
          v
    exit_status
```

---

# 18. `$?` после `export`

Успешный:

```bash
export TEST=hello
echo $?
```

Обычно:

```text
0
```

Некорректный:

```bash
export 123=hello
echo $?
```

Должен вернуть ненулевой статус.

Важно, чтобы builtin передавал результат обратно в Shell.

---

# 19. `$?` после `unset`

Например:

```bash
unset TEST
echo $?
```

Если команда выполнилась успешно:

```text
0
```

Логика:

```text
unset
  |
  v
return status
  |
  v
shell->exit_status
```

---

# 20. `$?` после `exit`

`exit` отличается от остальных builtins, потому что он завершает Minishell.

Например:

```bash
exit 42
```

Shell завершится с кодом:

```text
42
```

После этого внутри того же Shell уже нельзя выполнить:

```bash
echo $?
```

потому что Shell больше не работает.

Но родительский процесс может получить exit status Minishell.

---

# 21. `$?` и ошибки

Разные ошибки могут давать разные exit status.

Например:

```text
command not found
permission denied
file not found
invalid argument
syntax error
```

Все они связаны с ненулевым status, но конкретное значение зависит от ситуации.

Для Minishell необходимо воспроизводить поведение, требуемое subject и тестами.

---

## Command Not Found

Например:

```bash
does_not_exist
echo $?
```

Обычно Shell возвращает:

```text
127
```

Это традиционно означает:

```text
command not found
```

---

## Permission Denied

Если команда найдена, но не может быть выполнена, обычно используется:

```text
126
```

Главное различие:

```text
127 → команда не найдена

126 → команда найдена,
      но не может быть выполнена
```

---

# 22. `$?` и сигналы

Процесс может завершиться не через `exit()`, а из-за сигнала.

Например:

```text
SIGINT
SIGQUIT
```

Для дочернего процесса можно проверить:

```c
WIFSIGNALED(status)
```

и получить сигнал:

```c
WTERMSIG(status)
```

Логика:

```text
child
 |
 +---- normal exit
 |          |
 |          v
 |     WEXITSTATUS()
 |
 +---- signal
            |
            v
       WTERMSIG()
```

После этого Minishell должен установить правильный exit status в соответствии с поведением Shell и требованиями проекта.

---

# 23. `$?` в архитектуре Minishell

Хорошая архитектура может выглядеть так:

```text
                 +----------------+
                 |    t_shell     |
                 +----------------+
                 | env            |
                 | exit_status    |
                 +----------------+
                         |
              +----------+----------+
              |                     |
              v                     v
        Parameter Expansion      Execution
              |                     |
              |                     v
              |                command runs
              |                     |
              |                     v
              |                command status
              |                     |
              +----------<----------+
                         |
                    update status
```

---

# 23.1 Рекомендуемая структура

Например:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
} t_shell;
```

Тогда:

```c
shell->exit_status
```

является единственным источником значения для:

```bash
$?
```

---

# 24. Стратегия реализации

Лучше всего обрабатывать `$?` во время **Parameter Expansion**.

Допустим, пользователь вводит:

```bash
echo $?
```

Lexer должен определить:

```text
$
?
```

После этого Expansion определяет:

```text
это специальный параметр $?
```

и заменяет его текущим `exit_status`.

Концептуально:

```c
if (str[i] == '$' && str[i + 1] == '?')
{
    append_number(&result, shell->exit_status);
    i += 2;
}
```

---

# 24.1 Почему `$?` нужно обрабатывать отдельно?

Потому что:

```bash
$USER
```

и:

```bash
$?
```

работают по-разному.

Для:

```text
$USER
```

нужно:

```text
environment lookup
```

Для:

```text
$?
```

нужно:

```text
Shell state lookup
```

Поэтому логика может выглядеть так:

```c
if (next == '?')
    expand_exit_status();
else
    expand_environment_variable();
```

---

# 25. Псевдокод

Упрощённая функция:

```c
char *expand_word(char *word, t_shell *shell)
{
    char    *result;
    int     i;

    result = create_empty_string();
    i = 0;

    while (word[i])
    {
        if (word[i] == '$' && word[i + 1] == '?')
        {
            append_number(&result, shell->exit_status);
            i += 2;
        }
        else
        {
            append_char(&result, word[i]);
            i++;
        }
    }

    return (result);
}
```

Это только упрощённый пример.

Настоящая реализация должна учитывать:

```text
single quotes
double quotes
environment variables
$
$?
пустые переменные
word boundaries
```

---

# 25.1 Quote-Aware Expansion

Концептуально:

```c
while (word[i])
{
    if (word[i] == '\'' && !double_quoted)
    {
        toggle_single_quote();
        i++;
    }
    else if (word[i] == '"' && !single_quoted)
    {
        toggle_double_quote();
        i++;
    }
    else if (word[i] == '$'
        && word[i + 1] == '?'
        && !single_quoted)
    {
        append_number(&result, shell->exit_status);
        i += 2;
    }
    else
    {
        append_char(&result, word[i]);
        i++;
    }
}
```

Главное условие:

```c
!single_quoted
```

Потому что `$?` не должен расширяться внутри:

```bash
'$?'
```

---

# 26. Частые ошибки

## Ошибка 1 — считать `$?` Environment Variable

Неправильно:

```c
get_env_value(env, "?");
```

Правильно:

```c
shell->exit_status;
```

---

## Ошибка 2 — не сохранять status

Неправильная логика:

```text
command executes
      |
      v
status calculated
      |
      X
status потерян
```

Правильно:

```text
command executes
      |
      v
status
      |
      v
shell->exit_status
```

---

## Ошибка 3 — обновить status слишком рано

Для:

```bash
false
echo $?
```

нужно сначала получить:

```text
1
```

и только потом выполнить `echo`.

Правильная последовательность:

```text
false
 ↓
status = 1
 ↓
expand $?
 ↓
echo 1
 ↓
echo succeeds
 ↓
status = 0
```

---

## Ошибка 4 — расширять `$?` внутри single quotes

Неправильно:

```bash
echo '$?'
```

→

```text
0
```

Правильно:

```text
$?
```

---

## Ошибка 5 — не обновлять status после builtin

Например:

```bash
cd /invalid
echo $?
```

Если `cd` завершился ошибкой, Minishell обязан сохранить этот ненулевой status.

---

## Ошибка 6 — неправильный status pipeline

Для:

```bash
false | true
echo $?
```

нужно учитывать status последней команды pipeline:

```text
true → 0
```

---

## Ошибка 7 — путать error message и exit status

Например:

```text
cd: no such file or directory
```

— это **сообщение об ошибке**.

А:

```text
1
```

— это **exit status**.

Это разные вещи.

---

# 27. Тестирование

Для `$?` необходимо сделать отдельный набор тестов.

---

## Test 1 — `true`

```bash
true
echo $?
```

Ожидается:

```text
0
```

---

## Test 2 — `false`

```bash
false
echo $?
```

Ожидается:

```text
1
```

---

## Test 3 — успешный `echo`

```bash
echo hello
echo $?
```

Ожидается:

```text
0
```

---

## Test 4 — неизвестная команда

```bash
command_that_does_not_exist
echo $?
```

Проверь, что status соответствует ожидаемому поведению Shell.

---

## Test 5 — single quotes

```bash
false
echo '$?'
```

Ожидается:

```text
$?
```

---

## Test 6 — double quotes

```bash
false
echo "$?"
```

Ожидается:

```text
1
```

---

## Test 7 — текст вокруг `$?`

```bash
false
echo status=$?
```

Ожидается:

```text
status=1
```

---

## Test 8 — несколько `$?`

```bash
false
echo "$?" "$?"
```

Оба `$?` должны получить значение `1`.

Ожидается:

```text
1 1
```

Затем:

```bash
echo $?
```

Ожидается:

```text
0
```

потому что предыдущий `echo` успешно завершился.

---

## Test 9 — Pipeline

```bash
false | true
echo $?
```

Ожидается:

```text
0
```

---

## Test 10 — обратный Pipeline

```bash
true | false
echo $?
```

Ожидается:

```text
1
```

---

## Test 11 — успешный `cd`

```bash
cd /tmp
echo $?
```

Ожидается:

```text
0
```

---

## Test 12 — ошибочный `cd`

```bash
cd /does/not/exist
echo $?
```

Ожидается:

```text
non-zero
```

---

# 28. Checklist

## Основное понимание

* [ ] Я понимаю, что означает `$?`.
* [ ] Я понимаю, что такое exit status.
* [ ] Я знаю, что `0` обычно означает успех.
* [ ] Я знаю, что non-zero означает ошибку/неуспех.

---

## Реализация

* [ ] У меня есть `exit_status` в структуре Shell.
* [ ] `$?` не ищется в environment.
* [ ] `$?` обрабатывается во время Parameter Expansion.
* [ ] `exit_status` преобразуется в строку.
* [ ] Полученная строка вставляется в command arguments.

---

## Quotes

* [ ] `$?` расширяется без кавычек.
* [ ] `$?` расширяется внутри double quotes.
* [ ] `$?` НЕ расширяется внутри single quotes.

---

## Execution

* [ ] Builtins обновляют `exit_status`.
* [ ] External commands обновляют `exit_status`.
* [ ] Я правильно использую `waitpid()`.
* [ ] Я правильно извлекаю exit status child process.
* [ ] Я правильно обрабатываю pipeline.
* [ ] Я учитываю завершение через signals.

---

## Timing

* [ ] Я понимаю, что `$?` относится к предыдущей команде.
* [ ] Я понимаю, когда именно происходит Parameter Expansion.
* [ ] Я понимаю, почему `echo $?` получает status предыдущей команды.
* [ ] Я понимаю, почему следующий `$?` обычно становится `0` после успешного `echo`.

---

# 29. Проверка знаний

Попробуй ответить на вопросы, не заглядывая выше.

### Вопрос 1

Что означает:

```bash
$?
```

---

### Вопрос 2

Является ли `$?` environment variable?

---

### Вопрос 3

Где Minishell должен хранить значение `$?`?

---

### Вопрос 4

Какой результат:

```bash
true
echo $?
```

---

### Вопрос 5

Какой результат:

```bash
false
echo $?
```

---

### Вопрос 6

Что произойдёт здесь?

```bash
false
echo $?
echo $?
```

Объясни почему.

---

### Вопрос 7

Произойдёт ли expansion?

```bash
echo '$?'
```

---

### Вопрос 8

Произойдёт ли expansion?

```bash
echo "$?"
```

---

### Вопрос 9

Почему нельзя использовать:

```c
get_env_value(env, "?");
```

для реализации `$?`?

---

### Вопрос 10

Что должно произойти здесь?

```bash
false | true
echo $?
```

---

### Вопрос 11

Что должно произойти здесь?

```bash
true | false
echo $?
```

---

### Вопрос 12

Почему `$?` должен быть расширен до выполнения `echo`?

---

# 30. Главная модель понимания

Самое главное:

```text
                 COMMAND
                    |
                    v
                 execute
                    |
                    v
              exit status
                    |
                    v
          shell->exit_status
                    |
                    v
                  $?
```

Теперь полный пример:

```bash
false
echo $?
```

### Шаг 1

Запускается:

```text
false
```

### Шаг 2

`false` возвращает:

```text
1
```

### Шаг 3

Minishell сохраняет:

```c
shell->exit_status = 1;
```

### Шаг 4

Пользователь вводит:

```bash
echo $?
```

### Шаг 5

Parameter Expansion видит:

```text
$?
```

и получает:

```text
shell->exit_status
        ↓
        1
```

### Шаг 6

Команда становится:

```bash
echo 1
```

### Шаг 7

`echo` успешно завершается:

```text
0
```

### Шаг 8

Minishell обновляет:

```c
shell->exit_status = 0;
```

---

# Полный цикл

```text
┌────────────────────────┐
│    Выполнить команду   │
└────────────┬───────────┘
             ↓
┌────────────────────────┐
│ Получить exit status   │
└────────────┬───────────┘
             ↓
┌────────────────────────┐
│ shell->exit_status     │
└────────────┬───────────┘
             ↓
       пользователь
             ↓
┌────────────────────────┐
│       echo $?          │
└────────────┬───────────┘
             ↓
┌────────────────────────┐
│ Parameter Expansion    │
└────────────┬───────────┘
             ↓
       заменить `$?`
             ↓
┌────────────────────────┐
│        echo 1          │
└────────────┬───────────┘
             ↓
┌────────────────────────┐
│   команда завершена    │
└────────────┬───────────┘
             ↓
┌────────────────────────┐
│ exit_status = 0        │
└────────────────────────┘
```

---

# Главное правило

> **`$?` — это специальный параметр Shell, который содержит exit status предыдущей команды или pipeline. В Minishell этот status нужно хранить в состоянии Shell, использовать во время Parameter Expansion и обновлять после выполнения каждой команды.**

Главная цепочка, которую нужно запомнить:

```text
command execution
       ↓
получить status
       ↓
shell->exit_status
       ↓
Parameter Expansion
       ↓
$? → строковое значение exit_status
       ↓
выполнить следующую команду
       ↓
получить новый status
       ↓
обновить shell->exit_status
```
