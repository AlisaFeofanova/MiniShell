# Понимание Pipeline обработки команд Shell

## Содержание

* [1. Что такое Shell](#1-что-такое-shell)
* [2. Общий Pipeline обработки команды](#2-общий-pipeline-обработки-команды)
* [3. Этап 1 — Чтение команды](#3-этап-1--чтение-команды)
* [4. Этап 2 — Lexer / Tokenization](#4-этап-2--lexer--tokenization)
* [5. Этап 3 — Parser](#5-этап-3--parser)
* [6. Этап 4 — Expansion](#6-этап-4--expansion)
* [7. Этап 5 — Redirections](#7-этап-5--redirections)
* [8. Этап 6 — Pipes](#8-этап-6--pipes)
* [9. Этап 7 — Создание процессов](#9-этап-7--создание-процессов)
* [10. Этап 8 — File Descriptors](#10-этап-8--file-descriptors)
* [11. Этап 9 — Выполнение команды](#11-этап-9--выполнение-команды)
* [12. Этап 10 — Ожидание и Exit Status](#12-этап-10--ожидание-и-exit-status)
* [13. Полный разбор примера](#13-полный-разбор-примера)
* [14. Builtins и External Commands](#14-builtins-и-external-commands)
* [15. Environment Variables](#15-environment-variables)
* [16. Signals](#16-signals)
* [17. Основные System Calls](#17-основные-system-calls)
* [18. File Descriptors подробно](#18-file-descriptors-подробно)
* [19. Частые ошибки](#19-частые-ошибки)
* [20. Что необходимо реализовать в Minishell](#20-что-необходимо-реализовать-в-minishell)
* [21. Что должен знать каждый участник команды](#21-что-должен-знать-каждый-участник-команды)
* [22. Контрольные вопросы](#22-контрольные-вопросы)
* [23. Главная схема](#23-главная-схема)

---

# 1. Что такое Shell

**Shell** — это программа, которая является посредником между пользователем и операционной системой.

Например, пользователь вводит:

```bash
ls -la | grep ".c" > files.txt
```

Shell не просто запускает `ls`.

Он должен:

1. Получить строку от пользователя.
2. Понять специальные символы.
3. Разделить строку на токены.
4. Определить структуру команды.
5. Выполнить необходимые expansions.
6. Обработать redirections.
7. Создать pipes.
8. Создать процессы.
9. Настроить file descriptors.
10. Запустить программы.
11. Дождаться завершения процессов.
12. Получить exit status.
13. Показать новый prompt.

Главная идея:

> **Shell — это pipeline обработки команды.**

```text
User Input
    |
    v
readline()
    |
    v
Lexer
    |
    v
Parser
    |
    v
Expansion
    |
    v
Redirections
    |
    v
Pipes
    |
    v
fork()
    |
    v
File Descriptor Setup
    |
    v
Builtin / execve()
    |
    v
waitpid()
    |
    v
Exit Status
    |
    v
New Prompt
```

---

# 2. Общий Pipeline обработки команды

Возьмём команду:

```bash
cat < input.txt | grep "$USER" > result.txt
```

Shell должен превратить эту строку в работающую систему процессов.

Общая схема:

```text
                    USER INPUT
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
                     PIPES
                        |
                        v
                      FORK
                     /    \
                    /      \
                   v        v
                 cat      grep
                  |         |
                  |         |
                  v         v
                execve()  execve()
                    \       /
                     \     /
                      WAIT
                       |
                       v
                 EXIT STATUS
                       |
                       v
                     PROMPT
```

Очень важно понимать:

> **Parsing и execution — это разные этапы.**

Сначала Shell должен понять:

> Что пользователь хочет сделать?

И только после этого:

> Как это выполнить?

---

# 3. Этап 1 — Чтение команды

Shell находится в состоянии ожидания:

```text
minishell$
```

Пользователь вводит:

```bash
echo hello
```

Shell получает строку:

```text
"echo hello"
```

В `minishell` обычно используется:

```c
readline()
```

Например:

```c
char *line;

line = readline("minishell$ ");
```

После получения строки необходимо:

* проверить `NULL`;
* обработать `Ctrl+D`;
* при необходимости добавить команду в history;
* передать строку lexer.

Например:

```c
if (line && *line)
    add_history(line);
```

---

# 4. Этап 2 — Lexer / Tokenization

## Что такое Lexer?

Lexer превращает обычную строку в набор **tokens**.

Например:

```bash
echo hello | grep h
```

превращается примерно в:

```text
WORD       echo
WORD       hello
PIPE       |
WORD       grep
WORD       h
```

То есть:

```text
"echo hello | grep h"
```

становится:

```text
[echo] [hello] [|] [grep] [h]
```

---

## 4.1 Типы токенов

Можно создать enum:

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

И структуру:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

Например:

```bash
echo hello > file.txt
```

может превратиться в:

```text
TOKEN_WORD       "echo"
TOKEN_WORD       "hello"
TOKEN_REDIR_OUT  ">"
TOKEN_WORD       "file.txt"
```

---

# 4.2 Специальные символы

Shell имеет специальные операторы:

```text
|
<
>
<<
>>
```

Также важны:

```text
'
"
```

Например:

```bash
echo hello|grep hello
```

должно распознаться как:

```text
echo
hello
|
grep
hello
```

Даже если между `hello` и `|` нет пробела.

---

# 4.3 Одинарные кавычки `'`

Внутри:

```bash
'$USER'
```

`$USER` должен восприниматься как обычный текст.

То есть:

```bash
echo '$USER'
```

даёт:

```text
$USER
```

а не значение переменной.

---

# 4.4 Двойные кавычки `"`

В двойных кавычках переменные могут расширяться:

```bash
echo "$USER"
```

Если:

```text
USER=alice
```

результатом будет:

```text
alice
```

Поэтому lexer должен понимать контекст кавычек.

---

# 5. Этап 3 — Parser

После lexer мы имеем tokens.

Parser должен понять:

> Как эти tokens связаны друг с другом?

Например:

```bash
cat file.txt | grep hello > result.txt
```

Tokens:

```text
WORD cat
WORD file.txt
PIPE
WORD grep
WORD hello
REDIR_OUT
WORD result.txt
```

Parser должен построить структуру:

```text
COMMAND 1
    argv:
        cat
        file.txt

    stdout:
        PIPE

COMMAND 2
    argv:
        grep
        hello

    stdout:
        result.txt
```

---

# 5.1 Почему нужен Parser?

Нельзя просто сделать:

```c
split(line, ' ');
```

Потому что Shell имеет:

```text
quotes
pipes
redirections
heredoc
variables
```

Например:

```bash
echo "hello world"
```

должен создать **один аргумент**:

```text
"hello world"
```

а не два:

```text
hello
world
```

---

# 5.2 Parser и Pipeline

Команда:

```bash
ls | grep ".c" | wc -l
```

содержит три команды:

```text
COMMAND 1
    ls

    |
    v

COMMAND 2
    grep ".c"

    |
    v

COMMAND 3
    wc -l
```

Parser должен определить границы каждой команды.

---

# 6. Этап 4 — Expansion

После parsing Shell должен выполнить необходимые expansions.

Самый известный пример:

```bash
echo $USER
```

Если:

```text
USER=alice
```

то:

```text
$USER
```

заменяется на:

```text
alice
```

---

# 6.1 Environment Variable

Например:

```bash
echo $HOME
```

Если:

```text
HOME=/home/alice
```

результат:

```text
/home/alice
```

---

# 6.2 `$?`

Очень важная переменная:

```bash
$?
```

Она содержит exit status предыдущей команды.

Например:

```bash
false
echo $?
```

Результат:

```text
1
```

А:

```bash
true
echo $?
```

даёт:

```text
0
```

Поэтому Shell должен хранить текущий exit status:

```c
int exit_status;
```

---

# 6.3 Quotes + Expansion

Эти команды различаются:

```bash
echo '$USER'
```

и:

```bash
echo "$USER"
```

Первая:

```text
$USER
```

Вторая:

```text
alice
```

Следовательно:

> Expansion должен учитывать информацию о кавычках.

---

# 7. Этап 5 — Redirections

Redirection позволяет изменить:

```text
stdin
stdout
stderr
```

для конкретной команды.

Основные операторы:

```text
<
>
>>
<<
```

---

# 7.1 Input `<`

Команда:

```bash
cat < input.txt
```

означает:

```text
input.txt
    |
    v
 STDIN
    |
    v
  cat
```

Shell делает:

```c
fd = open("input.txt", O_RDONLY);
```

Затем:

```c
dup2(fd, STDIN_FILENO);
```

После этого:

```c
close(fd);
```

Теперь `cat` читает из файла через `stdin`.

---

# 7.2 Output `>`

Команда:

```bash
echo hello > output.txt
```

означает:

```text
echo
  |
  v
STDOUT
  |
  v
output.txt
```

Shell открывает файл:

```c
fd = open("output.txt",
          O_WRONLY | O_CREAT | O_TRUNC,
          0644);
```

Затем:

```c
dup2(fd, STDOUT_FILENO);
```

Теперь stdout направлен в файл.

`O_TRUNC` означает:

> старое содержимое файла будет удалено.

---

# 7.3 Append `>>`

Команда:

```bash
echo hello >> output.txt
```

использует:

```c
O_APPEND
```

Например:

```text
Before:

Hello


Command:

echo World >> output.txt


After:

Hello
World
```

---

# 7.4 Heredoc `<<`

Пример:

```bash
cat << EOF
hello
world
EOF
```

Shell читает строки:

```text
hello
world
```

до тех пор, пока не встретит:

```text
EOF
```

Схематично:

```text
User
 |
 | hello
 | world
 |
 v
Heredoc input
 |
 v
STDIN
 |
 v
cat
```

Heredoc особенно важен в `minishell`, потому что связан с:

* signals;
* pipes;
* expansion;
* quotes;
* file descriptors.

---

# 8. Этап 6 — Pipes

Pipe:

```bash
cmd1 | cmd2
```

соединяет:

```text
cmd1 stdout
     |
     v
    PIPE
     |
     v
cmd2 stdin
```

Для создания pipe используется:

```c
int pipefd[2];

pipe(pipefd);
```

После этого:

```text
pipefd[0] = read end
pipefd[1] = write end
```

---

# 8.1 Пример

```bash
ls | grep ".c"
```

Схема:

```text
+--------+       +------+       +----------+
|   ls   | ----> | PIPE | ----> | grep ".c"|
+--------+       +------+       +----------+
   stdout          | |             stdin
                   |
             read / write
```

Для `ls`:

```c
dup2(pipefd[1], STDOUT_FILENO);
```

Для `grep`:

```c
dup2(pipefd[0], STDIN_FILENO);
```

---

# 8.2 Pipeline из трёх команд

Например:

```bash
cat file.txt | grep hello | wc -l
```

Схема:

```text
       PIPE 1                PIPE 2

cat ----------> grep ----------------> wc
stdout          stdin/stdout            stdin
```

То есть:

```text
cat stdout
    |
    v
pipe1
    |
    v
grep stdin

grep stdout
    |
    v
pipe2
    |
    v
wc stdin
```

---

# 9. Этап 7 — Создание процессов

Для external command Shell обычно использует:

```c
fork();
```

После `fork()` существуют два процесса:

```text
             Parent
                |
              fork()
              /    \
             /      \
            v        v
         Parent     Child
```

Child обычно выполняет:

```text
redirections
pipes
execve()
```

Parent обычно:

```text
close()
waitpid()
```

---

# 9.1 Что возвращает `fork()`?

Очень важно знать:

```c
pid_t pid = fork();
```

Если:

```text
pid == -1
```

произошла ошибка.

Если:

```text
pid == 0
```

мы находимся в child.

Если:

```text
pid > 0
```

мы находимся в parent.

Например:

```c
pid = fork();

if (pid == -1)
{
    // error
}
else if (pid == 0)
{
    // child
}
else
{
    // parent
}
```

---

# 10. Этап 8 — File Descriptors

Это одна из самых важных тем для `minishell`.

Каждый Unix process имеет file descriptors.

Стандартные:

```text
0 = stdin
1 = stdout
2 = stderr
```

---

# 10.1 STDIN

```text
0
```

Обычно:

```text
keyboard -> stdin
```

---

# 10.2 STDOUT

```text
1
```

Обычно:

```text
stdout -> terminal
```

---

# 10.3 STDERR

```text
2
```

Используется для ошибок:

```text
stderr -> terminal
```

---

# 10.4 Что делает `open()`?

Например:

```c
int fd;

fd = open("file.txt", O_RDONLY);
```

Если:

```text
0, 1, 2
```

уже заняты, система может вернуть:

```text
fd = 3
```

Теперь:

```text
3 -> file.txt
```

---

# 10.5 Что делает `dup2()`?

Очень важная функция:

```c
dup2(fd, STDOUT_FILENO);
```

означает:

```text
stdout -> fd
```

Например:

```text
Before:

STDOUT ---> terminal


After:

STDOUT ---> file.txt
```

Поэтому:

```c
echo hello > file.txt
```

может быть реализовано концептуально так:

```c
fd = open("file.txt", ...);
dup2(fd, STDOUT_FILENO);
close(fd);
execve(...);
```

---

# 10.6 Почему после `dup2()` нужно делать `close()`?

Если:

```c
fd = open(...);
dup2(fd, STDOUT_FILENO);
```

то теперь существует:

```text
fd       ---> file
STDOUT   ---> file
```

Если дополнительный `fd` больше не нужен:

```c
close(fd);
```

Остаётся:

```text
STDOUT ---> file
```

Это предотвращает file descriptor leaks.

---

# 10.7 Закрытие Pipe

Очень важно правильно закрывать pipe descriptors.

Например:

```text
pipefd[0] = read
pipefd[1] = write
```

Если процесс не использует один из них, он должен его закрыть.

Неправильное закрытие может привести к:

```text
program hangs
```

Особенно часто это проявляется в:

```bash
cat file | grep text
```

Почему?

Потому что `grep` ждёт EOF.

Если какой-то процесс всё ещё держит write-end pipe открытым, EOF может не наступить.

---

# 11. Этап 9 — Выполнение команды

После того как:

* команда распарсена;
* expansions выполнены;
* pipes созданы;
* redirections настроены;

можно запускать программу.

Для external commands используется:

```c
execve()
```

---

# 11.1 Важное различие `fork()` и `execve()`

`fork()`:

```text
создаёт новый процесс
```

`execve()`:

```text
заменяет текущую программу новым executable
```

`execve()` **не создаёт новый процесс**.

Обычно:

```text
fork()
  |
  v
child
  |
  +--> dup2()
  |
  +--> close()
  |
  +--> execve()
  |
  v
external program
```

---

# 11.2 Поиск команды через PATH

Пользователь вводит:

```bash
ls
```

Shell должен найти executable.

Например:

```text
PATH=/usr/local/bin:/usr/bin:/bin
```

Shell проверяет:

```text
/usr/local/bin/ls
/usr/bin/ls
/bin/ls
```

Когда найден executable:

```c
execve("/usr/bin/ls", argv, envp);
```

---

# 12. Этап 10 — Ожидание и Exit Status

После запуска child-процессов parent должен дождаться их завершения.

Используется:

```c
waitpid()
```

Например:

```c
waitpid(pid, &status, 0);
```

---

# 12.1 Exit Status

У команды есть exit status.

Обычно:

```text
0     = success
non-0 = error
```

Например:

```bash
true
echo $?
```

получаем:

```text
0
```

А:

```bash
false
echo $?
```

получаем:

```text
1
```

---

# 12.2 Почему Exit Status важен?

Потому что:

```bash
echo $?
```

должен знать результат предыдущей команды.

Например:

```bash
ls
echo $?
```

Shell должен сохранить status, который вернул `ls`.

---

# 13. Полный разбор примера

Рассмотрим:

```bash
cat < input.txt | grep "$USER" > result.txt
```

Пусть:

```text
USER=alice
```

---

## Шаг 1 — Readline

Получаем:

```text
cat < input.txt | grep "$USER" > result.txt
```

---

## Шаг 2 — Lexer

Получаем:

```text
WORD       cat
REDIR_IN   <
WORD       input.txt
PIPE       |
WORD       grep
WORD       "$USER"
REDIR_OUT  >
WORD       result.txt
```

---

## Шаг 3 — Parser

Создаётся две команды.

### Command 1

```text
argv:
    cat

stdin:
    input.txt

stdout:
    pipe
```

### Command 2

```text
argv:
    grep
    "$USER"

stdin:
    pipe

stdout:
    result.txt
```

---

## Шаг 4 — Expansion

```text
"$USER"
```

становится:

```text
"alice"
```

То есть:

```bash
grep "$USER"
```

становится концептуально:

```bash
grep "alice"
```

---

## Шаг 5 — Создаём pipe

```c
pipe(pipefd);
```

Получаем:

```text
pipefd[0] = read
pipefd[1] = write
```

---

## Шаг 6 — Первый child

Создаётся:

```c
pid1 = fork();
```

Child должен запустить:

```text
cat
```

Его stdin:

```text
input.txt
```

Его stdout:

```text
pipe
```

Поэтому:

```c
fd = open("input.txt", O_RDONLY);

dup2(fd, STDIN_FILENO);
dup2(pipefd[1], STDOUT_FILENO);

close(fd);
```

После этого:

```text
cat stdin
    |
    v
input.txt


cat stdout
    |
    v
pipe
```

Затем:

```c
execve(...);
```

---

# Шаг 7 — Второй child

Создаётся:

```c
pid2 = fork();
```

Child запускает:

```text
grep alice
```

Его stdin:

```text
pipe
```

Его stdout:

```text
result.txt
```

Поэтому:

```c
dup2(pipefd[0], STDIN_FILENO);

fd = open("result.txt",
          O_WRONLY | O_CREAT | O_TRUNC,
          0644);

dup2(fd, STDOUT_FILENO);

close(fd);
```

Затем:

```c
execve(...);
```

---

# Шаг 8 — Parent закрывает pipe

Parent больше не использует:

```text
pipefd[0]
pipefd[1]
```

поэтому:

```c
close(pipefd[0]);
close(pipefd[1]);
```

---

# Шаг 9 — Parent ждёт

```c
waitpid(pid1, ...);
waitpid(pid2, ...);
```

---

# Финальная схема

```text
                   input.txt
                       |
                       v
                 +-----------+
                 |    cat    |
                 +-----------+
                       |
                       | stdout
                       v
                     PIPE
                       |
                       | stdin
                       v
                 +-----------+
                 | grep alice|
                 +-----------+
                       |
                       | stdout
                       v
                  result.txt
```

Именно это и должен уметь организовать `minishell`.

---

# 14. Builtins и External Commands

Shell содержит встроенные команды — **builtins**.

Например:

```text
echo
cd
pwd
export
unset
env
exit
```

External commands:

```text
ls
cat
grep
wc
sort
```

---

# 14.1 Почему Builtins особенные?

Рассмотрим:

```bash
cd /tmp
```

Если выполнить:

```text
fork()
  |
  v
child
  |
  v
cd /tmp
```

то изменится directory только child.

После завершения:

```text
parent
```

останется в старой директории.

Поэтому `cd` должен изменять состояние **самого shell process**.

---

# 14.2 Builtins, изменяющие состояние Shell

Особенно важны:

```text
cd
export
unset
exit
```

Они могут изменять состояние самого shell.

Например:

```bash
export NAME=Alice
```

После выполнения:

```bash
echo $NAME
```

должно вывести:

```text
Alice
```

Если `export` выполнить только в child process, parent shell не получит изменение.

---

# 14.3 Builtin внутри Pipeline

Например:

```bash
export NAME=Alice | echo hello
```

или:

```bash
cd /tmp | echo hello
```

требуют отдельного внимания.

Поведение builtin зависит от того:

```text
standalone command
```

это или:

```text
command inside pipeline
```

Это одна из важных частей архитектуры `minishell`.

---

# 15. Environment Variables

Shell хранит environment.

Например:

```text
USER=alice
HOME=/home/alice
PATH=/usr/bin:/bin
PWD=/home/alice
```

Environment передаётся external program через:

```c
execve(path, argv, envp);
```

---

# 15.1 `export`

Команда:

```bash
export NAME=Alice
```

должна изменить environment Shell.

После:

```bash
echo $NAME
```

получаем:

```text
Alice
```

---

# 15.2 `unset`

Команда:

```bash
unset NAME
```

удаляет:

```text
NAME
```

из environment.

---

# 16. Signals

Shell должен правильно обрабатывать signals.

Основные:

```text
SIGINT
SIGQUIT
SIGTERM
```

Также важен:

```text
Ctrl+D
```

---

# 16.1 Ctrl+C

Когда пользователь находится на prompt:

```text
minishell$
```

и нажимает:

```text
Ctrl+C
```

Shell должен отменить текущий ввод и показать новый prompt.

Например:

```text
minishell$ ^C
minishell$
```

Но если в этот момент выполняется child process, поведение другое.

Поэтому signal handling должен учитывать:

```text
parent shell
child process
pipeline
heredoc
```

---

# 16.2 SIGQUIT

`Ctrl+\` связан с:

```text
SIGQUIT
```

Поведение shell и child process должно быть настроено отдельно.

---

# 17. Основные System Calls

Для `minishell` необходимо хорошо понимать следующие функции.

## Input

```c
readline()
```

---

## Processes

```c
fork()
wait()
waitpid()
```

---

## Execution

```c
execve()
```

---

## Pipes

```c
pipe()
```

---

## Files

```c
open()
close()
access()
```

---

## File Descriptors

```c
dup()
dup2()
```

---

## Directories

```c
chdir()
getcwd()
```

---

## Environment

```c
getenv()
```

---

## Signals

```c
signal()
sigaction()
sigemptyset()
sigaddset()
```

---

# 18. File Descriptors подробно

Можно представить file descriptor как число, через которое process обращается к открытому ресурсу.

Например:

```text
0 -> stdin
1 -> stdout
2 -> stderr
3 -> input.txt
4 -> pipe
5 -> another file
```

---

## Пример

```c
fd = open("file.txt", O_RDONLY);
```

Получаем:

```text
fd = 3
```

Теперь:

```text
3 -> file.txt
```

Затем:

```c
dup2(fd, STDIN_FILENO);
```

получаем:

```text
0 -> file.txt
3 -> file.txt
```

После:

```c
close(fd);
```

получаем:

```text
0 -> file.txt
```

Таким образом команда:

```bash
cat < file.txt
```

работает потому, что Shell перенаправляет:

```text
stdin -> file.txt
```

---

# 19. Частые ошибки

## Ошибка 1 — Думать, что `fork()` запускает программу

Нет.

```text
fork()
```

создаёт процесс.

```text
execve()
```

запускает другую программу внутри текущего процесса, заменяя старый program image.

Правильная схема:

```text
fork()
  |
  v
child
  |
  v
dup2()
  |
  v
execve()
```

---

# Ошибка 2 — Не закрывать pipe descriptors

Например:

```bash
cat file | grep text
```

может зависнуть, если write-end pipe остаётся открытым там, где он не должен быть открыт.

Всегда нужно спрашивать:

```text
Кто читает pipe?
Кто пишет?
Кто должен закрыть read-end?
Кто должен закрыть write-end?
```

---

# Ошибка 3 — Неправильный порядок redirections

Рассмотрим:

```bash
command > file 2>&1
```

и:

```bash
command 2>&1 > file
```

Это не одно и то же.

Shell обрабатывает redirections **в определённом порядке**.

Поэтому parser должен сохранять порядок операций.

---

# Ошибка 4 — Разделять команду только по пробелам

Нельзя делать просто:

```c
split(line, ' ');
```

Потому что:

```bash
echo "hello world"
```

должно дать:

```text
argv[0] = "echo"
argv[1] = "hello world"
```

а не:

```text
argv[1] = "hello"
argv[2] = "world"
```

---

# Ошибка 5 — Игнорировать quotes

Нужно различать:

```bash
echo '$USER'
```

и:

```bash
echo "$USER"
```

---

# Ошибка 6 — Выполнять `cd` только в child

Если сделать:

```c
fork();
cd("/tmp");
```

directory изменится только у child.

Parent Shell останется в старой директории.

---

# Ошибка 7 — Игнорировать Exit Status

После:

```bash
command
```

Shell должен знать:

```text
success?
error?
signal?
exit code?
```

Потому что следующая команда:

```bash
echo $?
```

использует этот результат.

---

# 20. Что необходимо реализовать в Minishell

Удобно разделить проект на несколько логических частей.

```text
                     MINISHELL
                         |
        +----------------+----------------+
        |                |                |
        v                v                v
      INPUT            PARSER         EXECUTION
        |                |                |
    readline()         lexer            fork()
    history            parser           pipe()
    signals            expansion        dup2()
                       redirections      execve()
                                        waitpid()
```

---

## Component 1 — Input

Отвечает за:

```text
readline()
history
prompt
Ctrl+D
```

---

## Component 2 — Lexer

Отвечает за:

```text
words
quotes
pipes
redirections
tokens
```

---

## Component 3 — Parser

Отвечает за:

```text
command structure
pipeline structure
redirections
arguments
syntax
```

---

## Component 4 — Expansion

Отвечает за:

```text
$USER
$HOME
$?
quotes
environment variables
```

---

## Component 5 — Redirections

Отвечает за:

```text
<
>
>>
<<
```

и:

```c
open()
dup2()
close()
```

---

## Component 6 — Execution

Отвечает за:

```text
fork()
pipe()
execve()
PATH
builtins
```

---

## Component 7 — Process Management

Отвечает за:

```text
waitpid()
exit status
signals
children
```

---

# 21. Что должен знать каждый участник команды

Перед началом серьёзной реализации каждый член команды должен понимать следующие темы.

## Processes

* [ ] Что такое process.
* [ ] Что такое parent process.
* [ ] Что такое child process.
* [ ] Что делает `fork()`.
* [ ] Что возвращает `fork()`.
* [ ] Что делает `execve()`.
* [ ] Разницу между `fork()` и `execve()`.
* [ ] Что делает `waitpid()`.

---

## File Descriptors

* [ ] Что такое file descriptor.
* [ ] Что такое `stdin`.
* [ ] Что такое `stdout`.
* [ ] Что такое `stderr`.
* [ ] Что делает `open()`.
* [ ] Что делает `close()`.
* [ ] Что делает `dup()`.
* [ ] Что делает `dup2()`.

---

## Pipes

* [ ] Что делает `pipe()`.
* [ ] Что такое `pipefd[0]`.
* [ ] Что такое `pipefd[1]`.
* [ ] Как stdout одного process становится stdin другого.
* [ ] Что такое EOF.
* [ ] Почему pipe может вызвать зависание.
* [ ] Почему важно закрывать unused descriptors.

---

## Parsing

* [ ] Что такое token.
* [ ] Что такое lexer.
* [ ] Что такое parser.
* [ ] Разница между lexer и parser.
* [ ] Как работают quotes.
* [ ] Как определяются pipes.
* [ ] Как определяются redirections.
* [ ] Как определяются отдельные commands.

---

## Expansion

* [ ] `$VARIABLE`.
* [ ] `$?`.
* [ ] Single quotes.
* [ ] Double quotes.
* [ ] Когда выполняется expansion.
* [ ] Как expansion влияет на `argv`.

---

## Redirections

* [ ] `<`.
* [ ] `>`.
* [ ] `>>`.
* [ ] `<<`.
* [ ] `open()` flags.
* [ ] `dup2()`.
* [ ] Порядок redirections.

---

## Builtins

* [ ] Почему `cd` должен изменять parent shell.
* [ ] Как работает `export`.
* [ ] Как работает `unset`.
* [ ] Как работает `exit`.
* [ ] Чем standalone builtin отличается от builtin внутри pipeline.

---

## Signals

* [ ] `SIGINT`.
* [ ] `SIGQUIT`.
* [ ] `SIGTERM`.
* [ ] `Ctrl+C`.
* [ ] `Ctrl+\`.
* [ ] `Ctrl+D`.
* [ ] Разница signal behavior parent/child.

---

# 22. Контрольные вопросы

Перед тем как переходить к реализации execution pipeline, каждый участник должен самостоятельно ответить:

### Processes

1. Что делает `fork()`?
2. Что возвращает `fork()`?
3. Что происходит после `fork()`?
4. Что делает `execve()`?
5. Создаёт ли `execve()` новый process?
6. Почему обычно используется комбинация `fork()` + `execve()`?
7. Зачем нужен `waitpid()`?

### Pipes

8. Что делает `pipe()`?
9. Что находится в `pipefd[0]`?
10. Что находится в `pipefd[1]`?
11. Как соединить stdout `ls` со stdin `grep`?
12. Почему программа может зависнуть из-за незакрытого pipe descriptor?

### File Descriptors

13. Что такое FD `0`?
14. Что такое FD `1`?
15. Что такое FD `2`?
16. Что делает `dup2()`?
17. Зачем закрывать descriptor после `dup2()`?
18. Что произойдёт, если забыть закрыть pipe descriptor?

### Redirections

19. Как работает:

```bash
cat < file
```

20. Как работает:

```bash
echo hello > file
```

21. Разница между:

```bash
>
```

и:

```bash
>>
```

22. Как работает:

```bash
cat << EOF
hello
EOF
```

23. Почему порядок redirections имеет значение?

### Parsing

24. В чём разница между lexer и parser?
25. Почему нельзя использовать простой `split()` по пробелам?
26. Как работают single quotes?
27. Как работают double quotes?
28. Как parser определяет pipeline?

### Expansion

29. Что делает:

```bash
echo $USER
```

30. Чем отличаются:

```bash
echo '$USER'
```

и:

```bash
echo "$USER"
```

31. Что означает:

```bash
$?
```

### Builtins

32. Почему `cd` должен изменять parent shell?
33. Почему `export` должен изменять environment shell?
34. Что происходит с builtin внутри pipeline?

---

# 23. Главная схема

Если запомнить только одну схему из этого README, запомните эту:

```text
                     USER
                      |
                      v
              +---------------+
              |   readline()  |
              +---------------+
                      |
                      v
              +---------------+
              |     LEXER     |
              +---------------+
                      |
                      v
                  TOKENS
                      |
                      v
              +---------------+
              |     PARSER    |
              +---------------+
                      |
                      v
             COMMAND STRUCTURE
                      |
                      v
              +---------------+
              |   EXPANSION   |
              +---------------+
                      |
                      v
          +-----------------------+
          | REDIRECTIONS + PIPES  |
          +-----------------------+
                      |
                      v
                   fork()
                  /      \
                 /        \
                v          v
             CHILD       CHILD
                |           |
              dup2()      dup2()
                |           |
              close()     close()
                |           |
              execve()    execve()
                |           |
                v           v
             COMMAND     COMMAND
                 \         /
                  \       /
                   \     /
                    v   v
                  waitpid()
                      |
                      v
                 EXIT STATUS
                      |
                      v
                  NEW PROMPT
```

---

# Главное, что нужно понять

`minishell` — это не просто программа, которая делает:

```c
execve();
```

Это система, которая преобразует:

```text
команду пользователя
        |
        v
       lexer
        |
        v
      tokens
        |
        v
      parser
        |
        v
 command structure
        |
        v
    expansion
        |
        v
 redirections/pipes
        |
        v
     processes
        |
        v
    file descriptors
        |
        v
      execve
        |
        v
     processes
        |
        v
     waitpid
        |
        v
    exit status
```

### Самая важная концепция

Для каждой команды необходимо уметь ответить на 5 вопросов:

```text
1. ЧТО запускаем?
2. В КАКОМ process запускаем?
3. ОТКУДА команда читает stdin?
4. КУДА команда пишет stdout/stderr?
5. КАКОЙ exit status она возвращает?
```

Например:

```bash
cat < input.txt | grep hello > result.txt
```

ответ:

```text
Command 1:
    cat

    stdin  <- input.txt
    stdout -> pipe

Command 2:
    grep hello

    stdin  <- pipe
    stdout -> result.txt
```

Если команда состоит из нескольких частей:

```bash
A | B | C
```

нужно мыслить так:

```text
A stdout -> PIPE 1 -> B stdin
B stdout -> PIPE 2 -> C stdin
```

А если есть redirections:

```bash
A < input | B > output
```

то:

```text
input -> A -> pipe -> B -> output
```

Именно это понимание **Shell Processing Pipeline** является фундаментом для правильной реализации `minishell`.
