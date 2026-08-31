# Minishell — Developer 1 Plan

## Role

I am responsible for the front-end processing of the shell input.

My main pipeline is:

```text
USER INPUT
    ↓
LEXER
    ↓
TOKEN LIST
    ↓
SYNTAX VALIDATION
    ↓
PARSER
    ↓
COMMAND STRUCTURES
    ↓
EXPANSION
    ↓
READY FOR EXECUTOR
```

My goal is to transform:

```bash
echo "Hello $USER" | grep Hello > output.txt
```

into a structured representation that the Executor can execute.

---

# 🎯 Goal Before Friday

By Friday, I should:

* [ ] Understand the shell processing pipeline.
* [ ] Understand lexical analysis.
* [ ] Understand shell quoting rules.
* [ ] Understand token types.
* [ ] Understand syntax validation.
* [ ] Understand parameter expansion.
* [ ] Understand `$?`.
* [ ] Create the token structure.
* [ ] Create a basic Lexer.
* [ ] Handle spaces correctly.
* [ ] Recognize shell operators.
* [ ] Preserve quoted strings.
* [ ] Create basic Parser structures.
* [ ] Document the interface for Developer 2.

---

# 📚 Sources

## 1. Official Bash Reference Manual

Read the Bash manual sections about:

* Shell operation
* Quoting
* Shell expansions
* Parameter expansion
* Redirections
* Pipelines

### Main source

[GNU Bash Reference Manual](https://www.gnu.org/s/bash/manual/bash.html?utm_source=chatgpt.com)

---

## 2. Shell Processing Order

Read:

[Bash Shell Operation](https://www.gnu.org/software/bash/manual/html_node/Shell-Operation.html?utm_source=chatgpt.com)

The important pipeline is:

```text
1. Read input
        ↓
2. Split input into words and operators
        ↓
3. Parse tokens
        ↓
4. Perform expansions
        ↓
5. Perform redirections
        ↓
6. Execute commands
```

For Minishell, my responsibility mainly covers:

```text
INPUT
  ↓
TOKENIZATION
  ↓
PARSING
  ↓
EXPANSION
```

---

## 3. Quoting

Read:

[GNU Bash Quoting Documentation](https://www.gnu.org/s/bash/manual/html_node/Quoting.html?utm_source=chatgpt.com)

I must understand:

```bash
echo hello
echo "hello"
echo 'hello'
echo "$USER"
echo '$USER'
```

Important rule:

```text
Single quotes:
    literal content

Double quotes:
    preserve spaces
    allow parameter expansion
```

---

## 4. Shell Expansions

Read:

[GNU Bash Shell Expansions](https://www.gnu.org/software/bash/manual/html_node/Shell-Expansions.html?utm_source=chatgpt.com)

Focus on:

```text
Parameter expansion
Quote removal
Expansion order
```

For Minishell, I do not need to implement every Bash expansion.

I must focus on the expansions required by the project.

---

## 5. Parameter Expansion

Read:

[GNU Bash Parameter Expansion](https://www.gnu.org/s/bash/manual/html_node/Shell-Parameter-Expansion.html?utm_source=chatgpt.com)

Focus on:

```bash
$USER
$HOME
$PATH
$?
```

Understand:

```bash
echo $USER
echo "$USER"
echo '$USER'
```

---

## 6. Redirections

Read:

[GNU Bash Redirections Documentation](https://www.gnu.org/s/bash/manual/html_node/Redirections.html?utm_source=chatgpt.com)

I am not responsible for executing redirections, but my Lexer and Parser must recognize:

```text
<
>
>>
<<
```

Example:

```bash
cat < input.txt > output.txt
```

My Parser must provide enough information for Developer 2 to execute it.

---

## 7. Pipelines

Read:

[GNU Bash Pipelines Documentation](https://www.gnu.org/software/bash/manual/html_node/Pipelines.html?utm_source=chatgpt.com)

I need to understand that:

```bash
ls | grep .c | wc -l
```

represents multiple commands.

My Parser should transform it conceptually into:

```text
COMMAND 1
    ls

COMMAND 2
    grep .c

COMMAND 3
    wc -l
```

---

# 📅 DAY 1 — Understanding and Design

## Task 1 — Understand the Complete Pipeline

Study and be able to explain:

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
EXECUTOR
```

### Question

What is the difference between a Lexer and a Parser?

### Expected answer

The Lexer transforms characters into tokens.

Example:

```bash
echo hello | cat
```

becomes:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("cat")
```

The Parser transforms tokens into commands.

Example:

```text
WORD("echo")
WORD("hello")
PIPE
WORD("cat")
```

becomes:

```text
COMMAND
    argv = ["echo", "hello"]

PIPE

COMMAND
    argv = ["cat"]
```

---

# Task 2 — Study Token Types

Create a document:

```text
docs/tokens.md
```

Define every token.

Suggested tokens:

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

Document:

| Token             | Input                       |   |
| ----------------- | --------------------------- | - |
| `TOKEN_WORD`      | `echo`, `hello`, `file.txt` |   |
| `TOKEN_PIPE`      | `                           | ` |
| `TOKEN_REDIR_IN`  | `<`                         |   |
| `TOKEN_REDIR_OUT` | `>`                         |   |
| `TOKEN_APPEND`    | `>>`                        |   |
| `TOKEN_HEREDOC`   | `<<`                        |   |

---

# Task 3 — Design `t_token`

Create:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
} t_token;
```

Understand ownership.

For example:

```text
Input:
echo hello | cat
```

Creates:

```text
┌─────────────┐
│ WORD        │
│ echo        │
└──────┬──────┘
       ↓
┌─────────────┐
│ WORD        │
│ hello       │
└──────┬──────┘
       ↓
┌─────────────┐
│ PIPE        │
│ |           │
└──────┬──────┘
       ↓
┌─────────────┐
│ WORD        │
│ cat         │
└──────┬──────┘
       ↓
      NULL
```

---

# Task 4 — Study Quotes

Test in Bash:

```bash
echo "hello world"
echo 'hello world'

echo "$USER"
echo '$USER'

echo ""
echo ''

echo a"b"c
echo a'b'c

echo "hello"'world'
```

Write the results in:

```text
docs/quote_tests.md
```

Questions to answer:

* [ ] Are quote characters included in the final argument?
* [ ] What happens to spaces inside quotes?
* [ ] Does `$USER` expand inside single quotes?
* [ ] Does `$USER` expand inside double quotes?
* [ ] Can quoted and unquoted text belong to one word?

Example:

```bash
echo hello"world"
```

Expected concept:

```text
WORD("helloworld")
```

---

# Task 5 — Identify Lexer States

My Lexer should not simply split by spaces.

I need to track states.

Suggested states:

```c
typedef enum e_quote_state
{
    NO_QUOTE,
    SINGLE_QUOTE,
    DOUBLE_QUOTE
} t_quote_state;
```

Concept:

```text
Normal state
    |
    | '
    v
Single quote state
    |
    | '
    v
Normal state

Normal state
    |
    | "
    v
Double quote state
    |
    | "
    v
Normal state
```

---

# End of Day 1 — Expected Result

By the end of Day 1 I should have:

* [ ] Read the Bash sources.
* [ ] Tested quotes.
* [ ] Defined token types.
* [ ] Designed `t_token`.
* [ ] Designed quote states.
* [ ] Created `docs/tokens.md`.
* [ ] Created `docs/quote_tests.md`.
* [ ] Explained the difference between Lexer and Parser.
* [ ] Explained the interface I will provide to Developer 2.

---

# 📅 DAY 2 — Implementation

## Task 1 — Create Lexer Files

Create:

```text
src/lexer/
├── lexer.c
├── lexer_utils.c
├── token.c
└── token_utils.c
```

Possible responsibilities:

```text
lexer.c
    main tokenization loop

lexer_utils.c
    character checks
    quote handling

token.c
    create/add tokens

token_utils.c
    token helper functions
```

---

# Task 2 — Implement Token Creation

Implement:

```c
t_token *token_new(char *value, t_token_type type);
void    token_add_back(t_token **list, t_token *new);
void    token_clear(t_token **list);
```

Test:

```text
Create token
Add token
Print token list
Free token list
```

---

# Task 3 — Implement Operator Recognition

Recognize:

```text
|
<
>
>>
<<
```

Important:

```text
>>
```

must be recognized as one token.

Not:

```text
>
>
```

The same applies to:

```text
<<
```

Test:

```bash
echo hello|cat
echo hello >file
echo hello>>file
cat<<EOF
```

Spaces must not be required around operators.

---

# Task 4 — Implement Basic Word Tokenization

Test:

```bash
echo hello
```

Expected:

```text
WORD: echo
WORD: hello
```

Test:

```bash
ls -la /tmp
```

Expected:

```text
WORD: ls
WORD: -la
WORD: /tmp
```

---

# Task 5 — Implement Quote-Aware Tokenization

Test:

```bash
echo "hello world"
```

Expected:

```text
WORD: echo
WORD: "hello world"
```

At this stage, you can decide whether to:

### Option A — Keep quotes

Lexer output:

```text
"hello world"
```

Expansion later removes them.

### Option B — Store quote metadata

Example:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    int             quote_type;
    struct s_token  *next;
} t_token;
```

Discuss the architecture with your teammate before choosing.

For Minishell, I recommend preserving enough quote information for the Expansion stage.

---

# Task 6 — Test Mixed Words

These tests are extremely important:

```bash
echo hello"world"
echo hello'world'

echo "$USER"
echo '$USER'

echo a"b"c
echo a'b'c

echo ""hello
echo hello""
```

The Lexer must understand that:

```bash
hello"world"
```

is one shell word.

Conceptually:

```text
WORD: hello"world"
```

not:

```text
WORD: hello
WORD: "world"
```

---

# Task 7 — Basic Syntax Validation

Start validating obvious errors.

Examples:

```bash
|
```

```bash
echo hello |
```

```bash
| echo hello
```

```bash
echo >
```

```bash
echo >>
```

```bash
cat <
```

The Lexer may create tokens successfully.

The syntax validator should detect invalid sequences.

Example:

```text
PIPE cannot be:
    first
    last
    followed by another PIPE
```

And:

```text
REDIRECTION must be followed by WORD
```

---

# Task 8 — Start Parser Design

Create:

```text
src/parser/
├── parser.c
├── parser_utils.c
└── command.c
```

Suggested structure:

```c
typedef struct s_cmd
{
    char            **argv;
    t_redir         *redirections;
    struct s_cmd    *next;
} t_cmd;
```

Suggested redirection structure:

```c
typedef struct s_redir
{
    t_token_type    type;
    char            *file;
    struct s_redir  *next;
} t_redir;
```

Example:

```bash
cat < input.txt | grep hello > output.txt
```

Could become:

```text
COMMAND 1

argv:
    cat

redirection:
    < input.txt


COMMAND 2

argv:
    grep
    hello

redirection:
    > output.txt
```

The pipe itself can be represented by linking commands:

```text
CMD1
  |
 next
  ↓
CMD2
```

Developer 2 can then execute the command list.

---

# Task 9 — Define the Interface for Developer 2

Before Friday, agree on exactly what I will provide.

Recommended interface:

```c
t_cmd *parse_input(char *input, t_shell *shell);
```

Developer 2 receives:

```c
t_cmd *commands;
```

and should not need to know how the Lexer works internally.

My contract:

```text
INPUT
  ↓
LEXER
  ↓
VALID TOKENS
  ↓
PARSER
  ↓
VALID COMMAND LIST
```

Developer 2's contract:

```text
COMMAND LIST
  ↓
REDIRECTIONS
  ↓
PROCESSES
  ↓
EXECUTION
```

---

# 🌱 Expansion Plan

Expansion should happen after the input has been tokenized.

Reference:

[GNU Bash Simple Command Expansion Order](https://www.gnu.org/s/bash/manual/html_node/Simple-Command-Expansion.html?utm_source=chatgpt.com)

Start with:

```bash
$USER
$HOME
$PATH
$?
```

---

## Rule 1 — Single Quotes

```bash
echo '$USER'
```

Expected:

```text
$USER
```

No expansion.

---

## Rule 2 — Double Quotes

```bash
echo "$USER"
```

Expected:

```text
value of USER
```

Expansion happens.

---

## Rule 3 — Unquoted Variables

```bash
echo $USER
```

Expected:

```text
value of USER
```

---

## Rule 4 — Exit Status

```bash
echo $?
```

Expand `$?` using:

```c
shell->exit_status
```

---

# 🧪 My Test Checklist

## Basic words

```bash
echo hello
ls -la
cat file
```

* [ ] Passed

## Spaces

```bash
echo    hello
echo        hello
```

* [ ] Passed

## Pipes

```bash
echo hello|cat
echo hello | cat
echo hello |cat
```

* [ ] Passed

## Redirections

```bash
echo hello>file
echo hello>>file
cat<file
cat<<EOF
```

* [ ] Passed

## Quotes

```bash
echo "hello world"
echo 'hello world'
echo ""
echo ''
```

* [ ] Passed

## Mixed quotes

```bash
echo hello"world"
echo hello'world'
echo a"b"c
echo a'b'c
```

* [ ] Passed

## Variables

```bash
echo $USER
echo "$USER"
echo '$USER'
echo $?
```

* [ ] Passed

## Syntax errors

```bash
|
echo |
| echo
echo >
echo >>
cat <
```

* [ ] Passed

---

# 🤝 Friday Meeting Checklist

I must bring the following to the meeting:

* [ ] Token types.
* [ ] `t_token` structure.
* [ ] Quote state design.
* [ ] Basic Lexer implementation.
* [ ] Token debug printer.
* [ ] Quote test results.
* [ ] Syntax validation rules.
* [ ] Proposed `t_cmd` structure.
* [ ] Proposed `t_redir` structure.
* [ ] Lexer → Parser interface.
* [ ] Parser → Executor interface.
* [ ] Memory ownership rules.
* [ ] List of open questions.

---

# 📦 Deliverables

By Friday, my branch should contain:

```text
src/
├── lexer/
│   ├── lexer.c
│   ├── lexer_utils.c
│   ├── token.c
│   └── token_utils.c
│
└── parser/
    ├── parser.c
    ├── parser_utils.c
    └── command.c

docs/
├── tokens.md
├── quote_tests.md
└── lexer_design.md
```

---

# ⭐ Priority Order

If I run out of time, I should work in this order:

```text
1. Understand requirements
        ↓
2. Understand quoting
        ↓
3. Design token structure
        ↓
4. Implement basic Lexer
        ↓
5. Recognize operators
        ↓
6. Handle quotes
        ↓
7. Test tokens
        ↓
8. Syntax validation
        ↓
9. Parser design
        ↓
10. Expansion
```

Do not rush into implementing Expansion before the Lexer and token structure are stable.

---

# Final Goal

My responsibility is not simply to "split a string".

My responsibility is to transform:

```bash
echo "Hello $USER" | grep Hello > output.txt
```

into a valid command representation:

```text
INPUT
  ↓
TOKENS
  ↓
COMMANDS
  ↓
ARGUMENTS
  ↓
REDIRECTIONS
  ↓
READY FOR EXECUTION
```

The Executor should receive clean, validated command structures and should not need to understand the original raw input string.

> **A good Lexer and Parser make the Executor simple. A bad Lexer and Parser make the entire Minishell project difficult.**
