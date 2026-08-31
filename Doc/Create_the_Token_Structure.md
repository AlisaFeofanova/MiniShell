# Create the Token Structure

## Table of Contents

* [1. What is a Token?](#1-what-is-a-token)
* [2. Why Do We Need a Token Structure?](#2-why-do-we-need-a-token-structure)
* [3. What Data Should a Token Contain?](#3-what-data-should-a-token-contain)
* [4. Token Types](#4-token-types)
* [5. Understanding Each Token Type](#5-understanding-each-token-type)
* [6. The Complete Token Structure](#6-the-complete-token-structure)
* [7. Why Do We Need `next`?](#7-why-do-we-need-next)
* [8. Creating a Token](#8-creating-a-token)
* [9. Adding Tokens to the List](#9-adding-tokens-to-the-list)
* [10. Lexer Creates Tokens](#10-lexer-creates-tokens)
* [11. Quotes and Tokens](#11-quotes-and-tokens)
* [12. Operators](#12-operators)
* [13. Token Structure vs Command Structure](#13-token-structure-vs-command-structure)
* [14. Syntax Validation with Tokens](#14-syntax-validation-with-tokens)
* [15. Token Cleanup](#15-token-cleanup)
* [16. Memory Ownership](#16-memory-ownership)
* [17. Recommended Header](#17-recommended-header)
* [18. Debugging Tokens](#18-debugging-tokens)
* [19. Implementation Order](#19-implementation-order)
* [20. Testing](#20-testing)
* [21. Final Architecture](#21-final-architecture)
* [22. Final Checklist](#22-final-checklist)

---

# 1. What is a Token?

A **token** is one meaningful part of the user's shell input.

For example:

```bash
echo "hello world" > output.txt
```

The Lexer should split this input into meaningful elements:

```text
echo
"hello world"
>
output.txt
```

Each meaningful element becomes a **token**.

Conceptually:

```text
USER INPUT
    |
    v
  LEXER
    |
    v
TOKENS
```

---

# 2. Why Do We Need a Token Structure?

Minishell processes input through several stages:

```text
User Input
    |
    v
  Lexer
    |
    v
 Tokens
    |
    v
 Parser
    |
    v
Command Structure
    |
    v
 Executor
```

The Token Structure is the bridge between the **Lexer** and the **Parser**.

The Lexer answers:

> "What elements are present in the input?"

The Parser answers:

> "What does this sequence of elements mean?"

For example:

```bash
echo hello | wc
```

The Lexer identifies:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

The Parser can then understand that there are two commands connected by a pipe.

---

# 3. What Data Should a Token Contain?

A basic token should contain three things:

1. The token's text.
2. The token's type.
3. A pointer to the next token.

For example:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

Where:

```text
value
    ↓
actual token text

type
    ↓
what kind of token it is

next
    ↓
next token in the list
```

---

# 4. Token Types

We need to define the possible token types.

A simple implementation can use:

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

This gives us:

```text
TOKEN_WORD
TOKEN_PIPE
TOKEN_REDIR_IN
TOKEN_REDIR_OUT
TOKEN_APPEND
TOKEN_HEREDOC
```

---

# 5. Understanding Each Token Type

## `TOKEN_WORD`

A normal Shell word.

Examples:

```bash
echo
hello
file.txt
$USER
"hello world"
```

For:

```bash
echo hello
```

we get:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
```

---

# 5.1 `TOKEN_PIPE`

The pipe operator:

```bash
|
```

Example:

```bash
ls | wc
```

Tokens:

```text
TOKEN_WORD("ls")
TOKEN_PIPE("|")
TOKEN_WORD("wc")
```

The Parser later uses the pipe to connect two commands.

---

# 5.2 `TOKEN_REDIR_IN`

The input redirection operator:

```bash
<
```

Example:

```bash
cat < input.txt
```

Tokens:

```text
TOKEN_WORD("cat")
TOKEN_REDIR_IN("<")
TOKEN_WORD("input.txt")
```

---

# 5.3 `TOKEN_REDIR_OUT`

The output redirection operator:

```bash
>
```

Example:

```bash
echo hello > output.txt
```

Tokens:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
TOKEN_REDIR_OUT(">")
TOKEN_WORD("output.txt")
```

---

# 5.4 `TOKEN_APPEND`

The append redirection operator:

```bash
>>
```

Example:

```bash
echo hello >> output.txt
```

Tokens:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
TOKEN_APPEND(">>")
TOKEN_WORD("output.txt")
```

---

# 5.5 `TOKEN_HEREDOC`

The heredoc operator:

```bash
<<
```

Example:

```bash
cat << EOF
hello
EOF
```

Tokens:

```text
TOKEN_WORD("cat")
TOKEN_HEREDOC("<<")
TOKEN_WORD("EOF")
```

The Executor will later use the heredoc token to implement heredoc input.

---

# 6. The Complete Token Structure

A good basic implementation is:

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

typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

This is enough for the basic Lexer → Parser communication.

---

# 7. Why Do We Need `next`?

A linked list is a convenient way to store tokens.

For example:

```bash
echo hello | wc -l
```

The Lexer creates:

```text
+-------------+
| echo        |
| WORD        |
+------+------+
       |
       v
+-------------+
| hello       |
| WORD        |
+------+------+
       |
       v
+-------------+
| |           |
| PIPE        |
+------+------+
       |
       v
+-------------+
| wc          |
| WORD        |
+------+------+
       |
       v
+-------------+
| -l          |
| WORD        |
+------+------+
       |
       v
      NULL
```

In memory:

```text
token1 -> token2 -> token3 -> token4 -> token5 -> NULL
```

This makes it easy for the Parser to iterate through the tokens.

---

# 8. Creating a Token

We can create a constructor function:

```c
t_token *token_new(char *value, t_token_type type)
{
    t_token *token;

    token = malloc(sizeof(t_token));
    if (!token)
        return (NULL);

    token->value = value;
    token->type = type;
    token->next = NULL;

    return (token);
}
```

Example:

```c
t_token *token;

token = token_new(ft_strdup("echo"), TOKEN_WORD);
```

The resulting structure looks like:

```text
token
 |
 +-- value = "echo"
 +-- type = TOKEN_WORD
 +-- next = NULL
```

---

# 9. Adding Tokens to the List

We need a function to append a token:

```c
void token_add_back(t_token **tokens, t_token *new_token)
{
    t_token *current;

    if (!*tokens)
    {
        *tokens = new_token;
        return;
    }

    current = *tokens;
    while (current->next)
        current = current->next;

    current->next = new_token;
}
```

Then we can build a token list:

```c
token_add_back(&tokens,
    token_new(ft_strdup("echo"), TOKEN_WORD));

token_add_back(&tokens,
    token_new(ft_strdup("hello"), TOKEN_WORD));

token_add_back(&tokens,
    token_new(ft_strdup("|"), TOKEN_PIPE));

token_add_back(&tokens,
    token_new(ft_strdup("wc"), TOKEN_WORD));
```

Result:

```text
echo -> hello -> | -> wc -> NULL
```

---

# 10. Lexer Creates Tokens

The Lexer receives:

```text
echo hello | wc
```

and creates:

```text
TOKEN_WORD("echo")
TOKEN_WORD("hello")
TOKEN_PIPE("|")
TOKEN_WORD("wc")
```

The general Lexer process is:

```text
input
  |
  v
read characters
  |
  v
identify token
  |
  v
create token
  |
  v
add token to list
```

---

## Simplified Lexer Example

Conceptually:

```c
t_token *lexer(char *input)
{
    t_token *tokens;
    int     i;

    tokens = NULL;
    i = 0;

    while (input[i])
    {
        if (input[i] == '|')
        {
            add_token(&tokens, "|", TOKEN_PIPE);
            i++;
        }
        else if (input[i] == '<')
        {
            if (input[i + 1] == '<')
            {
                add_token(&tokens, "<<", TOKEN_HEREDOC);
                i += 2;
            }
            else
            {
                add_token(&tokens, "<", TOKEN_REDIR_IN);
                i++;
            }
        }
        else if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                add_token(&tokens, ">>", TOKEN_APPEND);
                i += 2;
            }
            else
            {
                add_token(&tokens, ">", TOKEN_REDIR_OUT);
                i++;
            }
        }
        else
        {
            /*
             * Parse a WORD.
             */
        }
    }

    return (tokens);
}
```

This is only a conceptual example.

A complete Lexer must also handle:

```text
spaces
quotes
single quotes
double quotes
operators
empty words
environment variables
$?
syntax
```

---

# 11. Quotes and Tokens

This is extremely important.

Consider:

```bash
echo "hello world"
```

It should NOT become:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

It should become:

```text
WORD("echo")
WORD("hello world")
```

Why?

Because:

```text
"hello world"
```

represents one Shell word.

The space inside the quotes does not separate the word.

---

# 11.1 Quote States

The Lexer needs to understand different states:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

For:

```bash
echo "hello world"
```

the Lexer conceptually does:

```text
echo
 ↓
NORMAL

"
 ↓
DOUBLE_QUOTE

hello world
 ↓
DOUBLE_QUOTE

"
 ↓
NORMAL
```

Therefore, the space between:

```text
hello world
```

does not split the token.

---

# 12. Operators

Minishell needs to recognize these operators:

```text
|
<
>
<<
>>
```

Each operator must become its own token.

For:

```bash
cat < file
```

we should NOT have:

```text
WORD("cat")
WORD("<")
WORD("file")
```

Instead:

```text
WORD("cat")
REDIR_IN("<")
WORD("file")
```

---

# 12.1 Why Are `>` and `>>` Different?

Because they have different execution behavior.

### `>`

```bash
echo hello > file
```

The file is opened for output and its existing contents are truncated.

Conceptually:

```c
O_WRONLY
O_CREAT
O_TRUNC
```

### `>>`

```bash
echo hello >> file
```

The data is appended to the existing file.

Conceptually:

```c
O_WRONLY
O_CREAT
O_APPEND
```

Therefore the Executor must know whether the token is:

```text
TOKEN_REDIR_OUT
```

or:

```text
TOKEN_APPEND
```

---

# 12.2 Why Is `<<` a Separate Token?

Because:

```bash
cat << EOF
```

starts a heredoc.

The Executor needs to perform special processing:

```text
<<
 ↓
read lines
 ↓
stop at delimiter
 ↓
provide input to command
```

Therefore:

```text
TOKEN_HEREDOC
```

must be a separate type.

---

# 13. Token Structure vs Command Structure

Do not confuse **tokens** with **commands**.

### Tokens

Represent individual syntax elements:

```text
echo
hello
|
wc
-l
```

### Command Structure

Represents the already-parsed command:

```text
command:
    argv = ["echo", "hello"]
```

For:

```bash
echo hello | wc -l
```

the Lexer produces:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
WORD("-l")
```

The Parser can then build:

```text
Command 1:
    argv = ["echo", "hello"]

        |

Command 2:
    argv = ["wc", "-l"]
```

---

# 14. Syntax Validation with Tokens

The Token Structure also makes syntax validation possible.

For example:

```bash
echo hello |
```

Tokens:

```text
WORD("echo")
WORD("hello")
PIPE("|")
```

The Parser sees:

```text
PIPE
 ↓
nothing after it
```

Therefore:

```text
syntax error
```

---

## Another Example

```bash
echo >
```

Tokens:

```text
WORD("echo")
REDIR_OUT(">")
```

After:

```text
>
```

the Parser expects a:

```text
WORD
```

But there is no following token.

Therefore:

```text
syntax error
```

---

# 14.1 Invalid Pipe

Input:

```bash
| echo
```

Tokens:

```text
PIPE("|")
WORD("echo")
```

The Parser can detect:

```text
PIPE is at the beginning
```

and report a syntax error.

---

# 14.2 Invalid Redirection

Input:

```bash
echo > | cat
```

Tokens:

```text
WORD("echo")
REDIR_OUT(">")
PIPE("|")
WORD("cat")
```

After:

```text
REDIR_OUT
```

the Parser expects:

```text
WORD
```

but receives:

```text
PIPE
```

Therefore:

```text
syntax error
```

---

# 15. Token Cleanup

Tokens are dynamically allocated, so they must be freed.

Example:

```c
void free_tokens(t_token *tokens)
{
    t_token *next;

    while (tokens)
    {
        next = tokens->next;
        free(tokens->value);
        free(tokens);
        tokens = next;
    }
}
```

If you allocate:

```text
token
 |
 +-- value
```

you must free both:

```text
free(value)
free(token)
```

---

# 16. Memory Ownership

You should clearly define who owns `value`.

A simple design is:

```text
Token owns value
```

For example:

```c
token = malloc(sizeof(t_token));
token->value = ft_strdup("hello");
```

When the token is destroyed:

```c
free(token->value);
free(token);
```

This makes memory ownership clear.

---

# 17. Recommended Header

You can create:

```text
includes/
    minishell.h
    token.h
```

For example:

```c
#ifndef TOKEN_H
# define TOKEN_H

typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC
}   t_token_type;

typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;

t_token *token_new(char *value, t_token_type type);
void    token_add_back(t_token **tokens, t_token *new_token);
void    free_tokens(t_token *tokens);

#endif
```

---

# 18. Debugging Tokens

It is very useful to have a token debugging function:

```c
void print_tokens(t_token *tokens)
{
    while (tokens)
    {
        printf("TYPE=%d VALUE=[%s]\n",
            tokens->type,
            tokens->value);
        tokens = tokens->next;
    }
}
```

For easier debugging, you can print readable names:

```text
TYPE=WORD
TYPE=PIPE
TYPE=REDIR_IN
TYPE=REDIR_OUT
TYPE=APPEND
TYPE=HEREDOC
```

For example:

```bash
echo hello | wc -l
```

should produce something similar to:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
[WORD] -l
```

This is extremely useful when debugging the Lexer.

---

# 19. Implementation Order

For a two-person Minishell team, implement the Token Structure in this order.

## Step 1 — Define the enum

Create:

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

---

## Step 2 — Define the structure

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

---

## Step 3 — Implement the constructor

Create:

```c
t_token *token_new(char *value, t_token_type type);
```

Check:

```text
malloc
value
type
next
```

---

## Step 4 — Implement append

Create:

```c
void token_add_back(t_token **tokens, t_token *new_token);
```

Test that multiple tokens are correctly connected.

---

## Step 5 — Implement cleanup

Create:

```c
void free_tokens(t_token *tokens);
```

Check:

```text
no memory leaks
no double free
no use-after-free
```

---

## Step 6 — Create a debug function

Create:

```c
void print_tokens(t_token *tokens);
```

This will make Lexer debugging much easier.

---

# 20. Testing

## Test 1 — Simple command

Input:

```bash
echo hello
```

Expected:

```text
WORD: echo
WORD: hello
```

---

## Test 2 — Pipeline

Input:

```bash
echo hello | wc
```

Expected:

```text
WORD: echo
WORD: hello
PIPE: |
WORD: wc
```

---

## Test 3 — Input redirection

Input:

```bash
cat < input.txt
```

Expected:

```text
WORD: cat
REDIR_IN: <
WORD: input.txt
```

---

## Test 4 — Output redirection

Input:

```bash
echo hello > output.txt
```

Expected:

```text
WORD: echo
WORD: hello
REDIR_OUT: >
WORD: output.txt
```

---

## Test 5 — Append

Input:

```bash
echo hello >> output.txt
```

Expected:

```text
WORD: echo
WORD: hello
APPEND: >>
WORD: output.txt
```

---

## Test 6 — Heredoc

Input:

```bash
cat << EOF
```

Expected:

```text
WORD: cat
HEREDOC: <<
WORD: EOF
```

---

## Test 7 — Quoted word

Input:

```bash
echo "hello world"
```

Expected:

```text
WORD: echo
WORD: hello world
```

The quoted text is one word.

---

## Test 8 — Single quotes

Input:

```bash
echo '$USER'
```

The token must preserve:

```text
$USER
```

The exact decision about expansion belongs to the expansion stage, depending on your architecture, but the Lexer must preserve enough information for correct quote handling.

---

# 21. Final Architecture

After implementing the Token Structure, your architecture should look like:

```text
                   USER INPUT
                       |
                       v
              +----------------+
              |      LEXER     |
              +--------+-------+
                       |
                       v
              +----------------+
              |   TOKEN LIST   |
              +----------------+
              | value          |
              | type           |
              | next           |
              +--------+-------+
                       |
                       v
              +----------------+
              |     PARSER     |
              +--------+-------+
                       |
                       v
                COMMAND LIST
                       |
                       v
              +----------------+
              |    EXECUTOR    |
              +----------------+
```

---

# 21.1 Example of the Complete Flow

Input:

```bash
echo hello > file | cat
```

### Lexer output

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("cat")
```

### Parser interpretation

```text
Command 1:
    argv = ["echo", "hello"]

    redirection:
        type = >
        file = "file"

    pipe
        |
        v

Command 2:
    argv = ["cat"]
```

### Executor

The Executor now has enough structured information to:

1. Create the required pipe.
2. Configure the redirection.
3. Execute `echo`.
4. Execute `cat`.
5. Close file descriptors.
6. Wait for the processes.
7. Update the Shell exit status.

---

# 22. Final Checklist

## Token Structure

* [ ] `t_token_type` is defined.
* [ ] `t_token` is defined.
* [ ] `value` stores the token text.
* [ ] `type` stores the token type.
* [ ] `next` points to the next token.

## Token Types

* [ ] `TOKEN_WORD`
* [ ] `TOKEN_PIPE`
* [ ] `TOKEN_REDIR_IN`
* [ ] `TOKEN_REDIR_OUT`
* [ ] `TOKEN_APPEND`
* [ ] `TOKEN_HEREDOC`

## Functions

* [ ] `token_new()`
* [ ] `token_add_back()`
* [ ] `free_tokens()`
* [ ] `print_tokens()`

## Lexer

* [ ] Words become `TOKEN_WORD`.
* [ ] `|` becomes `TOKEN_PIPE`.
* [ ] `<` becomes `TOKEN_REDIR_IN`.
* [ ] `>` becomes `TOKEN_REDIR_OUT`.
* [ ] `>>` becomes `TOKEN_APPEND`.
* [ ] `<<` becomes `TOKEN_HEREDOC`.
* [ ] Quotes are considered when determining word boundaries.
* [ ] Tokens are correctly linked together.
* [ ] The token list is correctly passed to the Parser.

## Memory

* [ ] Every token is freed.
* [ ] Every `value` is freed.
* [ ] No memory leaks.
* [ ] No double free.
* [ ] No use-after-free.

## Parser

* [ ] Parser can distinguish WORDs from operators.
* [ ] Parser can detect invalid pipes.
* [ ] Parser can detect missing redirection targets.
* [ ] Parser can detect invalid token sequences.

---

# The Main Concept to Remember

A **Token is not a command**.

A Token is one element of Shell syntax.

For example:

```bash
echo hello > file | cat
```

becomes:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("cat")
```

The **Lexer** creates this sequence.

The **Parser** interprets this sequence and converts it into commands.

The **Executor** finally executes those commands.

The complete architecture is:

```text
RAW INPUT
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
    v
COMMAND LIST
    |
    v
 EXECUTOR
```

### The key responsibility of this task

> **Create a reliable `t_token` structure that allows the Lexer to represent the user's Shell input as an ordered sequence of typed elements that the Parser can understand.**
