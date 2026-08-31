# Understanding the Shell Processing Pipeline

## Table of Contents

* [1. What Is a Shell?](#1-what-is-a-shell)
* [2. The Complete Shell Processing Pipeline](#2-the-complete-shell-processing-pipeline)
* [3. Stage 1 — Read the Command Line](#3-stage-1--read-the-command-line)
* [4. Stage 2 — Lexical Analysis / Tokenization](#4-stage-2--lexical-analysis--tokenization)
* [5. Stage 3 — Parsing](#5-stage-3--parsing)
* [6. Stage 4 — Expansions](#6-stage-4--expansions)
* [7. Stage 5 — Redirections](#7-stage-5--redirections)
* [8. Stage 6 — Pipes](#8-stage-6--pipes)
* [9. Stage 7 — Process Creation](#9-stage-7--process-creation)
* [10. Stage 8 — File Descriptor Management](#10-stage-8--file-descriptor-management)
* [11. Stage 9 — Command Execution](#11-stage-9--command-execution)
* [12. Stage 10 — Waiting and Exit Status](#12-stage-10--waiting-and-exit-status)
* [13. Complete Example](#13-complete-example)
* [14. Builtins vs External Commands](#14-builtins-vs-external-commands)
* [15. Environment Variables](#15-environment-variables)
* [16. Signals](#16-signals)
* [17. Important System Calls](#17-important-system-calls)
* [18. File Descriptors](#18-file-descriptors)
* [19. Common Mistakes](#19-common-mistakes)
* [20. What We Need to Implement in Minishell](#20-what-we-need-to-implement-in-minishell)
* [21. Team Study Checklist](#21-team-study-checklist)
* [22. Questions We Must Be Able to Answer](#22-questions-we-must-be-able-to-answer)

---

# 1. What Is a Shell?

A shell is a program that provides an interface between the user and the operating system.

When the user types:

```bash
ls -la | grep ".c" > files.txt
```

the shell does **much more** than simply call `ls`.

It must:

1. Read the input.
2. Understand quotes and special characters.
3. Split the command into meaningful tokens.
4. Build an internal representation of the command.
5. Perform expansions.
6. Set up redirections.
7. Create pipes.
8. Create processes.
9. Connect processes using file descriptors.
10. Execute commands.
11. Wait for processes.
12. Return the correct exit status.

The most important concept for `minishell` is:

> **The shell is a command-processing pipeline.**

```text
User Input
    |
    v
Readline
    |
    v
Lexer / Tokenizer
    |
    v
Parser
    |
    v
Expansions
    |
    v
Redirections
    |
    v
Pipes
    |
    v
Fork
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
```

---

# 2. The Complete Shell Processing Pipeline

Consider:

```bash
cat < input.txt | grep "$USER" > result.txt
```

The shell conceptually processes it like this:

```text
                    USER
                     |
                     v
              "$USER" expansion
                     |
                     v
Input:
cat < input.txt | grep "$USER" > result.txt
                     |
                     v
                 LEXER
                     |
                     v
              TOKENS / WORDS
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
             REDIRECTION SETUP
                     |
                     v
                PIPE SETUP
                     |
                     v
                  FORK
                 /    \
                /      \
              cat      grep
               |         |
             stdin     stdin <- pipe
                       stdout -> result.txt
                 \      /
                  \    /
                WAITPID
                    |
                    v
              Exit Status
```

A key distinction:

### Parsing is not execution.

The shell first needs to understand **what the user requested**.

Only after that does it actually execute it.

---

# 3. Stage 1 — Read the Command Line

A shell normally waits for user input:

```text
$ minishell>
```

The user types:

```bash
echo hello
```

The shell receives the string:

```text
"echo hello"
```

For a `minishell` project, `readline()` is commonly used:

```c
char *line;

line = readline("minishell$ ");
```

After receiving input, the shell should:

* check whether input is empty;
* add it to history when appropriate;
* pass it to the lexer.

Example:

```c
if (line && *line)
    add_history(line);
```

---

# 4. Stage 2 — Lexical Analysis / Tokenization

The lexer converts the raw string into tokens.

For:

```bash
echo hello | grep h
```

we can obtain:

```text
WORD      echo
WORD      hello
PIPE      |
WORD      grep
WORD      h
```

A possible token representation:

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

And:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

---

## 4.1 Characters That Matter to the Lexer

The shell gives special meaning to:

```text
|   <   >   <<   >>
'   "
```

For example:

```bash
echo hello|grep h
```

must still be recognized as:

```text
echo
hello
|
grep
h
```

Spaces are not always enough to determine tokens.

---

## 4.2 Quotes

Single quotes:

```bash
'hello $USER'
```

Double quotes:

```bash
"hello $USER"
```

behave differently.

Inside single quotes:

```text
$USER
```

is normally treated literally.

Inside double quotes:

```text
$USER
```

can be expanded.

Therefore:

```bash
echo '$USER'
```

might produce:

```text
$USER
```

while:

```bash
echo "$USER"
```

might produce:

```text
alice
```

The lexer must therefore understand quote boundaries.

---

# 5. Stage 3 — Parsing

After tokenization, the parser determines the **structure** of the command.

Example:

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

The parser can transform this into something conceptually like:

```text
COMMAND 1
    argv:
        cat
        file.txt

    pipe
        |
        v

COMMAND 2
    argv:
        grep
        hello

    output:
        result.txt
```

---

## 5.1 Why We Need a Parser

Consider:

```bash
echo hello > output.txt | grep hello
```

The shell must know:

* which command owns `>`;
* what `output.txt` means;
* where the pipe separates commands.

Without parsing, execution code becomes extremely complicated.

---

# 6. Stage 4 — Expansions

After the shell understands the command structure, it performs the necessary expansions.

Important expansions include:

### Environment variables

```bash
echo $USER
```

If:

```text
USER=alice
```

then:

```text
$USER
```

becomes:

```text
alice
```

---

## 6.1 `$?`

`$?` represents the exit status of the previous command.

Example:

```bash
false
echo $?
```

Output:

```text
1
```

So the shell needs to maintain something like:

```c
int exit_status;
```

and update it after commands finish.

---

## 6.2 Expansion Inside Quotes

Example:

```bash
echo "$USER"
```

Variable expansion occurs.

But:

```bash
echo '$USER'
```

does not expand `$USER`.

This means expansion cannot be implemented as a simple:

```c
str_replace("$USER", value);
```

The shell must know the quoting context.

---

# 7. Stage 5 — Redirections

Redirections modify the standard input/output of a command.

There are four important forms.

---

## 7.1 Input Redirection `<`

```bash
cat < input.txt
```

Conceptually:

```text
input.txt
    |
    v
STDIN
    |
    v
  cat
```

The shell opens the file:

```c
fd = open("input.txt", O_RDONLY);
```

Then:

```c
dup2(fd, STDIN_FILENO);
```

After that:

```c
close(fd);
```

The `cat` process reads from `input.txt` as if the user had typed the data into stdin.

---

# 7.2 Output Redirection `>`

```bash
echo hello > output.txt
```

The shell opens:

```c
open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

Then:

```c
dup2(fd, STDOUT_FILENO);
```

Result:

```text
echo hello
    |
    v
STDOUT
    |
    v
output.txt
```

`>` normally truncates the file.

---

# 7.3 Append `>>`

```bash
echo hello >> output.txt
```

The file is opened with append behavior:

```c
open("output.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
```

The existing contents remain.

---

# 7.4 Heredoc `<<`

Example:

```bash
cat << EOF
hello
world
EOF
```

The shell reads lines until it finds:

```text
EOF
```

Conceptually:

```text
User
 |
 | hello
 | world
 |
 v
temporary input
 |
 v
STDIN
 |
 v
cat
```

A heredoc is especially important in `minishell` because it interacts with:

* pipes;
* signals;
* environment expansion;
* quotes;
* delimiters.

---

# 8. Stage 6 — Pipes

The pipe operator:

```bash
cmd1 | cmd2
```

connects:

```text
cmd1 stdout
     |
     v
   PIPE
     |
     v
cmd2 stdin
```

The shell creates a pipe:

```c
int pipefd[2];

pipe(pipefd);
```

After `pipe()`:

```text
pipefd[0] = read end
pipefd[1] = write end
```

---

## 8.1 Example

```bash
ls | grep ".c"
```

The shell creates:

```text
        PIPE
      +-------+
      |       |
      v       v
   write    read
      ^       ^
      |       |
     ls     grep
```

For `ls`:

```c
dup2(pipefd[1], STDOUT_FILENO);
```

For `grep`:

```c
dup2(pipefd[0], STDIN_FILENO);
```

---

# 9. Stage 7 — Process Creation

External commands normally execute in child processes.

The shell uses:

```c
pid_t pid = fork();
```

After `fork()` there are two processes:

```text
             Parent
               |
             fork()
             /    \
            /      \
       Parent      Child
```

The child prepares its environment and executes the command.

The parent normally waits.

---

# 10. Stage 8 — File Descriptor Management

This is one of the most important parts of `minishell`.

Unix processes have file descriptors.

The standard ones are:

```text
0 = STDIN
1 = STDOUT
2 = STDERR
```

---

## 10.1 Standard Input

```text
fd 0
```

Normally comes from the keyboard.

---

## 10.2 Standard Output

```text
fd 1
```

Normally goes to the terminal.

---

## 10.3 Standard Error

```text
fd 2
```

Used for error messages.

---

# 10.4 `dup2()`

`dup2()` allows us to replace a standard file descriptor.

Example:

```c
dup2(fd, STDOUT_FILENO);
```

This means:

```text
stdout -> fd
```

Now anything written to stdout goes to `fd`.

For a file:

```text
Before:

STDOUT ---> terminal


After dup2():

STDOUT ---> output.txt
```

For a pipe:

```text
STDOUT ---> pipe
```

---

# 10.5 Why Closing File Descriptors Matters

Consider:

```bash
cat file | grep hello
```

If the parent or another process keeps the pipe's write end open, `grep` may never receive EOF.

The process can appear to hang.

Therefore:

> Every process must close the pipe ends it does not need.

Example:

```c
close(pipefd[0]);
close(pipefd[1]);
```

at the appropriate places.

File descriptor leaks are one of the most common sources of bugs in minishell implementations.

---

# 11. Stage 9 — Command Execution

Once the command has been parsed and the file descriptors are configured, the shell executes it.

For external commands, this is usually:

```c
execve(path, argv, envp);
```

Important:

> `execve()` does not create a new process.

It replaces the current process image.

Typical sequence:

```text
fork()
  |
  v
child process
  |
  +--> setup redirections
  |
  +--> setup pipes
  |
  +--> execve()
  |
  v
external program
```

---

# 11.1 Finding the Executable

If the user types:

```bash
ls
```

the shell needs to find the executable.

The `PATH` environment variable may contain:

```text
/usr/local/bin
/usr/bin
/bin
```

The shell searches:

```text
/usr/local/bin/ls
/usr/bin/ls
/bin/ls
```

until it finds an executable.

Then:

```c
execve("/usr/bin/ls", argv, envp);
```

---

# 12. Stage 10 — Waiting and Exit Status

After starting commands, the parent shell must wait for child processes.

Usually:

```c
waitpid(pid, &status, 0);
```

For multiple processes:

```text
command 1
command 2
command 3
```

the shell may need to wait for multiple PIDs.

Example:

```c
waitpid(pid1, &status, 0);
waitpid(pid2, &status, 0);
```

---

## 12.1 Exit Status

Every command has an exit status.

Conventionally:

```text
0     = success
non-0 = error/failure
```

Example:

```bash
true
echo $?
```

returns:

```text
0
```

While:

```bash
false
echo $?
```

returns:

```text
1
```

The shell uses this value for:

```bash
$?
```

and for determining the result of command execution.

---

# 13. Complete Example

Let's analyze:

```bash
cat < input.txt | grep "$USER" > result.txt
```

Assume:

```text
USER=alice
```

---

## Step 1 — Read Input

Raw string:

```text
cat < input.txt | grep "$USER" > result.txt
```

---

## Step 2 — Lexer

Possible tokens:

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

## Step 3 — Parser

The parser creates two commands:

```text
COMMAND 1
    argv = ["cat"]
    stdin = input.txt
    stdout = pipe


COMMAND 2
    argv = ["grep", "$USER"]
    stdin = pipe
    stdout = result.txt
```

---

## Step 4 — Expansion

The shell expands:

```text
"$USER"
```

to:

```text
"alice"
```

So:

```text
grep "$USER"
```

becomes conceptually:

```text
grep "alice"
```

---

## Step 5 — Create Pipe

```c
pipe(pipefd);
```

Result:

```text
pipefd[0] = read
pipefd[1] = write
```

---

## Step 6 — Fork First Command

```c
pid1 = fork();
```

Child 1:

```text
cat
```

It needs:

```text
stdin  = input.txt
stdout = pipe
```

Therefore:

```c
fd = open("input.txt", O_RDONLY);
dup2(fd, STDIN_FILENO);
dup2(pipefd[1], STDOUT_FILENO);
```

Then:

```c
execve(...);
```

---

## Step 7 — Fork Second Command

```c
pid2 = fork();
```

Child 2:

```text
grep alice
```

It needs:

```text
stdin  = pipe
stdout = result.txt
```

Therefore:

```c
dup2(pipefd[0], STDIN_FILENO);

fd = open("result.txt",
          O_WRONLY | O_CREAT | O_TRUNC,
          0644);

dup2(fd, STDOUT_FILENO);
```

Then:

```c
execve(...);
```

---

## Step 8 — Parent Closes Pipe

The parent no longer needs:

```text
pipefd[0]
pipefd[1]
```

so it closes them.

---

## Step 9 — Wait

The parent waits for the children:

```c
waitpid(pid1, ...);
waitpid(pid2, ...);
```

---

## Final Architecture

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

This is the core shell processing pipeline.

---

# 14. Builtins vs External Commands

A shell has commands that are implemented internally.

These are called **builtins**.

Examples:

```text
echo
cd
pwd
export
unset
env
exit
```

External commands are separate executable programs:

```bash
ls
cat
grep
wc
sort
```

---

## 14.1 Why Builtins Are Special

Consider:

```bash
cd /tmp
```

If the shell executes `cd` in a child process:

```text
parent shell
     |
    fork
     |
   child
     |
    cd
```

the child's current directory changes.

But when the child exits:

```text
parent shell
```

is still in the old directory.

Therefore `cd` must normally execute inside the shell process itself when it is a standalone command.

The same principle applies to builtins that modify shell state, especially:

```text
cd
export
unset
exit
```

---

# 14.2 Builtins Inside Pipes

Now consider:

```bash
cd /tmp | echo hello
```

Pipeline execution changes the process context.

Therefore, builtin execution requires careful handling depending on whether the builtin is:

```text
standalone
```

or:

```text
inside a pipeline
```

This is an important part of the `minishell` architecture.

---

# 15. Environment Variables

The shell maintains an environment.

Example:

```text
USER=alice
HOME=/home/alice
PATH=/usr/bin:/bin
PWD=/home/alice
```

Conceptually:

```c
char **envp;
```

Environment variables are used by:

```bash
echo $HOME
```

and passed to external programs through:

```c
execve(path, argv, envp);
```

---

# 15.1 `export`

Example:

```bash
export NAME=Alice
```

The shell must modify its own environment.

Then:

```bash
echo $NAME
```

should return:

```text
Alice
```

---

# 15.2 `unset`

Example:

```bash
unset NAME
```

removes the variable from the shell environment.

---

# 16. Signals

The shell also has to handle Unix signals.

Important signals include:

```text
SIGINT   = Ctrl+C
SIGQUIT  = Ctrl+\
SIGTERM  = termination request
EOF      = Ctrl+D / input end
```

For example:

```text
Ctrl+C
```

should normally interrupt the current command without killing the shell itself.

This means signal behavior differs between:

```text
parent shell
```

and:

```text
child process
```

---

# 16.1 Ctrl+C

At the prompt:

```text
minishell$ 
```

pressing:

```text
Ctrl+C
```

should generally:

```text
cancel current input
display a new prompt
```

During execution of a child process, behavior is different.

Therefore signal handling must be designed together with process management.

---

# 17. Important System Calls

For a minishell, we need to understand these functions deeply.

## Input

```c
readline()
```

---

## Process management

```c
fork()
wait()
waitpid()
```

---

## Program execution

```c
execve()
```

---

## Pipes

```c
pipe()
```

---

## File management

```c
open()
close()
access()
```

---

## File descriptors

```c
dup()
dup2()
```

---

## Directory management

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

# 18. File Descriptors

Understanding file descriptors is essential.

Think of a file descriptor as an integer representing an open input/output connection.

Example:

```text
0 -> stdin
1 -> stdout
2 -> stderr
3 -> some opened file
4 -> another opened file
```

When we execute:

```c
fd = open("file.txt", O_RDONLY);
```

we might get:

```text
fd = 3
```

Then:

```c
dup2(fd, 0);
```

means:

```text
stdin -> file.txt
```

After that:

```c
read(0, buffer, size);
```

reads from `file.txt`.

---

# 19. Common Mistakes

## Mistake 1 — Thinking `fork()` executes the command

It doesn't.

```text
fork()
```

creates a process.

```text
execve()
```

replaces the process with the requested program.

Correct mental model:

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

## Mistake 2 — Forgetting to close pipe descriptors

This can cause commands such as:

```bash
cat file | grep text
```

to hang.

Always reason about:

```text
Who owns read end?
Who owns write end?
Who must close them?
```

---

## Mistake 3 — Doing redirections in the wrong order

Redirections have ordering semantics.

For example:

```bash
echo hello > file 2>&1
```

and:

```bash
echo hello 2>&1 > file
```

are not necessarily equivalent.

The shell processes redirections in command order.

This is why the parser must preserve the order of redirection operations.

---

## Mistake 4 — Treating quotes as ordinary characters

For:

```bash
echo "hello world"
```

the shell should produce one argument:

```text
"hello world"
```

not:

```text
hello
world
```

---

## Mistake 5 — Expanding variables without understanding quotes

These are different:

```bash
echo '$USER'
```

and:

```bash
echo "$USER"
```

The lexer/parser/expansion stages must preserve enough information to distinguish them.

---

## Mistake 6 — Executing `cd` only in a child

This:

```bash
fork();
cd("/tmp");
exit();
```

does not change the parent shell's working directory.

Standalone state-changing builtins must be handled in the shell process.

---

## Mistake 7 — Ignoring exit status

After:

```bash
command
```

the shell needs to know:

```text
Did it succeed?
Did it fail?
Why?
```

because:

```bash
echo $?
```

depends on that information.

---

# 20. What We Need to Implement in Minishell

A practical architecture can be divided into the following components.

```text
                    MINISHELL
                        |
        +---------------+---------------+
        |               |               |
      INPUT           PARSER         EXECUTION
        |               |               |
    readline()       lexer            fork()
        |               |              pipe()
    history          parser           dup2()
                        |              execve()
                    expansion         waitpid()
                        |
                  redirections
```

---

## Component 1 — Input

Responsibilities:

```text
readline
history
EOF handling
prompt
```

---

## Component 2 — Lexer

Responsibilities:

```text
recognize words
recognize pipes
recognize redirections
recognize quotes
detect invalid syntax where appropriate
```

---

## Component 3 — Parser

Responsibilities:

```text
construct command structure
separate pipeline commands
associate redirections
preserve argument order
```

---

## Component 4 — Expansion

Responsibilities:

```text
$VARIABLE
$?
quote behavior
environment lookup
```

---

## Component 5 — Redirections

Responsibilities:

```text
<
>
>>
<<
```

and:

```c
open()
dup2()
close()
```

---

## Component 6 — Execution

Responsibilities:

```text
fork()
pipe()
execve()
builtin execution
PATH lookup
```

---

## Component 7 — Process Management

Responsibilities:

```text
waitpid()
exit status
signals
child processes
```

---

# 21. Team Study Checklist

Before implementing the execution part of `minishell`, the team should understand each topic.

## Unix Processes

* [ ] Understand what a process is.
* [ ] Understand parent and child processes.
* [ ] Understand `fork()`.
* [ ] Understand the return value of `fork()`.
* [ ] Understand `execve()`.
* [ ] Understand why `fork()` and `execve()` are normally used together.
* [ ] Understand `waitpid()`.

---

## File Descriptors

* [ ] Understand `stdin`.
* [ ] Understand `stdout`.
* [ ] Understand `stderr`.
* [ ] Understand `open()`.
* [ ] Understand `close()`.
* [ ] Understand `dup()`.
* [ ] Understand `dup2()`.
* [ ] Understand why file descriptors must be closed.

---

## Pipes

* [ ] Understand `pipe()`.
* [ ] Know `pipefd[0]` is the read end.
* [ ] Know `pipefd[1]` is the write end.
* [ ] Understand how stdout becomes a pipe.
* [ ] Understand how stdin becomes a pipe.
* [ ] Understand EOF.
* [ ] Understand why unused pipe ends must be closed.

---

## Parsing

* [ ] Understand tokens.
* [ ] Understand words.
* [ ] Understand operators.
* [ ] Understand quotes.
* [ ] Understand syntax errors.
* [ ] Understand command boundaries.
* [ ] Understand pipeline boundaries.

---

## Expansion

* [ ] Understand `$VARIABLE`.
* [ ] Understand `$?`.
* [ ] Understand single quotes.
* [ ] Understand double quotes.
* [ ] Understand when expansion happens.
* [ ] Understand how expansion affects arguments.

---

## Redirections

* [ ] Understand `<`.
* [ ] Understand `>`.
* [ ] Understand `>>`.
* [ ] Understand `<<`.
* [ ] Understand `open()` flags.
* [ ] Understand `dup2()`.
* [ ] Understand redirection order.

---

## Builtins

* [ ] Understand why `cd` must affect the parent shell.
* [ ] Understand `export`.
* [ ] Understand `unset`.
* [ ] Understand `exit`.
* [ ] Understand builtin behavior inside pipelines.

---

## Signals

* [ ] Understand `SIGINT`.
* [ ] Understand `SIGQUIT`.
* [ ] Understand Ctrl+C.
* [ ] Understand Ctrl+.
* [ ] Understand Ctrl+D.
* [ ] Understand parent/child signal behavior.

---

# 22. Questions We Must Be Able to Answer

Before starting serious implementation, every team member should be able to explain these without looking at documentation.

### Processes

1. What does `fork()` return?
2. What is the difference between parent and child after `fork()`?
3. What does `execve()` do?
4. Does `execve()` create a new process?
5. Why do we normally call `fork()` before `execve()`?

### Pipes

6. What does `pipe()` return?
7. Which descriptor reads?
8. Which descriptor writes?
9. How do we connect `ls` to `grep`?
10. Why can a pipe cause a process to hang?

### File descriptors

11. What are descriptors `0`, `1`, and `2`?
12. What does `dup2(fd, STDOUT_FILENO)` do?
13. Why do we close `fd` after `dup2()`?
14. What happens if a process doesn't close an unused pipe descriptor?

### Redirections

15. How does `<` work?
16. How does `>` work?
17. What is the difference between `>` and `>>`?
18. How does `<<` work?
19. Why does redirection order matter?

### Parsing

20. What is the difference between lexing and parsing?
21. Why can't we simply split the command using spaces?
22. How should quotes affect tokenization?
23. How does the parser identify separate commands in a pipeline?

### Execution

24. What happens when we execute:

```bash
ls -la | grep ".c" > result.txt
```

from beginning to end?

25. Which process executes `ls`?

26. Which process executes `grep`?

27. Where does `ls` stdout go?

28. Where does `grep` stdin come from?

29. Where does `grep` stdout go?

30. Which process waits for the children?

---

# Final Mental Model

The most important thing for the team to understand is this:

```text
                USER INPUT
                    |
                    v
              +-----------+
              |  READLINE  |
              +-----------+
                    |
                    v
              +-----------+
              |   LEXER   |
              +-----------+
                    |
                    v
              +-----------+
              |   PARSER  |
              +-----------+
                    |
                    v
              +-----------+
              | EXPANSION |
              +-----------+
                    |
                    v
           +-------------------+
           | COMMAND STRUCTURE |
           +-------------------+
                    |
                    v
          +---------------------+
          | REDIRECTIONS / PIPE |
          +---------------------+
                    |
                    v
                +-------+
                | FORK  |
                +-------+
                  /   \
                 /     \
                v       v
             CHILD     CHILD
                |       |
              dup2()  dup2()
                |       |
             execve() execve()
                |       |
                v       v
             COMMAND  COMMAND
                 \     /
                  \   /
                   v v
                WAITPID
                    |
                    v
               EXIT STATUS
                    |
                    v
                 PROMPT
```

The shell is therefore **not simply an `execve()` wrapper**.

It is a complete system that transforms:

```text
human-readable command
```

into:

```text
tokens
    ↓
syntax tree / command structure
    ↓
expanded arguments
    ↓
file descriptor configuration
    ↓
processes
    ↓
executed programs
    ↓
exit status
```

If the team understands this pipeline clearly, the implementation of `minishell` becomes much easier because every piece of code has a specific responsibility in the overall architecture.
