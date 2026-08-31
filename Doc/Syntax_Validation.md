# Understanding Syntax Validation in Minishell

## Table of Contents

* [1. What Is Syntax Validation?](#1-what-is-syntax-validation)
* [2. Where Does Syntax Validation Happen?](#2-where-does-syntax-validation-happen)
* [3. Lexer vs Parser](#3-lexer-vs-parser)
* [4. Shell Grammar](#4-shell-grammar)
* [5. Valid and Invalid Token Sequences](#5-valid-and-invalid-token-sequences)
* [6. Pipe Syntax](#6-pipe-syntax)
* [7. Redirection Syntax](#7-redirection-syntax)
* [8. Heredoc Syntax](#8-heredoc-syntax)
* [9. Multiple Redirections](#9-multiple-redirections)
* [10. Quotes and Syntax Validation](#10-quotes-and-syntax-validation)
* [11. Empty Input](#11-empty-input)
* [12. Whitespace](#12-whitespace)
* [13. Unsupported Operators](#13-unsupported-operators)
* [14. Syntax Error Detection](#14-syntax-error-detection)
* [15. Exit Status](#15-exit-status)
* [16. Parser State Machine](#16-parser-state-machine)
* [17. Syntax Validation Algorithm](#17-syntax-validation-algorithm)
* [18. Examples](#18-examples)
* [19. Common Mistakes](#19-common-mistakes)
* [20. Implementation Structure](#20-implementation-structure)
* [21. Testing Strategy](#21-testing-strategy)
* [22. Checklist](#22-checklist)
* [23. Knowledge Check](#23-knowledge-check)
* [24. Main Mental Model](#24-main-mental-model)

---

# 1. What Is Syntax Validation?

**Syntax validation** means checking whether a sequence of tokens follows the grammar rules of the shell.

For example:

```bash
echo hello | grep hello
```

is syntactically valid.

Its tokens are:

```text
WORD
WORD
PIPE
WORD
WORD
```

But:

```bash
echo hello |
```

is invalid because a `PIPE` must be followed by another command.

Its tokens are:

```text
WORD
WORD
PIPE
```

The Parser must detect this error before execution.

---

# 2. Where Does Syntax Validation Happen?

The general pipeline is:

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
SYNTAX VALIDATION
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

In practice, syntax validation is usually part of the Parser stage.

The Lexer answers:

> "What tokens are present?"

The Parser answers:

> "Are these tokens arranged in a valid shell structure?"

---

# 3. Lexer vs Parser

This distinction is extremely important.

## Lexer

Input:

```bash
echo hello > file
```

Lexer produces:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

The Lexer identifies the pieces.

---

## Parser

The Parser checks whether the sequence makes sense:

```text
WORD WORD REDIR_OUT WORD
```

This is valid.

But:

```bash
echo hello >
```

produces:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
```

The Parser detects:

```text
REDIR_OUT
    ↓
missing WORD
```

Therefore:

```text
Syntax error
```

---

# 4. Shell Grammar

For a basic Minishell, you can think about the grammar like this:

```text
command      → WORD | command WORD | command redirection

redirection  → REDIR_IN WORD
             | REDIR_OUT WORD
             | APPEND WORD
             | HEREDOC WORD

pipeline     → command
             | pipeline PIPE command
```

This is not a complete formal shell grammar, but it is a useful model for Minishell.

---

## Simplified Grammar

```text
COMMAND
   |
   +-- WORD
   |
   +-- WORD
   |
   +-- REDIRECTION
          |
          +-- operator
          +-- WORD
```

A pipeline:

```text
COMMAND PIPE COMMAND
```

For example:

```bash
cat file | grep hello
```

becomes:

```text
COMMAND
   |
   +-- WORD("cat")
   +-- WORD("file")

PIPE

COMMAND
   |
   +-- WORD("grep")
   +-- WORD("hello")
```

---

# 5. Valid and Invalid Token Sequences

The Parser should validate relationships between tokens.

## Valid

```text
WORD
```

Example:

```bash
ls
```

---

```text
WORD WORD
```

Example:

```bash
echo hello
```

---

```text
WORD PIPE WORD
```

Example:

```bash
ls | cat
```

---

```text
WORD REDIR_OUT WORD
```

Example:

```bash
echo hello > file
```

---

```text
WORD REDIR_IN WORD
```

Example:

```bash
cat < file
```

---

```text
WORD APPEND WORD
```

Example:

```bash
echo hello >> file
```

---

```text
WORD HEREDOC WORD
```

Example:

```bash
cat << EOF
```

---

## Invalid

```text
PIPE
```

Example:

```bash
|
```

---

```text
WORD PIPE
```

Example:

```bash
echo hello |
```

---

```text
PIPE WORD
```

Example:

```bash
| echo
```

---

```text
WORD REDIR_OUT
```

Example:

```bash
echo >
```

---

```text
WORD REDIR_IN
```

Example:

```bash
cat <
```

---

```text
WORD HEREDOC
```

Example:

```bash
cat <<
```

---

# 6. Pipe Syntax

The pipe operator:

```text
PIPE
```

connects two commands.

The basic rule is:

```text
COMMAND PIPE COMMAND
```

Therefore:

```bash
ls | grep txt
```

is valid.

---

## 6.1 Pipe at the Beginning

```bash
| ls
```

Tokens:

```text
PIPE
WORD
```

Invalid.

Why?

Because the Parser expects a command before `PIPE`.

Expected:

```text
COMMAND PIPE COMMAND
```

Actual:

```text
PIPE COMMAND
```

---

## 6.2 Pipe at the End

```bash
ls |
```

Tokens:

```text
WORD
PIPE
```

Invalid.

The Parser expects another command after `PIPE`.

---

## 6.3 Two Pipes

```bash
ls | | grep
```

Tokens:

```text
WORD
PIPE
PIPE
WORD
```

Invalid.

After:

```text
PIPE
```

the Parser expects:

```text
WORD
```

or the beginning of a command.

Instead it finds:

```text
PIPE
```

---

## 6.4 Multiple Valid Pipes

This is valid:

```bash
cat file | grep hello | wc -l
```

Tokens:

```text
WORD
WORD
PIPE
WORD
WORD
PIPE
WORD
WORD
```

Conceptually:

```text
cat
 |
grep
 |
wc
```

---

# 7. Redirection Syntax

Every redirection operator must be followed by a `WORD`.

The rules are:

```text
REDIR_IN   → WORD
REDIR_OUT  → WORD
APPEND     → WORD
HEREDOC    → WORD
```

---

# 7.1 Input Redirection

Valid:

```bash
cat < input.txt
```

Tokens:

```text
WORD
REDIR_IN
WORD
```

Invalid:

```bash
cat <
```

Tokens:

```text
WORD
REDIR_IN
```

The Parser expects:

```text
WORD
```

after `<`.

---

# 7.2 Output Redirection

Valid:

```bash
echo hello > output.txt
```

Tokens:

```text
WORD
WORD
REDIR_OUT
WORD
```

Invalid:

```bash
echo hello >
```

Tokens:

```text
WORD
WORD
REDIR_OUT
```

---

# 7.3 Append

Valid:

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD
WORD
APPEND
WORD
```

Invalid:

```bash
echo hello >>
```

Tokens:

```text
WORD
WORD
APPEND
```

---

# 7.4 Heredoc

Valid:

```bash
cat << EOF
```

Tokens:

```text
WORD
HEREDOC
WORD
```

Invalid:

```bash
cat <<
```

Tokens:

```text
WORD
HEREDOC
```

The delimiter is missing.

---

# 8. Heredoc Syntax

A heredoc has this structure:

```text
HEREDOC WORD
```

For:

```bash
cat << EOF
```

the delimiter is:

```text
EOF
```

The shell then reads lines until the delimiter is encountered.

Example:

```bash
cat << EOF
hello
world
EOF
```

The Parser only needs to understand:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

The actual heredoc reading is a later execution/input-handling step.

---

# 9. Multiple Redirections

A command can contain multiple redirections.

For example:

```bash
cat < input > output
```

is valid.

Tokens:

```text
WORD("cat")
REDIR_IN("<")
WORD("input")
REDIR_OUT(">")
WORD("output")
```

Another example:

```bash
echo hello > file1 > file2
```

is syntactically valid.

Tokens:

```text
WORD
WORD
REDIR_OUT
WORD
REDIR_OUT
WORD
```

The Parser should not reject multiple redirections simply because there is more than one.

---

## Multiple Input Redirections

For example:

```bash
cat < file1 < file2
```

is syntactically valid.

The exact behavior is determined later by redirection processing.

This is an important distinction:

> Syntax validation checks whether the structure is legal. It does not decide whether the command's behavior is useful.

---

# 10. Quotes and Syntax Validation

Quotes are handled primarily by the Lexer.

Consider:

```bash
echo "|"
```

The `|` is inside quotes.

Tokens:

```text
WORD
WORD
```

Therefore, there is no pipe.

---

## Example

```bash
echo "hello > world"
```

The `>` is ordinary text.

Tokens:

```text
WORD
WORD
```

This is valid.

---

## Unclosed Quotes

Consider:

```bash
echo "hello
```

There is no closing quote.

The Lexer should detect an unclosed quote.

Conceptually:

```text
LEXER ERROR
    ↓
UNTERMINATED QUOTE
```

The Parser should not attempt to interpret this as a normal command.

---

# 11. Empty Input

When the user presses Enter:

```text
""
```

there are no tokens.

This should normally not produce a syntax error.

Conceptually:

```text
INPUT
 ↓
no tokens
 ↓
do nothing
```

The shell simply displays the prompt again.

---

# 11.1 Only Spaces

Input:

```text
"     "
```

After whitespace removal:

```text
no tokens
```

This should also normally be ignored.

---

# 12. Whitespace

Whitespace separates words, but it is not itself usually represented as a token.

For:

```bash
echo hello world
```

the Lexer creates:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

The spaces disappear during tokenization.

---

## Important

Spaces are not required around operators.

This is valid:

```bash
echo hello>file
```

and becomes:

```text
WORD
WORD
REDIR_OUT
WORD
```

Likewise:

```bash
cat<input
```

becomes:

```text
WORD
REDIR_IN
WORD
```

---

# 13. Unsupported Operators

Minishell does not implement every feature of a full shell.

For the basic Minishell requirements, you generally need:

```text
|
<
>
<<
>>
```

Operators such as:

```text
&&
||
;
```

are not part of the required basic grammar.

Your implementation should handle them consistently with the project's specification rather than accidentally treating them as valid shell syntax.

---

## Example

```bash
echo hello && echo world
```

A robust Minishell implementation should not accidentally execute this as if `&&` were supported.

Depending on your Lexer design, it may:

```text
detect invalid operator
```

or:

```text
produce tokens that the Parser rejects
```

The important requirement is:

> Unsupported syntax must not silently become valid syntax with incorrect behavior.

---

# 14. Syntax Error Detection

The Parser can use simple rules.

## Rule 1 — Pipe must have a command before and after it

```text
COMMAND PIPE COMMAND
```

Therefore reject:

```text
PIPE
```

```text
PIPE COMMAND
```

```text
COMMAND PIPE
```

```text
COMMAND PIPE PIPE COMMAND
```

---

## Rule 2 — Redirection must have a WORD after it

For every:

```text
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
```

the next token must be:

```text
WORD
```

For example:

```text
WORD REDIR_OUT WORD
```

is valid.

But:

```text
WORD REDIR_OUT PIPE
```

is invalid.

---

# 14.1 Example

Input:

```bash
echo hello > | grep
```

Tokens:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
PIPE("|")
WORD("grep")
```

The Parser sees:

```text
REDIR_OUT
    ↓
expected WORD
    ↓
found PIPE
```

Therefore:

```text
Syntax error
```

---

# 14.2 Another Example

Input:

```bash
echo hello | >
```

Tokens:

```text
WORD
WORD
PIPE
REDIR_OUT
```

After `PIPE`, the Parser expects a command.

Instead it sees a redirection.

Depending on the grammar implementation, the Parser should continue according to the command grammar and ultimately reject the incomplete structure because `>` also requires a following `WORD`.

The important principle is:

> Validate the sequence of token types, not just individual characters.

---

# 15. Exit Status

When a syntax error occurs, Minishell should return a non-zero exit status.

For Bash-like behavior, syntax errors are commonly associated with:

```text
exit status = 2
```

For example:

```bash
echo hello |
```

should result in a syntax error and a non-zero status.

Your implementation should maintain the expected `$?` behavior required by the project.

---

# 15.1 Why Exit Status Matters

The shell provides the previous command's status through:

```bash
echo $?
```

For example:

```bash
echo hello |
```

produces a syntax error.

Then:

```bash
echo $?
```

should report the appropriate syntax-error status.

This means syntax validation is not just about printing an error.

It must also correctly update the shell's state.

---

# 16. Parser State Machine

A very useful way to design syntax validation is with parser states.

For example:

```text
EXPECT_COMMAND
EXPECT_WORD
```

---

## State 1 — EXPECT_COMMAND

At the beginning of input, the Parser expects:

```text
WORD
```

A command can start with a `WORD`.

For example:

```bash
ls
```

is valid.

---

## State 2 — After WORD

After a `WORD`, the Parser can see:

```text
WORD
PIPE
REDIRECTION
EOF
```

For example:

```text
echo hello
```

or:

```text
echo hello | grep
```

or:

```text
echo hello > file
```

---

## State 3 — After PIPE

After:

```text
PIPE
```

the Parser expects a new command.

Usually:

```text
WORD
```

So:

```text
ls | cat
```

is valid.

But:

```text
ls | |
```

is invalid.

---

## State 4 — After REDIRECTION

After:

```text
REDIR_IN
REDIR_OUT
APPEND
HEREDOC
```

the Parser expects:

```text
WORD
```

Examples:

```text
REDIR_OUT WORD
```

```text
HEREDOC WORD
```

---

# 16.1 State Diagram

A simplified model:

```text
             +----------------+
             | EXPECT_COMMAND |
             +----------------+
                     |
                    WORD
                     |
                     v
             +----------------+
             | EXPECT_ELEMENT |
             +----------------+
              /      |       \
             /       |        \
          WORD      PIPE     REDIR
           |         |          |
           |         |          v
           |         |      EXPECT_WORD
           |         |          |
           |         |         WORD
           |         |          |
           |         |          v
           |         +------> EXPECT_ELEMENT
           |
           +--------------------------+
```

At the end:

```text
EXPECT_ELEMENT + EOF
```

can be valid.

But:

```text
EXPECT_COMMAND + EOF
```

may represent an empty command depending on where the parser is.

---

# 17. Syntax Validation Algorithm

A simple algorithm:

```text
1. Get the first token.

2. If there are no tokens:
       return success.

3. If the first token is PIPE:
       syntax error.

4. While tokens remain:

       If current token is WORD:
           move to next token.

       If current token is REDIR_IN:
           next token must be WORD.

       If current token is REDIR_OUT:
           next token must be WORD.

       If current token is APPEND:
           next token must be WORD.

       If current token is HEREDOC:
           next token must be WORD.

       If current token is PIPE:
           next token must begin a command.

5. If the last token is PIPE:
       syntax error.

6. If a redirection is the last token:
       syntax error.

7. Otherwise:
       syntax is valid.
```

---

# 17.1 Pseudocode

A simplified implementation could look like:

```c
int validate_syntax(t_token *tokens)
{
    t_token *current;

    current = tokens;

    if (!current)
        return (0);

    if (current->type == TOKEN_PIPE)
        return (syntax_error(current));

    while (current)
    {
        if (is_redirection(current->type))
        {
            if (!current->next
                || current->next->type != TOKEN_WORD)
                return (syntax_error(current));
        }

        if (current->type == TOKEN_PIPE)
        {
            if (!current->next
                || current->next->type == TOKEN_PIPE)
                return (syntax_error(current));
        }

        current = current->next;
    }

    return (0);
}
```

This is only a simplified example.

Your actual parser will probably be more structured.

---

# 18. Examples

## Valid Example 1

```bash
echo hello
```

Tokens:

```text
WORD
WORD
EOF
```

Result:

```text
VALID
```

---

## Valid Example 2

```bash
ls | grep txt
```

Tokens:

```text
WORD
PIPE
WORD
WORD
EOF
```

Result:

```text
VALID
```

---

## Valid Example 3

```bash
cat < input.txt
```

Tokens:

```text
WORD
REDIR_IN
WORD
EOF
```

Result:

```text
VALID
```

---

## Valid Example 4

```bash
echo hello > output.txt
```

Tokens:

```text
WORD
WORD
REDIR_OUT
WORD
EOF
```

Result:

```text
VALID
```

---

## Valid Example 5

```bash
echo hello >> output.txt
```

Tokens:

```text
WORD
WORD
APPEND
WORD
EOF
```

Result:

```text
VALID
```

---

## Valid Example 6

```bash
cat << EOF
```

Tokens:

```text
WORD
HEREDOC
WORD
EOF
```

Result:

```text
VALID
```

---

# 18.1 Invalid Examples

### Pipe at beginning

```bash
| echo hello
```

Tokens:

```text
PIPE
WORD
WORD
```

Result:

```text
SYNTAX ERROR
```

---

### Pipe at end

```bash
echo hello |
```

Tokens:

```text
WORD
WORD
PIPE
```

Result:

```text
SYNTAX ERROR
```

---

### Double pipe

```bash
echo hello | | grep
```

Tokens:

```text
WORD
WORD
PIPE
PIPE
WORD
```

Result:

```text
SYNTAX ERROR
```

---

### Missing output file

```bash
echo hello >
```

Tokens:

```text
WORD
WORD
REDIR_OUT
```

Result:

```text
SYNTAX ERROR
```

---

### Missing input file

```bash
cat <
```

Tokens:

```text
WORD
REDIR_IN
```

Result:

```text
SYNTAX ERROR
```

---

### Missing heredoc delimiter

```bash
cat <<
```

Tokens:

```text
WORD
HEREDOC
```

Result:

```text
SYNTAX ERROR
```

---

### Redirection followed by pipe

```bash
echo hello > | cat
```

Tokens:

```text
WORD
WORD
REDIR_OUT
PIPE
WORD
```

Result:

```text
SYNTAX ERROR
```

---

# 18.2 Complex Valid Example

```bash
cat < input.txt | grep "hello world" >> result.txt
```

Tokens:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")

PIPE

WORD("grep")
WORD("hello world")

APPEND(">>")
WORD("result.txt")
```

The structure is:

```text
             PIPE
            /    \
           /      \
        cat        grep
         |           |
      < input     "hello world"
                       |
                    >> result.txt
```

Result:

```text
VALID
```

---

# 19. Common Mistakes

## Mistake 1 — Validating Characters Instead of Tokens

Do not write syntax validation only around characters.

For example:

```text
if input[i] == '|'
```

is not enough.

The Lexer has already determined whether the character is actually an operator.

The Parser should work with:

```text
TOKEN_PIPE
```

rather than raw characters.

---

# Mistake 2 — Treating Quotes as Parser Errors

For:

```bash
echo "|"
```

the `|` is a `WORD`.

The Parser should not see a `PIPE`.

Quote processing belongs primarily to lexical analysis.

---

# Mistake 3 — Forgetting Redirection Targets

This is invalid:

```bash
echo hello >
```

because:

```text
REDIR_OUT
```

requires:

```text
WORD
```

after it.

---

# Mistake 4 — Rejecting Multiple Redirections

This can be valid:

```bash
cat < input > output
```

Do not reject it simply because there are multiple redirections.

---

# Mistake 5 — Rejecting Operators Without Spaces

This is valid:

```bash
echo hello>file
```

The Parser should receive:

```text
WORD
WORD
REDIR_OUT
WORD
```

---

# Mistake 6 — Confusing Syntax Errors with Execution Errors

Consider:

```bash
cat < missing_file
```

This is syntactically valid.

The file may not exist, but that is an execution/redirection error, not a syntax error.

Compare:

```bash
cat <
```

This is a syntax error because the redirection has no target.

Important distinction:

```text
Syntax error
    ↓
Parser

File does not exist
    ↓
Redirection / execution

Command not found
    ↓
Execution
```

---

# 20. Implementation Structure

A clean Minishell architecture could look like:

```text
src/
├── lexer/
│   ├── lexer.c
│   ├── token.c
│   ├── quotes.c
│   └── operators.c
│
├── parser/
│   ├── parser.c
│   ├── syntax.c
│   ├── command.c
│   └── redirection.c
│
├── expansion/
│   └── ...
│
├── execution/
│   └── ...
│
└── main.c
```

Possible functions:

```c
int     validate_syntax(t_token *tokens);
int     is_redirection(int type);
int     is_pipe(int type);
int     expect_word(t_token *token);
int     syntax_error(t_token *token);
```

---

# 20.1 Useful Helper

A useful helper:

```c
int is_redirection(int type)
{
    return (
        type == TOKEN_REDIR_IN
        || type == TOKEN_REDIR_OUT
        || type == TOKEN_APPEND
        || type == TOKEN_HEREDOC
    );
}
```

Then:

```c
if (is_redirection(current->type))
{
    if (!current->next
        || current->next->type != TOKEN_WORD)
        return (syntax_error(current));
}
```

This keeps the Parser clean.

---

# 21. Testing Strategy

Do not test only simple commands.

Create groups of tests.

## Group 1 — Basic

```bash
echo
echo hello
ls
pwd
```

---

## Group 2 — Pipes

```bash
ls | cat
ls | grep txt
cat file | grep hello | wc -l
```

---

## Group 3 — Invalid Pipes

```bash
|
| ls
ls |
ls | |
ls | | cat
```

---

## Group 4 — Redirections

```bash
cat < file
echo hello > file
echo hello >> file
cat << EOF
```

---

## Group 5 — Invalid Redirections

```bash
cat <
echo >
echo >>
cat <<
```

---

## Group 6 — Redirection + Pipe

```bash
cat < file | grep hello
cat file | grep hello > result
cat < input | grep hello >> output
```

---

## Group 7 — Quotes

```bash
echo "|"
echo "<"
echo ">"
echo "hello | world"
echo 'hello > world'
```

These should not accidentally create operator tokens.

---

## Group 8 — Operators Without Spaces

```bash
cat<file
cat>file
cat>>file
cat<<EOF
echo hello|grep hello
```

---

# 21.1 Compare With Bash

A very useful testing technique is to compare Minishell with Bash.

For each test:

```text
Input
  ↓
Bash
  ↓
Expected behavior

Input
  ↓
Minishell
  ↓
Your behavior
```

Compare:

* error message behavior
* exit status
* whether execution happens
* tokenization
* redirection behavior

For syntax-specific tests, `$?` is particularly useful.

---

# 22. Checklist

Before considering **Syntax Validation** understood, you should know:

## Basic

* [ ] What syntax validation is.
* [ ] Why syntax validation is needed.
* [ ] Why the Parser is responsible for syntax validation.
* [ ] Difference between Lexer and Parser.
* [ ] What a valid token sequence looks like.
* [ ] What an invalid token sequence looks like.

---

## Pipes

* [ ] Pipe cannot start a command.
* [ ] Pipe cannot end a command.
* [ ] Two consecutive pipes are invalid for the basic Minishell grammar.
* [ ] Pipe separates two commands.
* [ ] Multiple valid pipes are possible.

---

## Redirections

* [ ] `<` requires a `WORD` after it.
* [ ] `>` requires a `WORD` after it.
* [ ] `>>` requires a `WORD` after it.
* [ ] `<<` requires a `WORD` delimiter after it.
* [ ] Multiple redirections can be valid.
* [ ] Redirections do not require spaces.

---

## Quotes

* [ ] Operators inside quotes are `WORD` content.
* [ ] Quotes are handled during lexical analysis.
* [ ] Unclosed quotes must be detected.
* [ ] Quoted spaces do not separate words.

---

## Errors

* [ ] Understand syntax errors.
* [ ] Understand execution errors.
* [ ] Understand command-not-found errors.
* [ ] Understand the role of exit status.
* [ ] Know that syntax errors should prevent execution.

---

# 23. Knowledge Check

Try answering these without looking at the previous sections.

### Basic

1. What is syntax validation?
2. Why does Minishell need syntax validation?
3. What is the difference between Lexer and Parser?
4. Which component should detect `echo hello >`?
5. Why?

---

### Pipes

6. Is this valid?

```bash
ls | cat
```

7. Is this valid?

```bash
| ls
```

8. Is this valid?

```bash
ls |
```

9. Is this valid?

```bash
ls | | cat
```

10. Why?

---

### Redirection

11. Is this valid?

```bash
cat < input
```

12. Is this valid?

```bash
cat <
```

13. Is this valid?

```bash
echo hello > output
```

14. Is this valid?

```bash
echo hello >
```

15. Is this valid?

```bash
echo hello >> output
```

16. Is this valid?

```bash
cat << EOF
```

17. What does `EOF` represent?

---

### Quotes

18. What tokens should be produced by:

```bash
echo "|"
```

19. Is the `|` a pipe?

20. What about:

```bash
echo "hello > world"
```

21. Why does the `>` not become `REDIR_OUT`?

---

### Syntax vs Execution

22. Is this a syntax error?

```bash
cat < missing_file
```

23. Is this a syntax error?

```bash
cat <
```

24. What is the difference between these two cases?

---

# 24. Main Mental Model

Remember this pipeline:

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
                        v
              +-------------------+
              | SYNTAX VALIDATION |
              +-------------------+
                        |
                Valid? | Invalid?
                    /        \
                   /          \
                  v            v
              PARSER       SYNTAX ERROR
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

The key difference is:

```text
LEXER
"What are these pieces?"

PARSER
"Do these pieces form a valid command?"

EXECUTOR
"How do I actually run this command?"
```

---

# The Most Important Rules

For a basic Minishell, remember these four rules.

## Rule 1 — A command starts with WORD

```text
WORD
```

is the basic beginning of a command.

---

## Rule 2 — PIPE connects commands

```text
COMMAND PIPE COMMAND
```

Therefore:

```text
PIPE
```

at the beginning or end is invalid.

---

## Rule 3 — Redirections require WORD

```text
REDIR_IN  WORD
REDIR_OUT WORD
APPEND    WORD
HEREDOC   WORD
```

Therefore:

```text
echo >
```

is invalid.

---

## Rule 4 — Syntax is different from execution

This:

```bash
cat < missing_file
```

is syntactically correct.

This:

```bash
cat <
```

is syntactically incorrect.

The first problem happens during redirection/execution.

The second problem happens during parsing.

---

# Final Architecture

Your Minishell should conceptually work like this:

```text
USER INPUT
    |
    v
+----------+
|  LEXER   |
+----------+
    |
    | creates
    v
+----------+
|  TOKENS  |
+----------+
    |
    | validates
    v
+----------+
|  PARSER  |
+----------+
    |
    | creates
    v
+------------------+
| COMMAND STRUCTURE|
+------------------+
    |
    v
+-------------+
| EXPANSION   |
+-------------+
    |
    v
+-------------+
| REDIRECTIONS|
+-------------+
    |
    v
+-------------+
| EXECUTION   |
+-------------+
```

The most important concept is:

> **Syntax validation prevents malformed token sequences from reaching the execution stage.**

For example:

```bash
echo hello |
```

must stop here:

```text
Lexer
  ↓
Tokens
  ↓
Parser
  ↓
SYNTAX ERROR
```

It must **never** reach:

```text
Execution
```

with an incomplete command.

Once you understand how to validate `PIPE`, `REDIR_IN`, `REDIR_OUT`, `APPEND`, `HEREDOC`, quotes, missing operands, and invalid token sequences, you are ready for the next major Minishell topic: **building the command structure / AST from the validated tokens**.
