# Create Basic Parser Structures

## Table of Contents

* [1. Task Goal](#1-task-goal)
* [2. What Is a Parser?](#2-what-is-a-parser)
* [3. Lexer vs Parser](#3-lexer-vs-parser)
* [4. Parser Responsibilities](#4-parser-responsibilities)
* [5. Why Do We Need Parser Structures?](#5-why-do-we-need-parser-structures)
* [6. Basic Shell Grammar](#6-basic-shell-grammar)
* [7. Command Structure](#7-command-structure)
* [8. Redirections](#8-redirections)
* [9. Pipes](#9-pipes)
* [10. AST Structure](#10-ast-structure)
* [11. Basic Data Structures](#11-basic-data-structures)
* [12. Token Structure](#12-token-structure)
* [13. AST Node Structure](#13-ast-node-structure)
* [14. Parser Context](#14-parser-context)
* [15. Parser Functions](#15-parser-functions)
* [16. Parsing a Simple Command](#16-parsing-a-simple-command)
* [17. Parsing Redirections](#17-parsing-redirections)
* [18. Parsing Pipes](#18-parsing-pipes)
* [19. Example AST](#19-example-ast)
* [20. Error Handling](#20-error-handling)
* [21. Common Mistakes](#21-common-mistakes)
* [22. Testing](#22-testing)
* [23. Final Checklist](#23-final-checklist)
* [24. Overall Architecture](#24-overall-architecture)

---

# 1. Task Goal

The task:

> **Create Basic Parser Structures**

means creating the data structures and basic functions that will allow Minishell to transform the token list produced by the Lexer into a structure that represents the command.

The general pipeline is:

```text
Input
  ↓
Lexer
  ↓
Tokens
  ↓
Parser
  ↓
Command / AST
  ↓
Expansion
  ↓
Execution
```

For example:

```bash
echo hello
```

The Lexer may produce:

```text
WORD("echo")
WORD("hello")
```

The Parser must transform this into a command structure:

```text
COMMAND
 ├── command: echo
 └── argument: hello
```

---

# 2. What Is a Parser?

A Parser takes tokens from the Lexer and determines their grammatical meaning.

The Lexer answers:

> "What are the individual pieces of the input?"

The Parser answers:

> "How are these pieces related?"

For example:

```bash
echo hello | grep h
```

The Lexer identifies:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("h")
```

The Parser understands:

```text
COMMAND
   |
   +── PIPE
       |
       +── COMMAND
```

More specifically:

```text
        PIPE
       /    \
   COMMAND COMMAND
    /  \     /  \
  echo hello grep h
```

---

# 3. Lexer vs Parser

It is important not to mix their responsibilities.

## Lexer

Input:

```bash
echo hello > file | grep h
```

Output:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("grep")
WORD("h")
```

The Lexer identifies tokens.

---

## Parser

The Parser receives:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("grep")
WORD("h")
```

and creates:

```text
PIPE
├── COMMAND
│   ├── echo
│   ├── hello
│   └── > file
│
└── COMMAND
    ├── grep
    └── h
```

The Parser identifies the relationships between tokens.

---

# 4. Parser Responsibilities

For a basic Minishell Parser, the main responsibilities are:

* [ ] Recognize commands.
* [ ] Recognize arguments.
* [ ] Recognize pipes.
* [ ] Recognize redirections.
* [ ] Associate redirection operators with their filenames.
* [ ] Build command structures.
* [ ] Detect basic syntax errors.
* [ ] Preserve the order of commands.
* [ ] Prepare a structure that the executor can use.

The Parser should NOT execute commands.

For example:

```bash
ls -la
```

The Parser should create:

```text
command = ls
arguments = [-la]
```

It should NOT call `execve()`.

---

# 5. Why Do We Need Parser Structures?

Without a structured representation, execution becomes difficult.

Consider:

```bash
cat file.txt | grep hello > result.txt
```

The executor needs to know:

```text
Command 1:
    program = cat
    argument = file.txt

Pipe

Command 2:
    program = grep
    argument = hello
    output = result.txt
```

The Parser creates this information.

---

# 6. Basic Shell Grammar

A simplified Minishell grammar can be represented as:

```text
input
    → pipeline

pipeline
    → command
    → command PIPE pipeline

command
    → words
    → words redirections

redirection
    → REDIR_IN WORD
    → REDIR_OUT WORD
    → APPEND WORD
    → HEREDOC WORD
```

A simpler conceptual grammar:

```text
pipeline
    |
    +── command
    |
    +── PIPE
    |
    +── command
```

---

# 7. Command Structure

A command can contain:

```text
COMMAND
├── command name
├── arguments
└── redirections
```

For:

```bash
grep hello file.txt
```

we have:

```text
COMMAND
├── command = grep
├── arg = hello
└── arg = file.txt
```

---

# 7.1 Example

Input:

```bash
ls -la /tmp
```

Structure:

```text
COMMAND
├── argv[0] = "ls"
├── argv[1] = "-la"
└── argv[2] = "/tmp"
```

This structure is very close to what the executor eventually needs.

---

# 8. Redirections

Shell supports four basic redirection operators in Minishell:

```text
<   input
>   output
>>  append
<<  heredoc
```

Each redirection requires a target.

Examples:

```bash
cat < input.txt
```

```bash
echo hello > output.txt
```

```bash
echo hello >> output.txt
```

```bash
cat << EOF
hello
EOF
```

The Parser should associate the operator with its target.

---

# 8.1 Redirection Structure

For:

```bash
echo hello > output.txt
```

we can represent:

```text
COMMAND
├── argv
│   ├── echo
│   └── hello
│
└── REDIRECTION
    ├── type = OUTPUT
    └── target = output.txt
```

---

# 8.2 Multiple Redirections

Input:

```bash
cat < input.txt > output.txt
```

Structure:

```text
COMMAND
├── cat
│
├── REDIR_IN
│   └── input.txt
│
└── REDIR_OUT
    └── output.txt
```

There can be multiple redirections attached to the same command.

---

# 9. Pipes

The pipe operator:

```text
|
```

connects the output of one command to the input of another.

Example:

```bash
ls | grep .c
```

Conceptually:

```text
COMMAND(ls)
      |
     PIPE
      |
COMMAND(grep .c)
```

---

# 9.1 Multiple Pipes

Input:

```bash
cat file | grep hello | wc -l
```

Structure:

```text
        PIPE
       /    \
    PIPE     COMMAND
   /    \      |
COMMAND COMMAND wc -l
  |       |
 cat     grep
 file    hello
```

Alternatively, the same pipeline can be represented as a linked list:

```text
COMMAND → COMMAND → COMMAND
   cat      grep       wc
```

with pipe relationships between them.

The exact representation depends on your project architecture.

---

# 10. AST Structure

A common Parser design is an AST:

> Abstract Syntax Tree

An AST represents the grammatical structure of a command.

For:

```bash
echo hello | grep hello
```

we can build:

```text
          PIPE
         /    \
     COMMAND  COMMAND
      /   \     /   \
    echo hello grep hello
```

The root is:

```text
PIPE
```

because the entire input is a pipeline.

---

# 10.1 Why AST?

AST makes complex commands easier to represent.

For:

```bash
cat < input.txt | grep hello > output.txt
```

we can build:

```text
             PIPE
            /    \
       COMMAND   COMMAND
       /    \      /   \
     cat    <     grep  >
             |          |
        input.txt   output.txt
                   +
                  hello
```

The Executor can then traverse this structure.

---

# 11. Basic Data Structures

There are several possible architectures.

For a simple Minishell, you can use:

```text
t_token
t_redir
t_command
t_ast
```

For example:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
}   t_token;
```

---

# 11.1 Redirection Structure

```c
typedef struct s_redir
{
    int             type;
    char            *target;
    struct s_redir  *next;
}   t_redir;
```

Possible types:

```c
#define REDIR_IN   1
#define REDIR_OUT  2
#define APPEND     3
#define HEREDOC    4
```

---

# 11.2 Command Structure

A simple command structure:

```c
typedef struct s_command
{
    char                **argv;
    t_redir             *redirs;
    struct s_command    *next;
}   t_command;
```

For:

```bash
echo hello | grep hello
```

we could have:

```text
command1
    argv = ["echo", "hello"]
    next → command2

command2
    argv = ["grep", "hello"]
    next = NULL
```

---

# 11.3 AST Structure

If using an AST:

```c
typedef struct s_ast
{
    int             type;
    struct s_ast    *left;
    struct s_ast    *right;
    t_command       *command;
}   t_ast;
```

Possible node types:

```c
#define AST_COMMAND 1
#define AST_PIPE    2
```

Then:

```bash
echo hello | grep hello
```

becomes:

```text
        AST_PIPE
        /      \
 AST_COMMAND  AST_COMMAND
```

---

# 12. Token Structure

The Parser depends on a correct token structure.

A typical token structure:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
}   t_token;
```

For:

```bash
echo hello > file
```

the token list could be:

```text
+-----------+
| WORD      |
| echo      |
+-----------+
      |
      v
+-----------+
| WORD      |
| hello     |
+-----------+
      |
      v
+-----------+
| REDIR_OUT |
| >         |
+-----------+
      |
      v
+-----------+
| WORD      |
| file      |
+-----------+
```

---

# 12.1 Token Types

Example:

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

This makes parser logic easier.

Instead of checking:

```c
if (!ft_strcmp(token->value, "|"))
```

you can write:

```c
if (token->type == TOKEN_PIPE)
```

---

# 13. AST Node Structure

A basic AST node can contain:

```text
type
left
right
command
```

For example:

```c
typedef struct s_ast
{
    t_token         *token;
    t_command       *command;
    struct s_ast    *left;
    struct s_ast    *right;
}   t_ast;
```

However, avoid putting unnecessary fields into every structure.

Choose a structure that matches your execution design.

---

# 13.1 Example AST

Input:

```bash
echo hello | wc -c
```

AST:

```text
             PIPE
            /    \
           /      \
       COMMAND   COMMAND
        /   \      /   \
      echo hello  wc   -c
```

---

# 14. Parser Context

It can be useful to have a parser context:

```c
typedef struct s_parser
{
    t_token *current;
    t_ast   *root;
}   t_parser;
```

This allows parser functions to keep track of their position.

Example:

```c
t_parser parser;

parser.current = tokens;
parser.root = NULL;
```

---

# 14.1 Why Keep `current`?

Instead of passing the token repeatedly:

```c
parse_command(tokens, ...)
parse_redirection(tokens, ...)
parse_pipeline(tokens, ...)
```

you can maintain:

```text
parser.current
```

and advance it as tokens are consumed.

For example:

```c
parser.current = parser.current->next;
```

This makes recursive parsing easier.

---

# 15. Parser Functions

A clean parser can be divided into several functions.

For example:

```c
t_ast       *parse_input(t_token *tokens);
t_ast       *parse_pipeline(t_parser *parser);
t_ast       *parse_command(t_parser *parser);
t_redir     *parse_redirection(t_parser *parser);
int         is_redirection(int type);
int         is_command_end(int type);
```

---

# 15.1 Function Responsibilities

### `parse_input()`

Starts parsing.

```text
input
 ↓
pipeline
```

---

### `parse_pipeline()`

Handles:

```text
command | command | command
```

---

### `parse_command()`

Handles:

```text
command arguments redirections
```

---

### `parse_redirection()`

Handles:

```text
< file
> file
>> file
<< EOF
```

---

# 16. Parsing a Simple Command

Input:

```bash
echo hello
```

Tokens:

```text
WORD("echo")
WORD("hello")
```

Parser starts at:

```text
WORD("echo")
```

It creates a command.

Then:

```text
WORD("hello")
```

is added to `argv`.

Final structure:

```text
COMMAND
├── argv[0] = "echo"
└── argv[1] = "hello"
```

---

# 16.1 Basic Pseudocode

```text
parse_command():

    create command

    while current token is WORD:

        add token value to argv

        move to next token

    return command
```

But this needs to be extended to support redirections.

---

# 17. Parsing Redirections

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

Parser:

```text
WORD
  ↓
argv

WORD
  ↓
argv

REDIR_OUT
  ↓
expect WORD
  ↓
create redirection
```

Result:

```text
COMMAND
├── argv
│   ├── echo
│   └── hello
│
└── redirs
    └── OUTPUT → file
```

---

# 17.1 Pseudocode

```text
parse_command():

    create command

    while current token is not PIPE and not EOF:

        if current token is WORD:
            add to argv

        else if current token is REDIRECTION:
            parse redirection

        else:
            syntax error

    return command
```

---

# 17.2 Parsing a Redirection

```text
parse_redirection():

    save current operator

    move to next token

    if current token is not WORD:
        syntax error

    create redirection

    redirection.type = operator.type
    redirection.target = current.value

    move to next token
```

For:

```bash
echo > output.txt
```

the parser sees:

```text
REDIR_OUT
    ↓
WORD("output.txt")
```

and creates:

```text
type = REDIR_OUT
target = "output.txt"
```

---

# 18. Parsing Pipes

Input:

```bash
echo hello | grep hello
```

Tokens:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

Parser first creates:

```text
COMMAND
├── echo
└── hello
```

Then encounters:

```text
PIPE
```

The parser knows:

> The current command ends here, and another command must follow.

Then it parses:

```text
COMMAND
├── grep
└── hello
```

Final AST:

```text
          PIPE
         /    \
    COMMAND  COMMAND
     /   \    /   \
   echo hello grep hello
```

---

# 18.1 Pipe Rules

A valid pipe must have:

```text
command | command
```

Invalid:

```bash
| echo
```

Invalid:

```bash
echo |
```

Invalid:

```bash
echo | | cat
```

The Parser should detect these syntax errors.

---

# 18.2 Pseudocode

```text
parse_pipeline():

    left = parse_command()

    while current token is PIPE:

        consume PIPE

        if next token cannot start a command:
            syntax error

        right = parse_command()

        create PIPE node

        pipe.left = left
        pipe.right = right

        left = pipe

    return left
```

This produces a left-associative tree.

For:

```bash
a | b | c
```

the structure becomes:

```text
        PIPE
       /    \
     PIPE    c
    /   \
   a     b
```

---

# 19. Example AST

Consider:

```bash
cat input.txt | grep hello > output.txt
```

Tokens:

```text
WORD("cat")
WORD("input.txt")
PIPE
WORD("grep")
WORD("hello")
REDIR_OUT
WORD("output.txt")
```

AST:

```text
                 PIPE
                /    \
               /      \
          COMMAND    COMMAND
          /    \       /    \
        cat  input.txt grep  hello
                         |
                       REDIR
                         |
                     output.txt
```

---

# 19.1 Command Representation

Left command:

```text
argv:
    [ "cat", "input.txt" ]

redirs:
    none
```

Right command:

```text
argv:
    [ "grep", "hello" ]

redirs:
    OUTPUT → "output.txt"
```

---

# 19.2 Another Example

Input:

```bash
< input.txt cat | grep hello >> result.txt
```

First command:

```text
COMMAND
├── argv
│   └── cat
│
└── REDIR_IN
    └── input.txt
```

Second command:

```text
COMMAND
├── argv
│   ├── grep
│   └── hello
│
└── APPEND
    └── result.txt
```

Connected by:

```text
PIPE
```

---

# 20. Error Handling

The Parser must detect invalid command structures.

Examples:

```bash
|
```

```bash
| ls
```

```bash
ls |
```

```bash
ls | | wc
```

These are syntax errors.

---

# 20.1 Invalid Redirection

Invalid:

```bash
echo >
```

There is no filename after `>`.

Invalid:

```bash
cat <
```

Invalid:

```bash
cat >> 
```

Invalid:

```bash
cat <<
```

The Parser expects a WORD after the redirection operator.

---

# 20.2 Example

Input:

```bash
echo > | cat
```

Tokens:

```text
WORD("echo")
REDIR_OUT
PIPE
WORD("cat")
```

After `>` the Parser expects:

```text
WORD
```

but finds:

```text
PIPE
```

Therefore:

```text
SYNTAX ERROR
```

---

# 20.3 Parser Error Function

A helper function can be useful:

```c
void parser_error(const char *message)
{
    ft_putstr_fd("minishell: syntax error: ", 2);
    ft_putendl_fd((char *)message, 2);
}
```

The exact error message should match your project requirements.

---

# 21. Common Mistakes

## Mistake 1 — Making the Parser Execute Commands

The Parser should not call:

```c
execve()
fork()
wait()
```

Its job is to build the command structure.

---

# 21.1 Mistake 2 — Mixing Lexer and Parser Logic

Do not make the Parser responsible for discovering characters.

The Lexer should already have converted:

```text
|
>
>>
<
<<
```

into token types.

The Parser works with tokens.

---

# 21.2 Mistake 3 — Ignoring Redirection Targets

Wrong:

```bash
echo hello > file
```

Parser creates:

```text
COMMAND
├── echo
└── hello
```

and ignores:

```text
> file
```

Correct:

```text
COMMAND
├── argv
│   ├── echo
│   └── hello
│
└── REDIR_OUT
    └── file
```

---

# 21.3 Mistake 4 — Treating `|` as an Argument

Wrong:

```bash
echo hello | grep hello
```

as:

```text
argv:
    echo
    hello
    |
    grep
    hello
```

Correct:

```text
PIPE
├── COMMAND
│   ├── echo
│   └── hello
│
└── COMMAND
    ├── grep
    └── hello
```

---

# 21.4 Mistake 5 — Not Validating Pipe Positions

The Parser should reject:

```bash
| ls
```

```bash
ls |
```

```bash
ls | | wc
```

---

# 21.5 Mistake 6 — Not Validating Redirection Targets

Reject:

```bash
echo >
```

and:

```bash
echo < |
```

because a redirection requires a target WORD.

---

# 21.6 Mistake 7 — Losing Command Order

For:

```bash
cat | grep | wc
```

the order must remain:

```text
cat
 ↓
grep
 ↓
wc
```

The Executor depends on this order.

---

# 22. Testing

## Test 1 — Simple command

```bash
echo hello
```

Expected:

```text
COMMAND
├── echo
└── hello
```

---

## Test 2 — Multiple arguments

```bash
ls -la /tmp
```

Expected:

```text
COMMAND
├── ls
├── -la
└── /tmp
```

---

## Test 3 — Output redirection

```bash
echo hello > file
```

Expected:

```text
COMMAND
├── echo
├── hello
└── REDIR_OUT → file
```

---

## Test 4 — Input redirection

```bash
cat < input.txt
```

Expected:

```text
COMMAND
├── cat
└── REDIR_IN → input.txt
```

---

## Test 5 — Append

```bash
echo hello >> file
```

Expected:

```text
COMMAND
├── echo
├── hello
└── APPEND → file
```

---

## Test 6 — Heredoc

```bash
cat << EOF
```

Expected:

```text
COMMAND
├── cat
└── HEREDOC → EOF
```

---

## Test 7 — Pipe

```bash
echo hello | grep hello
```

Expected:

```text
       PIPE
      /    \
   echo    grep
    |       |
  hello    hello
```

---

## Test 8 — Multiple pipes

```bash
cat file | grep hello | wc -l
```

Expected:

```text
PIPE
├── PIPE
│   ├── cat file
│   └── grep hello
│
└── wc -l
```

---

## Test 9 — Multiple redirections

```bash
cat < input.txt > output.txt
```

Expected:

```text
COMMAND
├── cat
├── REDIR_IN → input.txt
└── REDIR_OUT → output.txt
```

---

## Test 10 — Pipe + redirection

```bash
cat file | grep hello > result.txt
```

Expected:

```text
PIPE
├── COMMAND
│   ├── cat
│   └── file
│
└── COMMAND
    ├── grep
    ├── hello
    └── REDIR_OUT → result.txt
```

---

## Test 11 — Invalid pipe

```bash
| echo
```

Expected:

```text
SYNTAX ERROR
```

---

## Test 12 — Pipe at the end

```bash
echo hello |
```

Expected:

```text
SYNTAX ERROR
```

---

## Test 13 — Double pipe

```bash
echo hello | | cat
```

Expected:

```text
SYNTAX ERROR
```

---

## Test 14 — Missing redirection target

```bash
echo >
```

Expected:

```text
SYNTAX ERROR
```

---

## Test 15 — Redirection followed by pipe

```bash
echo > | cat
```

Expected:

```text
SYNTAX ERROR
```

---

# 22.1 Parser Test Table

| Input             | Expected                     |
| ----------------- | ---------------------------- |
| `echo hello`      | One command                  |
| `ls -la`          | Command + argument           |
| `cat < file`      | Command + input redirection  |
| `echo hi > file`  | Command + output redirection |
| `echo hi >> file` | Command + append             |
| `cat << EOF`      | Command + heredoc            |
| `ls \| grep .c`   | Two commands + pipe          |
| `a \| b \| c`     | Three commands + two pipes   |
| `cat < in > out`  | Command + two redirections   |
| `\| ls`           | Syntax error                 |
| `ls \|`           | Syntax error                 |
| `ls \| \| wc`     | Syntax error                 |
| `echo >`          | Syntax error                 |
| `echo > \| cat`   | Syntax error                 |

---

# 23. Final Checklist

## Parser Structures

* [ ] `t_token` exists.
* [ ] Token types are defined.
* [ ] `t_command` exists.
* [ ] Redirection structure exists.
* [ ] AST structure exists if using AST.
* [ ] Parser context exists if needed.

## Commands

* [ ] Simple commands can be parsed.
* [ ] Arguments are stored.
* [ ] Command order is preserved.

## Redirections

* [ ] `<` is recognized.
* [ ] `>` is recognized.
* [ ] `>>` is recognized.
* [ ] `<<` is recognized.
* [ ] Redirection targets are stored.
* [ ] Missing targets cause syntax errors.

## Pipes

* [ ] `|` creates a pipe relationship.
* [ ] Commands before and after the pipe are parsed.
* [ ] Multiple pipes work.
* [ ] Pipe at the beginning is rejected.
* [ ] Pipe at the end is rejected.
* [ ] Consecutive pipes are rejected.

## Memory

* [ ] AST nodes are allocated safely.
* [ ] Commands are freed correctly.
* [ ] Redirections are freed correctly.
* [ ] Token references are handled safely.
* [ ] No memory leaks.
* [ ] No invalid reads.
* [ ] No double frees.

---

# 24. Overall Architecture

At this point, the Minishell architecture should look like:

```text
                     USER INPUT
                         |
                         v
                       LEXER
                         |
                         v
                    TOKEN LIST
                         |
                         v
                      PARSER
                         |
              +----------+----------+
              |                     |
              v                     v
          COMMANDS                 AST
              |                     |
              +----------+----------+
                         |
                         v
                  PARAMETER EXPANSION
                         |
                         v
                    QUOTE REMOVAL
                         |
                         v
                     EXECUTOR
                         |
              +----------+----------+
              |                     |
              v                     v
            PIPE                 REDIRECTION
              |                     |
              +----------+----------+
                         |
                         v
                      execve()
```

---

# Key Concept

The Parser does **not** care about individual characters anymore.

The Lexer has already converted the input:

```bash
cat file | grep hello > result.txt
```

into tokens:

```text
WORD("cat")
WORD("file")
PIPE
WORD("grep")
WORD("hello")
REDIR_OUT
WORD("result.txt")
```

The Parser's job is to understand the relationship between them:

```text
                  PIPE
                 /    \
                /      \
           COMMAND    COMMAND
           /    \      /    \
         cat   file   grep  hello
                           |
                        REDIR_OUT
                            |
                       result.txt
```

The most important principle is:

> **Lexer identifies tokens. Parser identifies structure. Executor performs the command.**

Keeping these responsibilities separated will make the rest of Minishell significantly easier to implement, debug, and test.
