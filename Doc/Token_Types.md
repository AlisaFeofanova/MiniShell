# Understanding Token Types in Minishell

## Table of Contents

* [1. What Is a Token?](#1-what-is-a-token)
* [2. Why Do We Need Token Types?](#2-why-do-we-need-token-types)
* [3. Lexer and Tokens](#3-lexer-and-tokens)
* [4. Main Token Types](#4-main-token-types)
* [5. WORD](#5-word)
* [6. PIPE](#6-pipe)
* [7. REDIR_IN](#7-redir_in)
* [8. REDIR_OUT](#8-redir_out)
* [9. APPEND](#9-append)
* [10. HEREDOC](#10-heredoc)
* [11. END / EOF](#11-end--eof)
* [12. Why Do Quotes Change Token Type?](#12-why-do-quotes-change-token-type)
* [13. Quoted Operators](#13-quoted-operators)
* [14. Operators Inside WORD](#14-operators-inside-word)
* [15. Tokenization Examples](#15-tokenization-examples)
* [16. Token Structure](#16-token-structure)
* [17. Enum for Token Types](#17-enum-for-token-types)
* [18. How Does the Lexer Determine Token Type?](#18-how-does-the-lexer-determine-token-type)
* [19. Lexer Algorithm](#19-lexer-algorithm)
* [20. Parser and Token Types](#20-parser-and-token-types)
* [21. Tokens → Command Structure](#21-tokens--command-structure)
* [22. Lexer Errors](#22-lexer-errors)
* [23. Parser Errors](#23-parser-errors)
* [24. Common Mistakes](#24-common-mistakes)
* [25. Testing Examples](#25-testing-examples)
* [26. Checklist](#26-checklist)
* [27. Knowledge Check](#27-knowledge-check)
* [28. Main Mental Model](#28-main-mental-model)

---

# 1. What Is a Token?

A **token** is a meaningful unit of a shell command line.

For example:

```bash
echo hello
```

can be represented as:

```text
WORD("echo")
WORD("hello")
```

Another example:

```bash
echo hello | grep hello
```

becomes:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

A token should contain at least:

```text
value
type
```

For example:

```text
value = "echo"
type  = WORD
```

---

# 2. Why Do We Need Token Types?

The Parser needs to understand the structure of the command.

For example:

```bash
cat file.txt > output.txt
```

The Lexer produces:

```text
WORD("cat")
WORD("file.txt")
REDIR_OUT(">")
WORD("output.txt")
```

Now the Parser can understand:

```text
command = cat
argument = file.txt
output redirection = output.txt
```

Without token types, the Parser would not know whether:

```text
>
```

is an operator or ordinary text.

Therefore:

> A Token Type tells the Parser what role a particular part of the input has.

---

# 3. Lexer and Tokens

The general Minishell pipeline is:

```text
RAW INPUT
    |
    v
+---------+
|  LEXER  |
+---------+
    |
    v
  TOKENS
    |
    v
+---------+
| PARSER  |
+---------+
    |
    v
COMMAND STRUCTURE
    |
    v
EXPANSION
    |
    v
EXECUTION
```

For example:

```bash
echo hello | grep hello
```

The Lexer creates:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

The Parser then uses these tokens to understand the command structure.

---

# 4. Main Token Types

For a basic Minishell, the main token types are:

```text
WORD
PIPE
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
```

It is also useful to have:

```text
EOF
```

to mark the end of the input.

---

## Token Table

| Token Type  | Symbol       | Meaning                           |
| ----------- | ------------ | --------------------------------- |
| `WORD`      | text         | command, argument, filename, etc. |
| `PIPE`      | `\|`         | connects commands                 |
| `REDIR_IN`  | `<`          | input redirection                 |
| `REDIR_OUT` | `>`          | output redirection                |
| `APPEND`    | `>>`         | append output                     |
| `HEREDOC`   | `<<`         | heredoc                           |
| `EOF`       | end of input | end of token stream               |

---

# 5. WORD

`WORD` is the most common token type.

Examples:

```text
echo
hello
file.txt
$USER
abc
"hello world"
```

For:

```bash
echo hello
```

we get:

```text
WORD("echo")
WORD("hello")
```

---

## 5.1 A Command Is Also a WORD

This is very important.

For:

```bash
echo hello
```

`echo` does **not** need a special:

```text
COMMAND
```

token type.

The Lexer produces:

```text
WORD("echo")
WORD("hello")
```

The Parser determines that the first `WORD` is the command.

---

## 5.2 A Filename Is Also a WORD

For:

```bash
cat file.txt
```

the Lexer produces:

```text
WORD("cat")
WORD("file.txt")
```

`file.txt` is still a `WORD`.

---

## 5.3 Filename After Redirection

For:

```bash
echo hello > output.txt
```

the tokens are:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

`output.txt` is still a `WORD`.

The Parser understands that this `WORD` is the target of the redirection.

---

# 6. PIPE

The symbol:

```text
|
```

has the token type:

```text
PIPE
```

For:

```bash
ls | grep txt
```

we get:

```text
WORD("ls")
PIPE("|")
WORD("grep")
WORD("txt")
```

A pipe connects the output of one command to the input of another command.

Conceptually:

```text
ls
 |
 v
grep
```

---

## 6.1 Pipe Is Not a WORD

Without quotes:

```bash
echo hello | grep hello
```

the `|` is:

```text
PIPE
```

not:

```text
WORD("|")
```

---

## 6.2 Pipe Inside Quotes

However:

```bash
echo "|"
```

produces:

```text
WORD("echo")
WORD("|")
```

The `|` is inside quotes, so it is ordinary text.

The same applies to single quotes:

```bash
echo '|'
```

produces:

```text
WORD("echo")
WORD("|")
```

---

# 7. REDIR_IN

The symbol:

```text
<
```

has the token type:

```text
REDIR_IN
```

For:

```bash
cat < input.txt
```

the Lexer produces:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

The Parser understands:

```text
stdin ← input.txt
```

---

## 7.1 `<` Inside Quotes

For:

```bash
echo "<"
```

the result is:

```text
WORD("echo")
WORD("<")
```

The `<` is not a redirection because it is inside quotes.

---

# 8. REDIR_OUT

The symbol:

```text
>
```

has the token type:

```text
REDIR_OUT
```

For:

```bash
echo hello > output.txt
```

we get:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

Conceptually:

```text
stdout → output.txt
```

---

## 8.1 `>` Inside Quotes

For:

```bash
echo ">"
```

we get:

```text
WORD("echo")
WORD(">")
```

It is not a redirection operator.

---

# 9. APPEND

The two-character operator:

```text
>>
```

is one token:

```text
APPEND
```

For:

```bash
echo hello >> output.txt
```

we get:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("output.txt")
```

---

## 9.1 Difference Between `>` and `>>`

```text
>   → overwrite
>>  → append
```

For example:

```bash
echo hello > file
```

writes to the file, replacing its previous contents.

While:

```bash
echo hello >> file
```

adds the output to the end of the file.

---

## 9.2 `>>` Is One Token

This:

```text
>>
```

must become:

```text
APPEND
```

not:

```text
REDIR_OUT
REDIR_OUT
```

The Lexer must recognize the two-character operator as a single token.

---

# 10. HEREDOC

The two-character operator:

```text
<<
```

has the token type:

```text
HEREDOC
```

For:

```bash
cat << EOF
```

the Lexer produces:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

`EOF` is the heredoc delimiter.

---

## 10.1 What Does `<<` Do?

The shell reads input until it encounters the delimiter.

Example:

```bash
cat << EOF
hello
world
EOF
```

The input given to `cat` is:

```text
hello
world
```

---

## 10.2 `<<` Is One Token

Just like `>>`:

```text
<<
```

must become:

```text
HEREDOC
```

not:

```text
REDIR_IN
REDIR_IN
```

---

# 11. END / EOF

The Lexer needs to know when the input has ended.

For:

```bash
echo hello
```

the token stream can be:

```text
WORD("echo")
WORD("hello")
EOF
```

`EOF` is useful because the Parser can clearly detect the end of the token list.

---

# 12. Why Do Quotes Change Token Type?

This is one of the most important concepts.

The Token Type depends not only on the character itself, but also on its context.

Compare:

```bash
echo |
```

with:

```bash
echo "|"
```

First:

```text
WORD("echo")
PIPE("|")
```

Second:

```text
WORD("echo")
WORD("|")
```

Why?

Because in the second case, `|` is inside quotes.

Therefore:

> Operators are recognized as operators only when they are in the appropriate shell parsing context, not when they are inside quotes.

---

# 13. Quoted Operators

Operators inside quotes become part of a `WORD`.

### Pipe

```bash
echo "|"
```

→

```text
WORD("echo")
WORD("|")
```

### Input redirection

```bash
echo "<"
```

→

```text
WORD("echo")
WORD("<")
```

### Output redirection

```bash
echo ">"
```

→

```text
WORD("echo")
WORD(">")
```

### Append

```bash
echo ">>"
```

→

```text
WORD("echo")
WORD(">>")
```

### Heredoc

```bash
echo "<<"
```

→

```text
WORD("echo")
WORD("<<")
```

---

# 14. Operators Inside WORD

Operators do not require spaces around them.

For example:

```bash
echo hello>file
```

must be tokenized as:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

Not:

```text
WORD("hello>file")
```

---

## Another Example

```bash
echo hello|grep
```

becomes:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
```

---

## Another Example

```bash
cat<file
```

becomes:

```text
WORD("cat")
REDIR_IN("<")
WORD("file")
```

---

# 15. Tokenization Examples

## Example 1 — Basic Command

Input:

```bash
echo hello
```

Tokens:

```text
WORD("echo")
WORD("hello")
```

---

## Example 2 — Multiple Arguments

Input:

```bash
echo hello world
```

Tokens:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

---

## Example 3 — Pipe

Input:

```bash
ls | grep txt
```

Tokens:

```text
WORD("ls")
PIPE("|")
WORD("grep")
WORD("txt")
```

---

## Example 4 — Input Redirection

Input:

```bash
cat < input.txt
```

Tokens:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

## Example 5 — Output Redirection

Input:

```bash
echo hello > output.txt
```

Tokens:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

## Example 6 — Append

Input:

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("output.txt")
```

---

## Example 7 — Heredoc

Input:

```bash
cat << EOF
```

Tokens:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

## Example 8 — Quotes

Input:

```bash
echo "hello world"
```

Tokens:

```text
WORD("echo")
WORD("hello world")
```

The spaces inside quotes do not separate tokens.

---

## Example 9 — Variable

Input:

```bash
echo "$USER"
```

Tokens:

```text
WORD("echo")
WORD("$USER")
```

Expansion happens later.

The Lexer should not perform variable expansion.

---

## Example 10 — Single Quotes

Input:

```bash
echo '$USER'
```

Tokens:

```text
WORD("echo")
WORD("$USER")
```

The difference between single and double quotes matters later during expansion.

---

# 16. Token Structure

A simple C structure can look like:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
}   t_token;
```

For example:

```text
t_token
   |
   +-- value = "echo"
   +-- type  = WORD
   |
   v
t_token
   |
   +-- value = "|"
   +-- type  = PIPE
   |
   v
t_token
   |
   +-- value = "grep"
   +-- type  = WORD
```

---

## Doubly Linked List

You can also use:

```c
typedef struct s_token
{
    char            *value;
    int             type;
    struct s_token  *next;
    struct s_token  *prev;
}   t_token;
```

A doubly linked list can make some Parser operations easier.

---

# 17. Enum for Token Types

Using an `enum` is recommended.

For example:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND,
    TOKEN_HEREDOC,
    TOKEN_EOF
}   t_token_type;
```

Instead of:

```c
type = 3;
```

you can write:

```c
type = TOKEN_REDIR_OUT;
```

This is much easier to understand and maintain.

---

# 18. How Does the Lexer Determine Token Type?

A simplified decision process:

```text
Current character
       |
       v
Is it whitespace?
       |
       +-- YES → skip it
       |
       NO
       |
       v
Is it inside quotes?
       |
       +-- YES → parse quoted WORD
       |
       NO
       |
       v
Is it "|"?
       |
       +-- YES → PIPE
       |
       NO
       |
       v
Is it "<"?
       |
       +-- YES → REDIR_IN or HEREDOC
       |
       NO
       |
       v
Is it ">"?
       |
       +-- YES → REDIR_OUT or APPEND
       |
       NO
       |
       v
Parse WORD
```

The exact implementation can vary, but the important principle is:

> Quote state must be considered while recognizing operators.

---

# 19. Lexer Algorithm

A useful general algorithm is:

```text
while input is not finished:

    1. Skip whitespace.

    2. Check for operators.

       "|"   → PIPE
       "<"   → REDIR_IN
       ">"   → REDIR_OUT
       "<<"  → HEREDOC
       ">>"  → APPEND

    3. Otherwise parse a WORD.

       A WORD may contain:
       - normal characters
       - single quotes
       - double quotes
       - escaped characters
       - expansions that will be processed later

    4. Create a token.

    5. Add it to the token list.

    6. Continue until the input ends.
```

But remember:

```text
Operators must only be recognized when they are
not protected by quotes.
```

---

# 19.1 Why Can't We Simply Search for Characters?

A naive implementation might do:

```c
if (input[i] == '|')
    token->type = TOKEN_PIPE;
```

This is incorrect.

Consider:

```bash
echo "|"
```

The `|` is not a pipe.

It is ordinary text.

Therefore, the Lexer needs to know whether it is currently inside:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

---

# 19.2 The Same Applies to `<` and `>`

This is wrong:

```c
if (input[i] == '>')
    token->type = TOKEN_REDIR_OUT;
```

because:

```bash
echo ">"
```

contains `>` as ordinary text.

The correct logic is conceptually:

```text
if state == NORMAL and current character == '>'
    → REDIR_OUT
else
    → part of WORD
```

---

# 20. Parser and Token Types

The Parser uses Token Types to understand the grammar.

For:

```bash
echo hello | grep hello
```

the Lexer creates:

```text
WORD
WORD
PIPE
WORD
WORD
```

The Parser can understand this as:

```text
Command 1
    |
    +-- echo
    +-- hello

PIPE

Command 2
    |
    +-- grep
    +-- hello
```

---

# 20.1 Redirection

Input:

```bash
echo hello > output.txt
```

Tokens:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

The Parser can build:

```text
COMMAND
 |
 +-- argv:
 |     echo
 |     hello
 |
 +-- redirection:
       >
       output.txt
```

---

# 20.2 Heredoc

Input:

```bash
cat << EOF
```

Tokens:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

The Parser understands:

```text
command = cat

heredoc delimiter = EOF
```

The actual heredoc input is handled later.

---

# 21. Tokens → Command Structure

There are different ways to represent the parsed command.

## Approach 1 — Command List

For example:

```c
typedef struct s_command
{
    char    **argv;
    /* redirections */
    /* etc. */
}   t_command;
```

---

## Approach 2 — AST

You can represent:

```bash
ls | grep txt
```

as:

```text
          PIPE
         /    \
        /      \
      CMD      CMD
       |        |
      ls       grep
                 \
                 txt
```

Or conceptually:

```text
          PIPE
         /    \
       ls     grep txt
```

---

## Redirection

For:

```bash
cat < input.txt
```

the structure could be:

```text
COMMAND
 |
 +-- argv:
 |     cat
 |
 +-- REDIR_IN
       |
       +-- input.txt
```

---

# 22. Lexer Errors

The Lexer can detect certain syntax problems.

For example:

```bash
echo "hello
```

contains an unclosed quote.

The Lexer should detect this and return an error.

Conceptually:

```text
UNCLOSED_QUOTE
```

or an appropriate error status.

---

# 23. Parser Errors

Other errors are detected by the Parser.

For example:

```bash
|
```

A pipe cannot appear without commands around it.

---

### Pipe at the End

```bash
echo hello |
```

After `PIPE`, the Parser expects another command.

---

### Redirection Without Target

```bash
echo hello >
```

After:

```text
REDIR_OUT
```

the Parser expects a `WORD`.

---

### Heredoc Without Delimiter

```bash
cat <<
```

After:

```text
HEREDOC
```

the Parser expects a delimiter represented by a `WORD`.

---

# 24. Common Mistakes

## Mistake 1 — Treating `>>` as Two `>`

Wrong:

```text
REDIR_OUT
REDIR_OUT
```

Correct:

```text
APPEND
```

---

## Mistake 2 — Treating `<<` as Two `<`

Wrong:

```text
REDIR_IN
REDIR_IN
```

Correct:

```text
HEREDOC
```

---

## Mistake 3 — Treating Quoted Operators as Operators

Wrong:

```bash
echo "|"
```

→

```text
PIPE
```

Correct:

```text
WORD("|")
```

---

## Mistake 4 — Creating a COMMAND Token

You generally do not need:

```text
COMMAND
```

For:

```bash
echo hello
```

the Lexer should produce:

```text
WORD("echo")
WORD("hello")
```

The Parser determines that `echo` is the command.

---

## Mistake 5 — Creating a FILENAME Token

For:

```bash
cat < input.txt
```

you do not need:

```text
REDIR_IN
FILENAME
```

Use:

```text
REDIR_IN
WORD("input.txt")
```

The Parser understands the role of the `WORD`.

---

## Mistake 6 — Assuming Operators Need Spaces

This:

```bash
echo hello>file
```

is valid tokenization.

It should produce:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

# 25. Testing Examples

## Basic

```bash
echo hello
```

Expected:

```text
WORD echo
WORD hello
```

---

## Pipe

```bash
echo hello | grep hello
```

Expected:

```text
WORD echo
WORD hello
PIPE
WORD grep
WORD hello
```

---

## Input Redirection

```bash
cat < input
```

Expected:

```text
WORD cat
REDIR_IN
WORD input
```

---

## Output Redirection

```bash
echo hello > file
```

Expected:

```text
WORD echo
WORD hello
REDIR_OUT
WORD file
```

---

## Append

```bash
echo hello >> file
```

Expected:

```text
WORD echo
WORD hello
APPEND
WORD file
```

---

## Heredoc

```bash
cat << EOF
```

Expected:

```text
WORD cat
HEREDOC
WORD EOF
```

---

## No Spaces

```bash
cat<input
```

Expected:

```text
WORD cat
REDIR_IN
WORD input
```

---

```bash
echo hello>file
```

Expected:

```text
WORD echo
WORD hello
REDIR_OUT
WORD file
```

---

```bash
echo hello|grep
```

Expected:

```text
WORD echo
WORD hello
PIPE
WORD grep
```

---

## Quotes

```bash
echo "hello | world"
```

Expected:

```text
WORD echo
WORD "hello | world"
```

The `|` must not become `PIPE`.

---

```bash
echo 'hello > world'
```

Expected:

```text
WORD echo
WORD 'hello > world'
```

The `>` must not become `REDIR_OUT`.

---

## Concatenation

```bash
echo hello"world"
```

The shell treats this as one word:

```text
WORD("helloworld")
```

---

## Multiple Redirections

```bash
cat < input > output
```

Expected:

```text
WORD cat
REDIR_IN
WORD input
REDIR_OUT
WORD output
```

---

## Complex Example

Input:

```bash
cat < input.txt | grep "hello world" >> result.txt
```

Expected:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
PIPE("|")
WORD("grep")
WORD("hello world")
APPEND(">>")
WORD("result.txt")
```

---

# 26. Checklist

Before considering **Token Types** understood, you should be able to explain:

## Basic Concepts

* [ ] What is a Token?
* [ ] What is a Token Type?
* [ ] Why does the Parser need Token Types?
* [ ] Why is a command a `WORD`?
* [ ] Why is a filename a `WORD`?
* [ ] What is `EOF`?

---

## Operators

You should recognize:

```text
|
<
>
<<
>>
```

as:

```text
PIPE
REDIR_IN
REDIR_OUT
HEREDOC
APPEND
```

---

## Quotes

You should understand why:

```bash
echo "|"
echo "<"
echo ">"
echo "<<"
echo ">>"
```

produce `WORD` tokens rather than operator tokens.

---

## Operators Without Spaces

You should understand:

```bash
cat<input
```

```bash
echo hello>file
```

```bash
echo hello|grep
```

and know exactly how they are tokenized.

---

## Lexer

You should understand:

```text
Raw Input
    ↓
Lexer
    ↓
Tokens
```

---

## Parser

You should understand:

```text
Tokens
    ↓
Parser
    ↓
Command Structure
```

---

# 27. Knowledge Check

Try to answer these without looking at the previous sections.

## Basic Questions

1. What is a Token?
2. What is a Token Type?
3. Why does Minishell need a Lexer?
4. Why does Minishell need a Parser?
5. Why is `echo` a `WORD` rather than a `COMMAND` token?
6. Why is `file.txt` a `WORD`?

---

## Operators

7. What Token Type does `|` have?
8. What Token Type does `<` have?
9. What Token Type does `>` have?
10. What Token Type does `>>` have?
11. What Token Type does `<<` have?

---

## Quotes

12. What tokens are produced by:

```bash
echo "|"
```

13. What tokens are produced by:

```bash
echo ">"
```

14. Why does `|` inside quotes not become `PIPE`?

---

## Lexer

15. Do operators require spaces around them?

16. Tokenize:

```bash
cat<input
```

17. Tokenize:

```bash
echo hello>file
```

18. Tokenize:

```bash
echo hello|grep
```

19. Tokenize:

```bash
echo "hello | world"
```

20. Tokenize:

```bash
cat << EOF
```

---

## Parser

21. What should the Parser do with:

```text
WORD
WORD
PIPE
WORD
```

22. What should happen if the input ends after:

```text
REDIR_OUT
```

23. What should happen if the input starts with:

```text
PIPE
```

24. What does:

```text
HEREDOC
WORD
```

represent?

---

# 28. Main Mental Model

The most important model to remember is:

```text
                    RAW INPUT
                        |
                        v
                 +--------------+
                 |    LEXER     |
                 +--------------+
                        |
                        v
                     TOKENS
                        |
          +-------------+-------------+
          |             |             |
          v             v             v
        WORD        OPERATORS        EOF
                      |
          +-----------+-----------+
          |           |           |
          v           v           v
        PIPE        REDIRS      HEREDOC
                      |
             +--------+--------+
             |        |        |
             v        v        v
             <        >       << / >>
             |        |          |
             |        |          |
             +--------+----------+
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
                  EXECUTION
```

---

# The Most Important Concept

For Minishell, keep this separation very clear:

```text
CHARACTERS
     ↓
   LEXER
     ↓
   TOKENS
     ↓
TOKEN TYPES
     ↓
  PARSER
     ↓
COMMAND STRUCTURE
     ↓
  EXPANSION
     ↓
 EXECUTION
```

For example:

```bash
echo hello | grep hello
```

becomes:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("grep")
WORD("hello")
```

But:

```bash
echo "hello | world"
```

becomes:

```text
WORD("echo")
WORD("hello | world")
```

The important difference is the **quote context**.

The character `|` is not automatically a `PIPE`.

Its meaning depends on where it appears.

The main Token Types you need to master are:

```text
WORD
PIPE
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
EOF
```

If you can correctly explain how the Lexer transforms raw shell input into these tokens — including quotes, operators without spaces, and multi-character operators such as `<<` and `>>` — you have understood the **Token Types** stage and are ready to move on to **Parser / Shell Grammar / Command Structure**.
