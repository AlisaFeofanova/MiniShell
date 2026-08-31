# Lexical Analysis — Understanding the Shell Lexer

## Table of Contents

* [1. What is Lexical Analysis?](#1-what-is-lexical-analysis)
* [2. The Lexer in Minishell](#2-the-lexer-in-minishell)
* [3. Lexer Pipeline](#3-lexer-pipeline)
* [4. What is a Token?](#4-what-is-a-token)
* [5. Token Types](#5-token-types)
* [6. Words](#6-words)
* [7. Operators](#7-operators)
* [8. Pipes](#8-pipes)
* [9. Redirections](#9-redirections)
* [10. Quotes](#10-quotes)
* [11. Single Quotes](#11-single-quotes)
* [12. Double Quotes](#12-double-quotes)
* [13. Mixing Quotes and Text](#13-mixing-quotes-and-text)
* [14. Spaces](#14-spaces)
* [15. Environment Variables](#15-environment-variables)
* [16. Why Expansion Should Not Happen in the Lexer](#16-why-expansion-should-not-happen-in-the-lexer)
* [17. Syntax Errors](#17-syntax-errors)
* [18. Lexer State Machine](#18-lexer-state-machine)
* [19. Tokenization Examples](#19-tokenization-examples)
* [20. Token Data Structure](#20-token-data-structure)
* [21. Lexer Algorithm](#21-lexer-algorithm)
* [22. Pseudocode](#22-pseudocode)
* [23. Minishell Architecture](#23-minishell-architecture)
* [24. Common Mistakes](#24-common-mistakes)
* [25. Team Checklist](#25-team-checklist)
* [26. Questions You Should Be Able to Answer](#26-questions-you-should-be-able-to-answer)
* [27. Key Idea](#27-key-idea)

---

# 1. What is Lexical Analysis?

**Lexical Analysis** is the process of converting a raw input string into a sequence of meaningful **tokens**.

For example, the user enters:

```bash
echo hello | grep hello
```

The Lexer converts it into:

```text
WORD      "echo"
WORD      "hello"
PIPE      "|"
WORD      "grep"
WORD      "hello"
```

In other words:

> The Lexer determines **what elements are present in the command**.

The Lexer does **not** execute the command.

The basic flow is:

```text
Raw Input
    |
    v
"echo hello | grep hello"
    |
    v
   LEXER
    |
    v
  TOKENS
```

---

# 2. The Lexer in Minishell

In `minishell`, the Lexer must recognize the special elements used by the shell.

For example:

```text
|
<
>
<<
>>
'
"
```

as well as ordinary text:

```text
ls
hello
file.txt
/usr/bin
$USER
```

The main goal is:

```text
Input string
     |
     v
Recognize tokens
     |
     v
Create token list
```

Example:

```bash
cat < input.txt | grep hello > output.txt
```

becomes:

```text
WORD       "cat"
REDIR_IN   "<"
WORD       "input.txt"
PIPE       "|"
WORD       "grep"
WORD       "hello"
REDIR_OUT  ">"
WORD       "output.txt"
```

---

# 3. Lexer Pipeline

The general process looks like this:

```text
                INPUT
                  |
                  v
        +-------------------+
        |   Read character  |
        +-------------------+
                  |
                  v
        +-------------------+
        | Determine token   |
        +-------------------+
                  |
                  v
        +-------------------+
        | Read token        |
        +-------------------+
                  |
                  v
        +-------------------+
        | Save token        |
        +-------------------+
                  |
                  v
             Next character
                  |
                  v
                ...
                  |
                  v
             TOKEN LIST
```

After the Lexer:

```text
TOKEN LIST
    |
    v
  Parser
```

---

# 4. What is a Token?

A **token** is one logical element of a shell command.

For example:

```bash
echo hello > file.txt
```

contains:

```text
echo
hello
>
file.txt
```

But the shell needs to know not only the value of each element, but also its **type**.

For example:

```text
"echo"      -> WORD
"hello"     -> WORD
">"         -> REDIR_OUT
"file.txt"  -> WORD
```

---

# 5. Token Types

A simple token type definition could be:

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

The mapping is:

| Symbol        | Token Type  | Meaning             |
| ------------- | ----------- | ------------------- |
| ordinary text | `WORD`      | command or argument |
| `\|`          | `PIPE`      | pipeline            |
| `<`           | `REDIR_IN`  | input redirection   |
| `>`           | `REDIR_OUT` | output redirection  |
| `<<`          | `HEREDOC`   | heredoc             |
| `>>`          | `APPEND`    | append output       |

---

# 6. Words

The most common token is `WORD`.

For example:

```bash
ls
```

becomes:

```text
WORD "ls"
```

---

## 6.1 Multiple Words

Input:

```bash
ls -la file.txt
```

Tokens:

```text
WORD "ls"
WORD "-la"
WORD "file.txt"
```

---

## 6.2 A Word Is Not Necessarily One Physical Word

This is extremely important.

Consider:

```bash
echo "hello world"
```

The result is:

```text
WORD "echo"
WORD "hello world"
```

Not:

```text
WORD "echo"
WORD "hello"
WORD "world"
```

Quotes affect tokenization.

---

# 7. Operators

Shell has several special operators.

The main ones relevant to `minishell` are:

```text
|
<
>
<<
>>
```

These are not ordinary `WORD` tokens.

For example:

```bash
cat < file
```

becomes:

```text
WORD      "cat"
REDIR_IN  "<"
WORD      "file"
```

---

# 8. Pipes

A pipe:

```bash
command1 | command2
```

creates two commands connected by a `PIPE` token.

Tokens:

```text
WORD "command1"
PIPE "|"
WORD "command2"
```

For example:

```bash
ls | grep txt
```

becomes:

```text
WORD "ls"
PIPE "|"
WORD "grep"
WORD "txt"
```

---

## 8.1 Pipes Without Spaces

These two commands:

```bash
ls|grep txt
```

and:

```bash
ls | grep txt
```

must produce the same tokens:

```text
WORD "ls"
PIPE "|"
WORD "grep"
WORD "txt"
```

Therefore, the Lexer must recognize `|` independently of surrounding spaces.

---

# 9. Redirections

## Input Redirection `<`

```bash
cat < input.txt
```

Tokens:

```text
WORD      "cat"
REDIR_IN  "<"
WORD      "input.txt"
```

---

## Output Redirection `>`

```bash
echo hello > output.txt
```

Tokens:

```text
WORD       "echo"
WORD       "hello"
REDIR_OUT  ">"
WORD       "output.txt"
```

---

## Append `>>`

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD    "echo"
WORD    "hello"
APPEND  ">>"
WORD    "output.txt"
```

---

## Heredoc `<<`

```bash
cat << EOF
```

Tokens:

```text
WORD     "cat"
HEREDOC  "<<"
WORD     "EOF"
```

---

# 10. Quotes

Quotes are one of the most important parts of shell lexical analysis.

There are two main types:

```text
'
"
```

They behave differently.

---

# 11. Single Quotes

Single quotes:

```bash
'text'
```

mean:

> Treat everything inside as literal text.

For example:

```bash
echo '$USER'
```

The `$USER` inside single quotes must **not** be expanded.

Conceptually, the Lexer sees:

```text
WORD "$USER"
```

But it must preserve the information that this text was inside single quotes.

Expansion happens later.

---

# 11.1 Why Does Quote Information Matter?

Compare:

```bash
echo '$USER'
```

with:

```bash
echo "$USER"
```

The text appears similar, but their behavior is different.

Inside single quotes:

```text
$USER
```

is literal.

Inside double quotes:

```text
$USER
```

can be expanded.

Therefore, the Lexer must not lose quote context.

---

# 12. Double Quotes

Double quotes:

```bash
"hello world"
```

keep everything inside as one word.

For example:

```bash
echo "hello world"
```

produces:

```text
WORD "echo"
WORD "hello world"
```

---

## 12.1 Variables Inside Double Quotes

Consider:

```bash
echo "$USER"
```

The Lexer should understand that:

```text
"$USER"
```

is one logical word.

Later, Expansion can transform:

```text
"$USER"
```

into something like:

```text
"alice"
```

if:

```text
USER=alice
```

---

# 13. Mixing Quotes and Text

This is extremely important.

Consider:

```bash
echo hello"world"
```

This is **not** two arguments.

It is one word:

```text
helloworld
```

Therefore:

```text
WORD "helloworld"
```

---

## Another Example

```bash
echo "hello"'world'
```

Result:

```text
WORD "helloworld"
```

---

## Another Example

```bash
echo abc"def"ghi
```

Result:

```text
WORD "abcdefghi"
```

The quotes do not necessarily create separate tokens.

They can be part of the same `WORD`.

---

# 14. Spaces

Normally, spaces separate words.

For example:

```bash
echo hello world
```

becomes:

```text
WORD "echo"
WORD "hello"
WORD "world"
```

---

## 14.1 Multiple Spaces

```bash
echo      hello
```

still becomes:

```text
WORD "echo"
WORD "hello"
```

Multiple unquoted spaces are separators.

---

## 14.2 Spaces Inside Quotes

However:

```bash
echo "hello     world"
```

becomes:

```text
WORD "echo"
WORD "hello     world"
```

The spaces inside quotes do not separate the word.

---

# 15. Environment Variables

The shell supports variables such as:

```bash
$USER
$HOME
$PATH
```

For example:

```bash
echo $USER
```

The Lexer can represent this as:

```text
WORD "echo"
WORD "$USER"
```

Expansion can then transform:

```text
$USER
```

into:

```text
alice
```

if:

```text
USER=alice
```

---

# 16. Why Expansion Should Not Happen in the Lexer

This is an important architectural principle.

The Lexer is responsible for understanding the **structure** of the input.

Expansion is a separate operation.

Conceptually:

```text
Raw Input
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
 Expansion
    |
    v
 Execution
```

For example:

```bash
echo $USER
```

Lexer:

```text
WORD "echo"
WORD "$USER"
```

Expansion:

```text
"$USER"
   |
   v
"alice"
```

---

## Why Not Expand Immediately?

Because quotes change how expansion works.

Compare:

```bash
'$USER'
```

and:

```bash
"$USER"
```

If the Lexer immediately replaces `$USER`, it may lose important information about the original context.

Therefore:

> The Lexer should recognize and preserve the necessary structure. Expansion should happen later.

---

# 17. Syntax Errors

The Lexer should detect lexical problems such as unclosed quotes.

For example:

```bash
echo "hello
```

There is an opening:

```text
"
```

but no closing quote.

Similarly:

```bash
echo 'hello
```

contains an unclosed single quote.

---

## 17.1 Lexical Error vs Syntax Error

It is important to distinguish:

```text
Lexical error
```

from:

```text
Syntax error
```

For example:

```bash
echo "hello
```

has an unclosed quote and is a lexical/quote-processing problem.

But:

```bash
echo hello |
```

may tokenize as:

```text
WORD "echo"
WORD "hello"
PIPE "|"
```

and the fact that the pipe has no command after it is a **syntax** issue that the Parser should detect.

The exact division of responsibility can depend on your project architecture, but the concepts should remain separate.

---

# 18. Lexer State Machine

One of the best ways to understand a Lexer is to think of it as a **Finite State Machine**.

The main states are:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

---

# 18.1 NORMAL State

In the normal state:

```text
NORMAL
```

the Lexer can encounter:

```text
space
'
"
|
<
>
```

For example:

```bash
echo "hello"
     ^
```

When the Lexer sees:

```text
"
```

it transitions:

```text
NORMAL
   |
   | "
   v
DOUBLE_QUOTE
```

---

# 18.2 SINGLE_QUOTE State

Inside:

```bash
'hello world'
```

the Lexer is in:

```text
SINGLE_QUOTE
```

Most characters are treated as ordinary text.

The next:

```text
'
```

closes the single-quoted section.

Transition:

```text
SINGLE_QUOTE
      |
      | '
      v
    NORMAL
```

---

# 18.3 DOUBLE_QUOTE State

Inside:

```bash
"hello $USER"
```

the Lexer is in:

```text
DOUBLE_QUOTE
```

The next:

```text
"
```

closes the double-quoted section.

Transition:

```text
DOUBLE_QUOTE
      |
      | "
      v
    NORMAL
```

---

# 18.4 State Diagram

```text
                    "
        +--------------------------+
        |                          v
    +--------+              +-------------+
    | NORMAL |              |   DOUBLE    |
    +--------+              |   QUOTE     |
        |                   +-------------+
        | '                        |
        v                          | "
 +---------------+                 |
 | SINGLE_QUOTE  |-----------------+
 +---------------+
        |
        | '
        v
     NORMAL
```

Simplified:

```text
NORMAL
  |
  +-- ' --> SINGLE_QUOTE
  |             |
  |             +-- ' --> NORMAL
  |
  +-- " --> DOUBLE_QUOTE
                |
                +-- " --> NORMAL
```

---

# 19. Tokenization Examples

## Example 1

Input:

```bash
echo hello
```

Tokens:

```text
WORD "echo"
WORD "hello"
```

---

## Example 2

Input:

```bash
echo hello world
```

Tokens:

```text
WORD "echo"
WORD "hello"
WORD "world"
```

---

## Example 3

Input:

```bash
ls | grep txt
```

Tokens:

```text
WORD "ls"
PIPE "|"
WORD "grep"
WORD "txt"
```

---

## Example 4

Input:

```bash
cat < input.txt
```

Tokens:

```text
WORD      "cat"
REDIR_IN  "<"
WORD      "input.txt"
```

---

## Example 5

Input:

```bash
echo hello > output.txt
```

Tokens:

```text
WORD       "echo"
WORD       "hello"
REDIR_OUT  ">"
WORD       "output.txt"
```

---

## Example 6

Input:

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD    "echo"
WORD    "hello"
APPEND  ">>"
WORD    "output.txt"
```

---

## Example 7

Input:

```bash
cat << EOF
```

Tokens:

```text
WORD     "cat"
HEREDOC  "<<"
WORD     "EOF"
```

---

## Example 8

Input:

```bash
echo "hello world"
```

Tokens:

```text
WORD "echo"
WORD "hello world"
```

---

## Example 9

Input:

```bash
echo '$USER'
```

Tokens conceptually:

```text
WORD "echo"
WORD "$USER"
```

But the Lexer must preserve the fact that `$USER` was inside single quotes.

---

## Example 10

Input:

```bash
echo "$USER"
```

Tokens conceptually:

```text
WORD "echo"
WORD "$USER"
```

Again, quote context must be preserved because expansion behavior is different.

---

## Example 11

Input:

```bash
echo hello"world"
```

Result:

```text
WORD "helloworld"
```

---

## Example 12

Input:

```bash
echo "hello"'world'
```

Result:

```text
WORD "helloworld"
```

---

## Example 13

Input:

```bash
echo abc"123"def
```

Result:

```text
WORD "abc123def"
```

---

# 20. Token Data Structure

A simple linked-list structure could look like:

```c
typedef struct s_token
{
    char            *value;
    t_token_type    type;
    struct s_token  *next;
}   t_token;
```

For example:

```text
+---------+-------------+
| value   | type        |
+---------+-------------+
| echo    | WORD        |
| hello   | WORD        |
| |       | PIPE        |
| grep    | WORD        |
| hello   | WORD        |
+---------+-------------+
```

---

# 20.1 Should Quotes Be Stored?

There are several possible architectures.

### Option 1 — Keep Original Quotes

Store something like:

```text
value = "\"hello world\""
```

and process quotes later.

### Option 2 — Remove Quotes and Store Metadata

The Lexer can remove quotes while keeping information about the quote context.

### Option 3 — Store Word Parts

A more detailed representation could be:

```text
WORD
 |
 +-- TEXT: hello
 |
 +-- DOUBLE_QUOTED: world
 |
 +-- TEXT: test
```

This can make later Expansion easier.

The important rule is:

> Do not destroy information that later stages need.

---

# 21. Lexer Algorithm

A general algorithm is:

```text
1. Start at the first character.
2. Skip unquoted spaces.
3. Check for special operators.
4. If `|` is found, create a PIPE token.
5. If `<` or `>` is found, check whether it forms a double operator.
6. If a quote is found, enter the appropriate quote state.
7. If normal text is found, start building a WORD.
8. Continue until a separator or operator is reached.
9. Create the WORD token.
10. Move to the next character.
11. Repeat until the end of the input.
```

---

# 21.1 The Main Principle

The Lexer reads:

```text
character by character
```

For example:

```bash
echo "hello world" | grep hello
```

The characters are:

```text
e
c
h
o
space
"
h
e
l
l
o
space
w
o
r
l
d
"
space
|
space
g
r
e
p
...
```

The Lexer groups them into logical tokens:

```text
echo
hello world
|
grep
hello
```

---

# 22. Pseudocode

A simplified Lexer could work like this:

```text
while current character exists:

    skip spaces

    if current == '|':
        create PIPE token
        move forward

    else if current == '<':
        if next == '<':
            create HEREDOC token
            move 2 characters
        else:
            create REDIR_IN token
            move 1 character

    else if current == '>':
        if next == '>':
            create APPEND token
            move 2 characters
        else:
            create REDIR_OUT token
            move 1 character

    else:
        start reading WORD

        while current is not:
            space
            pipe
            redirection
            end of input

            if current == '\'':
                process SINGLE_QUOTE

            else if current == '"':
                process DOUBLE_QUOTE

            else:
                add character to WORD

        create WORD token
```

---

# 22.1 Reading a WORD

Consider:

```bash
echo abc"hello"xyz
```

The Lexer starts with:

```text
a
b
c
```

Then it sees:

```text
"
```

and enters:

```text
DOUBLE_QUOTE
```

It reads:

```text
hello
```

Then it sees:

```text
"
```

and returns to:

```text
NORMAL
```

Then it continues reading:

```text
xyz
```

The final result is:

```text
WORD "abchelloxyz"
```

---

# 23. Minishell Architecture

A clean architecture can look like this:

```text
                     readline()
                         |
                         v
                     raw string
                         |
                         v
                     +-------+
                     | Lexer |
                     +-------+
                         |
                         v
                      tokens
                         |
                         v
                     +--------+
                     | Parser |
                     +--------+
                         |
                         v
                 command structure
                         |
                         v
                    Expansion
                         |
                         v
                  Redirections
                         |
                         v
                     Execution
```

---

# 23.1 Lexer Responsibilities

The Lexer should be responsible for:

```text
✔ recognizing tokens
✔ recognizing operators
✔ handling quote states
✔ grouping characters into WORD tokens
✔ creating the token list
✔ detecting relevant lexical problems
```

The Lexer should **not** be responsible for:

```text
✘ fork()
✘ execve()
✘ pipe()
✘ waitpid()
✘ executing commands
```

Keep responsibilities separated.

---

# 24. Common Mistakes

## Mistake 1 — Using `split()` by spaces

For example:

```c
split(line, ' ');
```

does not correctly implement shell tokenization.

Consider:

```bash
echo "hello world"
```

A normal split could produce:

```text
echo
"hello
world"
```

But the shell needs:

```text
echo
hello world
```

as two logical words.

---

# Mistake 2 — Ignoring Operators Without Spaces

The Lexer must correctly handle:

```bash
ls|grep txt
```

not only:

```bash
ls | grep txt
```

---

# Mistake 3 — Treating `>>` as Two Operators

This:

```bash
>>
```

must be recognized as:

```text
APPEND
```

not:

```text
REDIR_OUT
REDIR_OUT
```

---

# Mistake 4 — Treating `<<` as Two Operators

Similarly:

```bash
<<
```

must become:

```text
HEREDOC
```

---

# Mistake 5 — Splitting Words Around Quotes

This:

```bash
echo hello"world"
```

must not become:

```text
WORD "hello"
WORD "world"
```

It is:

```text
WORD "helloworld"
```

---

# Mistake 6 — Removing Quotes Too Early

If you completely remove quote information during lexical analysis, later stages may no longer know whether:

```bash
'$USER'
```

was single-quoted or:

```bash
"$USER"
```

was double-quoted.

That information can affect Expansion.

---

# Mistake 7 — Performing Expansion in the Lexer

Do not make the Lexer responsible for replacing:

```text
$USER
```

with:

```text
alice
```

Instead:

```text
Lexer
  |
  v
"$USER"
  |
  v
Expansion
  |
  v
"alice"
```

---

# Mistake 8 — Ignoring Empty Quotes

Consider:

```bash
echo ""
```

The empty quotes represent an **empty argument**.

Conceptually:

```text
argv[0] = "echo"
argv[1] = ""
```

This is different from having no second argument.

---

# Mistake 9 — Not Tracking Quote States

The Lexer should understand:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

For example:

```bash
echo "'"
```

and:

```bash
echo '"'
```

must be processed correctly.

---

# 25. Team Checklist

Every team member should understand the following.

## Basic Concepts

* [ ] What lexical analysis is.
* [ ] What a Lexer does.
* [ ] What a token is.
* [ ] The difference between Lexer and Parser.
* [ ] Why `split()` is not enough for shell parsing.

---

## Tokens

* [ ] `WORD`
* [ ] `PIPE`
* [ ] `REDIR_IN`
* [ ] `REDIR_OUT`
* [ ] `APPEND`
* [ ] `HEREDOC`

---

## Operators

* [ ] `|`
* [ ] `<`
* [ ] `>`
* [ ] `<<`
* [ ] `>>`

---

## Quotes

* [ ] Single quotes `'`.
* [ ] Double quotes `"`.
* [ ] Quotes inside words.
* [ ] Multiple quote sections.
* [ ] Empty quotes.
* [ ] Unclosed quotes.

---

## Words

Understand why:

```bash
hello"world"
```

becomes:

```text
helloworld
```

and why:

```bash
"hello world"
```

remains one word.

---

## State Machine

Understand these states:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

and the transitions:

```text
NORMAL
  |
  +-- ' --> SINGLE_QUOTE
  |
  +-- " --> DOUBLE_QUOTE
```

---

# 26. Questions You Should Be Able to Answer

Before considering the Lexer part understood, every team member should be able to answer these questions.

## General Questions

1. What is lexical analysis?
2. What does a Lexer do?
3. What is a token?
4. What is the difference between a Lexer and a Parser?
5. Why can't we simply use `split()` by spaces?

---

## Operators

6. Which shell operators must `minishell` recognize?
7. What is the difference between `<` and `<<`?
8. What is the difference between `>` and `>>`?
9. Why must `>>` be one token?
10. Why should `ls|grep` and `ls | grep` produce the same tokens?

---

## Quotes

11. Why do we need quote states?
12. What is the difference between `'...'` and `"..."`?
13. Why does:

```bash
echo "hello world"
```

have only one argument after `echo`?

14. Why does:

```bash
echo hello"world"
```

become:

```text
helloworld
```

15. What should happen with:

```bash
echo ""
```

16. What happens with:

```bash
echo "hello
```

---

## Expansion

17. Should the Lexer perform `$USER` expansion?
18. Why must quote information sometimes be preserved?
19. What is the difference between:

```bash
'$USER'
```

and:

```bash
"$USER"
```

---

# 27. Key Idea

The most important concept to remember is:

> **The Lexer does not execute the command. The Lexer understands the structure of the command line.**

For example, the user enters:

```bash
cat < input.txt | grep "$USER" >> result.txt
```

The Lexer should identify:

```text
WORD       "cat"
REDIR_IN   "<"
WORD       "input.txt"
PIPE       "|"
WORD       "grep"
WORD       "$USER"
APPEND     ">>"
WORD       "result.txt"
```

Then the information moves through the shell pipeline:

```text
                    RAW INPUT
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
                     EXECUTION
```

---

# Final Mental Model

When implementing the Lexer, constantly ask:

> **"What does this character mean in the current context?"**

For example:

```text
NORMAL:
    "  -> start DOUBLE_QUOTE
    '  -> start SINGLE_QUOTE
    |  -> PIPE
    <  -> REDIR_IN / HEREDOC
    >  -> REDIR_OUT / APPEND
    space -> separator


SINGLE_QUOTE:
    most characters -> literal text
    ' -> end SINGLE_QUOTE


DOUBLE_QUOTE:
    most characters -> quoted text
    " -> end DOUBLE_QUOTE
```

The key concept is:

```text
CHARACTERS
     |
     v
  CONTEXT
     |
     v
   TOKENS
     |
     v
   PARSER
     |
     v
  COMMANDS
```

If your Lexer can correctly transform:

```bash
echo abc"hello world"def | grep 'hello world' > result.txt
```

into:

```text
WORD       "echo"
WORD       "abchello worlddef"
PIPE       "|"
WORD       "grep"
WORD       "hello world"
REDIR_OUT  ">"
WORD       "result.txt"
```

while preserving the necessary quote information, then you understand the core of **Lexical Analysis for Minishell**.
