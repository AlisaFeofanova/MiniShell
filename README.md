# Minishell — 2-Person Team Plan

## 👥 Team

This project is developed by a team of two students.

### Developer 1 — Lexer / Parser / Expansion

Main responsibilities:

* Readline / user input
* Lexer
* Tokenization
* Quotes
* Parser
* Syntax validation
* Environment variables
* Expansion
* `$?`
* Preparing the command structure for the Executor

### Developer 2 — Executor / Processes / Builtins

Main responsibilities:

* Executor
* `fork`
* `execve`
* `waitpid`
* `pipe`
* `dup2`
* `open`
* `close`
* PATH resolution
* Redirections
* Builtins
* Exit status
* Signals

> **Important:** The responsibilities are divided, but architecture, data structures, and interfaces between modules must be designed together.

---

# 🎯 Goal of the First 2 Days

By the meeting on Friday, we should:

* [ ] Understand the Minishell requirements
* [ ] Understand the required Bash behavior
* [ ] Study the main Unix system calls
* [ ] Agree on the project architecture
* [ ] Design the main data structures
* [ ] Define interfaces between modules
* [ ] Create the project structure
* [ ] Create the Makefile
* [ ] Create Git branches
* [ ] Assign responsibilities
* [ ] Implement the initial project skeleton
* [ ] Start the Lexer
* [ ] Start the Executor
* [ ] Prepare questions and unclear points for the Friday meeting

## ❌ What we do NOT need to finish in 2 days

Do not try to:

* Finish the Lexer
* Finish the Parser
* Implement all builtins
* Finish the Executor
* Implement every edge case
* Build a complete Minishell

The goal is to **understand the project and build a solid foundation**.

---

# 📚 Part 1 — What BOTH Developers Must Study

Both developers should understand the following topics before starting serious implementation.

---

## 1. Processes

Study:

```c
fork()
execve()
wait()
waitpid()
exit()
```

Understand:

* What is a process?
* What happens after `fork()`?
* What is a parent process?
* What is a child process?
* What is a PID?
* What is a PPID?
* What does `execve()` do?
* Why does the program's execution change after `execve()`?
* Why do we need `waitpid()`?

### Example

```c
pid_t pid;

pid = fork();
```

You should understand what happens in both:

```text
Parent
   |
   └── fork()
          |
          └── Child
```

---

# 2. File Descriptors

Understand:

```text
0 → stdin
1 → stdout
2 → stderr
```

Study:

```c
open()
close()
dup()
dup2()
```

Especially understand:

```c
dup2(fd, STDOUT_FILENO);
```

and:

```c
dup2(fd, STDIN_FILENO);
```

### Example

Understand how Bash implements:

```bash
echo hello > file.txt
```

Conceptually:

```text
echo
  |
stdout
  |
file.txt
```

---

# 3. Pipes

Study:

```c
pipe()
fork()
dup2()
close()
waitpid()
```

Understand:

```bash
ls | grep ".c"
```

as:

```text
             pipe
        ┌─────────────┐
        │             │
       ls            grep
        │             │
     stdout          stdin
```

You must understand:

* How data moves through a pipe
* Which file descriptors are duplicated
* Which file descriptors must be closed
* Why leaving pipe FDs open can cause a program to hang

---

# 4. Environment Variables

Understand:

```c
char **envp;
```

Study common variables:

```text
PATH
HOME
USER
PWD
OLDPWD
SHLVL
_
```

Understand:

```bash
echo $PATH
echo $HOME
```

and:

```bash
export TEST=hello
echo $TEST
unset TEST
```

---

# 5. PATH

Understand how the shell finds:

```bash
ls
cat
grep
```

when the user does not provide an absolute path.

For example:

```text
PATH=/usr/local/bin:/usr/bin:/bin
```

The shell searches:

```text
/usr/local/bin/ls
/usr/bin/ls
/bin/ls
```

Study:

```c
access()
execve()
```

---

# 6. Signals

Study:

```c
signal()
sigaction()
```

Understand:

```text
SIGINT
SIGQUIT
SIGTERM
```

and:

```text
Ctrl+C
Ctrl+\
Ctrl+D
```

Test Bash behavior with:

```bash
cat
```

Then press:

```text
Ctrl+C
Ctrl+\
Ctrl+D
```

Write down what happens.

---

# 7. Readline

Study:

```c
readline()
add_history()
```

Understand how to create:

```text
minishell$
```

and how command history works.

---

# 🧠 Part 2 — DAY 1

# Day 1 — Understanding & Architecture

## 🎯 Goal

By the end of Day 1, both developers should understand the complete data flow:

```text
INPUT
  ↓
LEXER
  ↓
TOKENS
  ↓
PARSER
  ↓
COMMAND STRUCTURE
  ↓
EXPANSION
  ↓
REDIRECTIONS
  ↓
EXECUTOR
  ↓
PROCESS
  ↓
EXIT STATUS
```

---

# 👥 Task 1 — Read the Minishell Subject

### Both developers

Read the complete project subject.

Create:

```text
docs/requirements.md
```

Add a checklist:

```markdown
- [ ] readline
- [ ] command history
- [ ] pipes
- [ ] redirections
- [ ] heredoc
- [ ] single quotes
- [ ] double quotes
- [ ] environment variables
- [ ] `$?`
- [ ] PATH
- [ ] builtins
- [ ] signals
- [ ] exit status
- [ ] error handling
```

---

# 👥 Task 2 — Study Bash Behavior

Create:

```text
docs/bash_tests.md
```

Test commands directly in Bash.

## Basic commands

```bash
echo hello
pwd
env
ls
ls -la
```

## Arguments

```bash
echo hello world
echo "hello world"
echo 'hello world'
```

## Quotes

```bash
echo '$USER'
echo "$USER"
echo "'"
echo '"'
echo ""
echo ''
```

## Variables

```bash
echo $USER
echo $HOME
echo $?
```

## Pipes

```bash
ls | grep ".c"
ls | grep ".c" | wc -l
```

## Redirections

```bash
echo hello > file
cat < file
echo world >> file
```

## Heredoc

```bash
cat << EOF
hello
world
EOF
```

## Errors

```bash
command_that_does_not_exist
cat nonexistent
cd nonexistent
```

For each test, record:

```text
Command:
Expected output:
Expected exit status:
Notes:
```

---

# 👩‍💻 Developer 1 — DAY 1

# Lexer / Parser Research

## Study Tokens

Understand the difference between:

```text
WORD
PIPE
REDIRECT_IN
REDIRECT_OUT
REDIRECT_APPEND
HEREDOC
```

Example:

```bash
echo hello | grep hello > output
```

Should conceptually become:

```text
WORD        echo
WORD        hello
PIPE        |
WORD        grep
WORD        hello
REDIRECT    >
WORD        output
```

---

## Study Quotes

Understand the difference between:

```bash
echo "$USER"
echo '$USER'
```

Understand why a simple:

```c
ft_split(input, ' ');
```

cannot be used as the Minishell parser.

---

## Study Expansion

Understand:

```text
$USER
$HOME
$?
```

and:

```bash
"$USER"
'$USER'
```

---

## Practical Task

Create:

```text
src/lexer/
```

Initial files:

```text
lexer.c
token.c
token_utils.c
```

Create token types:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC
} t_token_type;
```

And a token structure:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
} t_token;
```

At this stage, the complete Lexer is not required.

The main goal is to agree on the data structure and start implementing the token system.

---

# 👨‍💻 Developer 2 — DAY 1

# Executor Research

## Study

```c
fork()
execve()
waitpid()
pipe()
dup2()
open()
close()
access()
```

---

## Practical Exercise 1 — `fork()`

Create:

```text
fork_test.c
```

Experiment with:

```text
Parent
  |
fork()
  |
Child
```

Understand:

```text
PID
PPID
fork() return value
```

---

## Practical Exercise 2 — `execve()`

Create:

```text
exec_test.c
```

Execute:

```text
/bin/ls
```

using:

```c
execve()
```

Understand exactly what happens to the current process.

---

## Practical Exercise 3 — Pipe

Create:

```text
pipe_test.c
```

Implement:

```bash
ls | wc -l
```

without Minishell.

The goal is to fully understand:

```text
pipe()
fork()
dup2()
execve()
close()
waitpid()
```

---

# 👥 DAY 1 — Shared Architecture Task

Design the main Minishell structures together.

A possible starting point:

```c
typedef struct s_shell
{
    char        **envp;
    int         exit_status;
    t_token     *tokens;
    t_cmd       *commands;
} t_shell;
```

Command structure:

```c
typedef struct s_cmd
{
    char            **argv;
    char            *input_file;
    char            *output_file;
    int             append;
    int             heredoc;
    struct s_cmd    *next;
} t_cmd;
```

> These are only examples. The final structures must be agreed upon by both developers.

---

# 🧩 Architecture

Our initial architecture:

```text
                    readline()
                        |
                        v
                     INPUT
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
                   t_cmd list
                        |
                        v
                   EXPANSION
                        |
                        v
                  REDIRECTIONS
                        |
                        v
                    EXECUTOR
                        |
             +----------+----------+
             |                     |
             v                     v
          BUILTIN               EXECVE
             |                     |
             +----------+----------+
                        |
                        v
                   EXIT STATUS
```

---

# 🧠 Part 3 — DAY 2

# Day 2 — Skeleton & First Implementation

## 🎯 Goal

By the end of Day 2 we should have:

```text
Makefile
main.c
shell initialization
readline loop
token structures
command structures
basic lexer
executor skeleton
Git branches
documentation
```

---

# 👩‍💻 Developer 1 — DAY 2

## Task 1 — Basic Lexer

Implement basic tokenization.

Input:

```bash
echo hello
```

Expected tokens:

```text
WORD echo
WORD hello
```

---

## Task 2 — Pipe Token

Input:

```bash
echo hello | cat
```

Expected tokens:

```text
WORD echo
WORD hello
PIPE
WORD cat
```

---

## Task 3 — Redirection Tokens

Recognize:

```text
<
>
>>
<<
```

Example:

```bash
cat < file
```

Expected:

```text
WORD cat
REDIR_IN
WORD file
```

---

## Task 4 — Quotes

Do not incorrectly split:

```bash
echo "hello world"
```

or:

```bash
echo 'hello world'
```

---

## Deliverable

By the end of Day 2:

```text
lexer(input)
      ↓
t_token list
```

should work for basic cases.

---

# 👨‍💻 Developer 2 — DAY 2

## Task 1 — Readline Loop

Create:

```text
src/main/main.c
src/main/shell.c
```

The program should display:

```text
minishell$
```

After:

```bash
echo hello
```

it can initially print debug information:

```text
INPUT: echo hello
```

---

## Task 2 — Basic Executor Skeleton

Create:

```text
execute_command()
```

Prepare the executor to eventually run:

```bash
ls
pwd
/bin/ls
```

---

## Task 3 — PATH

Prepare:

```c
find_command_path()
```

It should eventually find:

```text
ls
```

using:

```text
$PATH
```

---

## Task 4 — Process Handling

Prepare functions such as:

```c
spawn_process()
wait_for_process()
```

Clearly define where the following will be used:

```c
fork()
execve()
waitpid()
```

---

# 👥 DAY 2 — Shared Task

## Create the Repository Structure

Suggested structure:

```text
minishell/
│
├── Makefile
├── README.md
│
├── includes/
│   └── minishell.h
│
├── src/
│   ├── main/
│   │   ├── main.c
│   │   └── shell.c
│   │
│   ├── lexer/
│   │   ├── lexer.c
│   │   ├── token.c
│   │   └── token_utils.c
│   │
│   ├── parser/
│   │   ├── parser.c
│   │   └── parser_utils.c
│   │
│   ├── expansion/
│   │   └── expansion.c
│   │
│   ├── executor/
│   │   ├── executor.c
│   │   ├── process.c
│   │   ├── pipe.c
│   │   └── path.c
│   │
│   ├── builtins/
│   │   ├── echo.c
│   │   ├── cd.c
│   │   ├── pwd.c
│   │   ├── export.c
│   │   ├── unset.c
│   │   ├── env.c
│   │   └── exit.c
│   │
│   ├── redirections/
│   │   ├── redirect.c
│   │   └── heredoc.c
│   │
│   ├── signals/
│   │   └── signals.c
│   │
│   └── utils/
│       └── ...
│
└── docs/
    ├── requirements.md
    ├── bash_tests.md
    ├── architecture.md
    └── functions.md
```

---

# 🌿 Git Strategy

Create:

```text
main
dev
```

Developer 1:

```text
feature/lexer
feature/parser
feature/expansion
```

Developer 2:

```text
feature/executor
feature/builtins
feature/redirections
feature/signals
```

---

# Git Rules

Before starting work:

```bash
git pull
```

After completing a small task:

```bash
git add .
git commit -m "feat: implement basic lexer"
git push
```

Avoid huge commits such as:

```text
"everything"
"minishell done"
"fix stuff"
```

Prefer focused commits:

```text
feat: add token types
feat: implement basic lexer
feat: add pipe token
feat: add command structure
feat: initialize shell
```

---

# 📝 Questions for the Friday Meeting

Both developers must be able to answer these questions.

## Architecture

* [ ] What is `t_shell`?
* [ ] What is `t_token`?
* [ ] What is `t_cmd`?
* [ ] Who creates each structure?
* [ ] Who frees each structure?
* [ ] How does Lexer communicate with Parser?
* [ ] How does Parser communicate with Executor?

## Lexer

* [ ] What token types do we need?
* [ ] How do we handle quotes?
* [ ] How do we handle operators?
* [ ] How do we determine where a word ends?

## Parser

* [ ] How do we represent multiple commands?
* [ ] How do we represent pipes?
* [ ] How do we represent redirections?

## Executor

* [ ] When do we create a child process?
* [ ] When must a builtin run in the parent?
* [ ] How do we redirect stdin/stdout?
* [ ] Who closes file descriptors?

## Environment

* [ ] Where do we store the environment?
* [ ] How will `export` work?
* [ ] How will `unset` work?
* [ ] How will `PWD` / `OLDPWD` be updated?

## Signals

* [ ] What should Ctrl+C do at the prompt?
* [ ] What should Ctrl+C do during a command?
* [ ] What should Ctrl+\ do?
* [ ] What should Ctrl+D do?

---

# 🚨 Questions to Answer Before Serious Implementation

Both developers should independently try to answer:

### 1. Why can't `cd` always run in a child process?

```bash
cd /tmp
```

### 2. What happens with:

```bash
ls | cd /tmp
```

### 3. Why must unused pipe file descriptors be closed?

### 4. What happens if we never call:

```c
waitpid()
```

### 5. What is the difference between:

```bash
echo '$USER'
echo "$USER"
```

### 6. What happens internally when executing:

```bash
echo hello > file
```

Think about:

```text
open
dup2
close
execve
```

### 7. How does the shell find:

```bash
ls
```

using `$PATH`?

### 8. Where does the value of:

```bash
echo $?
```

come from?

### 9. Why must:

```bash
export TEST=hello
```

modify the environment of the Minishell itself?

### 10. What happens to file descriptors when executing:

```bash
cat < input | grep hello > output
```

---

# 🧪 Minimum Tests After Day 2

The complete Minishell does **not** need to work yet.

However, the team should be able to demonstrate progress with:

```bash
echo hello
```

```bash
ls
```

```bash
pwd
```

```bash
echo hello | cat
```

And the Lexer should recognize:

```bash
echo hello | grep hello > output
```

as:

```text
WORD echo
WORD hello
PIPE
WORD grep
WORD hello
REDIR_OUT
WORD output
```

---

# 📊 Definition of Done — Friday

The first stage is complete when:

* [ ] Both developers understand the overall architecture.
* [ ] Roles are clearly assigned.
* [ ] Git branches are created.
* [ ] Makefile exists.
* [ ] Header exists.
* [ ] Main data structures are agreed upon.
* [ ] Readline loop exists.
* [ ] Lexer skeleton exists.
* [ ] Executor skeleton exists.
* [ ] Both developers understand `fork/execve/waitpid`.
* [ ] Both developers understand `pipe/dup2`.
* [ ] Redirections are understood.
* [ ] Quotes are understood.
* [ ] `$PATH` is understood.
* [ ] `$?` is understood.
* [ ] Bash test cases are documented.
* [ ] Each developer has completed at least one small implementation task.
* [ ] Open architecture questions are documented.
* [ ] `docs/architecture.md` contains the agreed design.

---

# 🚀 Roadmap After Friday

## WEEK 1

```text
Lexer
   ↓
Parser
   ↓
Basic Executor
   ↓
Pipes
   ↓
Basic Redirections
```

## WEEK 2

```text
Expansion
   ↓
Environment
   ↓
Builtins
   ↓
Heredoc
   ↓
Signals
```

## WEEK 3

```text
Integration
   ↓
Error handling
   ↓
Memory management
   ↓
Valgrind
   ↓
Edge cases
   ↓
Bash comparison
```

## FINAL

```text
Norminette
   ↓
Tests
   ↓
Memory leaks
   ↓
Crash testing
   ↓
Peer evaluation preparation
```

---

# ⭐ Main Rule

Do not try to write Minishell all at once.

Build it layer by layer:

```text
INPUT
  ↓
TOKENS
  ↓
COMMAND
  ↓
EXPANSION
  ↓
EXECUTION
```

Every layer must have:

1. A clearly defined input.
2. A clearly defined output.
3. An assigned developer.
4. Tests.
5. A clear memory ownership/freeing rule.

> **Before writing code, both developers must understand what data one module gives to the next module.**

This is the key to working in parallel without creating unnecessary merge conflicts or rewriting each other's code.
