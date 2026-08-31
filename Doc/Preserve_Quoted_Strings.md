# Preserve Quoted Strings

## Table of Contents

* [1. Task Goal](#1-task-goal)
* [2. What Does "Preserve Quoted Strings" Mean?](#2-what-does-preserve-quoted-strings-mean)
* [3. Why Does the Shell Use Quotes?](#3-why-does-the-shell-use-quotes)
* [4. Two Types of Quotes](#4-two-types-of-quotes)
* [5. Single Quotes `'...'`](#5-single-quotes-)
* [6. Double Quotes `"..."`](#6-double-quotes-)
* [7. Preserving Quoted Content](#7-preserving-quoted-content)
* [8. Quotes and Spaces](#8-quotes-and-spaces)
* [9. Quotes and Operators](#9-quotes-and-operators)
* [10. Quotes Inside a WORD](#10-quotes-inside-a-word)
* [11. Empty Quotes](#11-empty-quotes)
* [12. Mixing Quote Types](#12-mixing-quote-types)
* [13. Unclosed Quotes](#13-unclosed-quotes)
* [14. Lexer Algorithm](#14-lexer-algorithm)
* [15. Code Structure](#15-code-structure)
* [16. Common Mistakes](#16-common-mistakes)
* [17. Testing](#17-testing)
* [18. Final Checklist](#18-final-checklist)

---

# 1. Task Goal

The task:

> **Preserve quoted strings**

means that the Lexer must correctly handle strings inside:

```text
'...'
```

and:

```text
"..."
```

and keep their contents as part of the same `WORD` token.

For example:

```bash
echo "hello world"
```

must NOT become:

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

---

# 2. What Does "Preserve Quoted Strings" Mean?

Consider:

```bash
echo "hello world"
```

Normally, a space separates WORDs:

```bash
hello world
```

becomes:

```text
WORD("hello")
WORD("world")
```

But inside quotes:

```bash
"hello world"
```

the space is part of the string.

Therefore:

```text
"hello world"
```

must be treated as one WORD:

```text
WORD("hello world")
```

---

# 3. Why Does the Shell Use Quotes?

Quotes change how the Shell interprets special characters and spaces.

Without quotes:

```bash
echo hello world
```

`echo` receives two arguments:

```text
hello
world
```

With quotes:

```bash
echo "hello world"
```

`echo` receives one argument:

```text
hello world
```

Therefore, the Lexer must distinguish between:

```text
hello world
```

and:

```text
"hello world"
```

---

# 4. Two Types of Quotes

The Shell mainly uses two types of quotes.

### Single quotes

```text
'...'
```

### Double quotes

```text
"..."
```

They have different rules and must be handled separately.

---

# 5. Single Quotes `'...'`

Single quotes preserve their contents literally.

For example:

```bash
echo 'hello world'
```

should produce:

```text
WORD("echo")
WORD("hello world")
```

---

# 5.1 Spaces Inside Single Quotes

Input:

```bash
echo 'hello     world'
```

Result:

```text
WORD("echo")
WORD("hello     world")
```

All spaces inside the quotes belong to the WORD.

---

# 5.2 Operators Inside Single Quotes

Consider:

```bash
echo 'hello|world'
```

The `|` is NOT a Pipe operator.

Result:

```text
WORD("echo")
WORD("hello|world")
```

Not:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("world")
```

---

# 5.3 Redirection Inside Single Quotes

Input:

```bash
echo 'hello>world'
```

Result:

```text
WORD("echo")
WORD("hello>world")
```

The `>` is ordinary text because it is inside quotes.

---

# 5.4 `<<` Inside Single Quotes

Input:

```bash
echo 'hello<<EOF'
```

Result:

```text
WORD("echo")
WORD("hello<<EOF")
```

`<<` is not a HEREDOC operator here.

---

# 6. Double Quotes `"..."`

Double quotes also prevent spaces and operators from splitting a WORD.

Example:

```bash
echo "hello world"
```

Result:

```text
WORD("echo")
WORD("hello world")
```

---

# 6.1 Operators Inside Double Quotes

Input:

```bash
echo "hello | world"
```

Result:

```text
WORD("echo")
WORD("hello | world")
```

The `|` is not a Pipe.

---

# 6.2 Redirection Inside Double Quotes

Input:

```bash
echo "hello > world"
```

Result:

```text
WORD("echo")
WORD("hello > world")
```

The `>` is ordinary text.

---

# 6.3 Multiple Operators

Input:

```bash
echo "a | b < c > d << e >> f"
```

Result:

```text
WORD("echo")
WORD("a | b < c > d << e >> f")
```

All operators inside the quotes are part of the WORD.

---

# 7. Preserving Quoted Content

It is important to distinguish between:

### Original input

```bash
echo "hello world"
```

### Token structure

```text
WORD
WORD
```

### Value of the second WORD

```text
hello world
```

The quotes themselves are Shell syntax. They are normally not part of the final argument passed to the command.

For example:

```bash
echo "hello world"
```

eventually passes:

```text
hello world
```

not:

```text
"hello world"
```

---

# 7.1 Why Shouldn't We Simply Remove Quotes Immediately?

It is tempting to remove quotes during lexical analysis.

However, doing this too early can make later processing more difficult.

Consider:

```bash
echo "hello"world
```

This is one WORD:

```text
helloworld
```

And:

```bash
echo "hello world"
```

is also one WORD:

```text
hello world
```

The Lexer must therefore preserve enough information about the original structure so that later stages can correctly perform:

* parameter expansion;
* quote removal;
* word construction;
* execution.

Depending on the project architecture, you can:

1. Keep the original quotes in the token value.
2. Remove quotes in a dedicated quote-removal stage.
3. Store additional information about quoted/unquoted parts.

The important rule is:

> **Do not destroy information that later stages still need.**

---

# 8. Quotes and Spaces

This is one of the most important cases.

Without quotes:

```bash
echo hello world
```

produces:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

With quotes:

```bash
echo "hello world"
```

produces:

```text
WORD("echo")
WORD("hello world")
```

---

# 8.1 Quotes Inside a WORD

Consider:

```bash
echo hello" world"
```

This is still one WORD.

Result:

```text
WORD("echo")
WORD("hello world")
```

The space inside the quotes does not terminate the WORD.

---

# 8.2 A WORD Can Contain Multiple Parts

For example:

```bash
echo abc"def"ghi
```

The input contains:

```text
abc
"def"
ghi
```

But all three parts belong to the same WORD:

```text
WORD("abcdefghi")
```

This is a very important concept.

> Quotes do not necessarily surround the entire WORD.

---

# 8.3 Another Example

Input:

```bash
echo "hello"world
```

Result:

```text
WORD("helloworld")
```

And:

```bash
echo hello"world"
```

Result:

```text
WORD("helloworld")
```

---

# 9. Quotes and Operators

Operators should only be recognized when they are **outside quotes**.

For example:

```bash
echo "hello|world"
```

produces:

```text
WORD("echo")
WORD("hello|world")
```

But:

```bash
echo hello|world
```

produces:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("world")
```

---

# 9.1 Comparison

| Input                 | Result                                  |
| --------------------- | --------------------------------------- |
| `echo hello\|world`   | `WORD("hello") PIPE WORD("world")`      |
| `echo "hello\|world"` | `WORD("hello\|world")`                  |
| `echo 'hello\|world'` | `WORD("hello\|world")`                  |
| `echo hello>world`    | `WORD("hello") REDIR_OUT WORD("world")` |
| `echo "hello>world"`  | `WORD("hello>world")`                   |

---

# 10. Quotes Inside a WORD

Quotes can appear in the middle of a WORD.

For example:

```bash
echo hello" world"
```

The Lexer must produce:

```text
WORD("echo")
WORD("hello world")
```

After the closing quote, the Lexer must continue reading the same WORD if the next character is still part of the WORD.

---

# 10.1 Correct Algorithm

Consider:

```text
hello" world"test
```

The Lexer should process:

```text
hello
```

then:

```text
" world"
```

then:

```text
test
```

But the final result is:

```text
WORD("hello worldtest")
```

It is one token.

---

# 10.2 Why Is This Important?

The closing quote does NOT mean:

> "The WORD is finished."

It means:

> "We are leaving quote mode."

If the next character is not a separator or an operator, the WORD continues.

For example:

```bash
echo "hello"world
```

must produce:

```text
WORD("helloworld")
```

---

# 11. Empty Quotes

Empty quotes are valid:

```bash
echo ""
```

and:

```bash
echo ''
```

They represent an empty string.

Conceptually:

```text
WORD("echo")
WORD("")
```

---

# 11.1 Why Are Empty WORDs Important?

Compare:

```bash
echo ""
```

with:

```bash
echo
```

In the first case, `echo` receives an empty argument.

In the second case, there is no argument.

Therefore, the Lexer must not simply ignore:

```text
""
```

or:

```text
''
```

---

# 11.2 Empty Quotes Inside a WORD

Example:

```bash
echo ab""cd
```

Result:

```text
WORD("echo")
WORD("abcd")
```

The empty quoted section is still part of the WORD.

---

# 12. Mixing Quote Types

Single and double quotes can be used in the same WORD.

For example:

```bash
echo "hello"'world'
```

This is one WORD:

```text
WORD("helloworld")
```

---

# 12.1 Another Example

```bash
echo 'hello'"world"
```

Result:

```text
WORD("helloworld")
```

---

# 12.2 Single Quotes Inside Double Quotes

Consider:

```bash
echo "hello 'world'"
```

The single quotes do not start or end a quote mode because we are already inside double quotes.

The value is:

```text
hello 'world'
```

Result:

```text
WORD("hello 'world'")
```

---

# 12.3 Double Quotes Inside Single Quotes

Similarly:

```bash
echo 'hello "world"'
```

The double quotes are ordinary characters.

Result:

```text
WORD("hello \"world\"")
```

The actual value is:

```text
hello "world"
```

---

# 13. Unclosed Quotes

This is another important case.

For example:

```bash
echo "hello
```

or:

```bash
echo 'hello
```

The Lexer detects that a quote was opened but never closed.

Conceptually:

```text
UNCLOSED_DOUBLE_QUOTE
```

or:

```text
UNCLOSED_SINGLE_QUOTE
```

The exact behavior should follow the requirements of your Minishell project.

---

# 13.1 Detecting an Unclosed Quote

When the Lexer sees:

```text
"
```

it enters:

```text
DOUBLE_QUOTE_MODE
```

Then it searches for another:

```text
"
```

If the input ends first:

```text
EOF
```

the quote is unclosed.

---

# 13.2 Example Function

```c
int find_closing_quote(char *input, int *i, char quote)
{
    (*i)++;

    while (input[*i])
    {
        if (input[*i] == quote)
        {
            (*i)++;
            return (1);
        }
        (*i)++;
    }

    return (0);
}
```

If the function returns:

```text
0
```

the quote was not closed.

---

# 14. Lexer Algorithm

The main idea is to distinguish between:

```text
OUTSIDE_QUOTES
```

and:

```text
INSIDE_QUOTES
```

Conceptually:

```text
                 INPUT
                   |
                   v
              current char
                   |
           +-------+-------+
           |               |
         quote           normal
           |               |
           v               v
      enter quote       check:
         mode           space/operator
           |               |
           v               v
     read until        continue WORD
     closing quote
```

---

# 14.1 Main Loop

Pseudo-code:

```text
while input[i]:

    if space:
        skip spaces

    else if operator outside quotes:
        create operator token

    else:
        start WORD

        while current character belongs to WORD:

            if single quote:
                read until closing single quote

            else if double quote:
                read until closing double quote

            else if space:
                stop WORD

            else if operator:
                stop WORD

            else:
                add character

        create WORD token
```

---

# 14.2 Most Important Rule

While reading a WORD, the Lexer must know whether it is:

```text
OUTSIDE QUOTES
```

or:

```text
INSIDE QUOTES
```

Outside quotes:

```text
space → separator
|     → operator
<     → operator
>     → operator
```

Inside quotes:

```text
space → normal character
|     → normal character
<     → normal character
>     → normal character
```

---

# 15. Code Structure

One possible implementation:

```c
void handle_word(char *input, int *i, t_token **tokens)
{
    int start;

    start = *i;

    while (input[*i])
    {
        if (input[*i] == '\'')
            handle_single_quote(input, i);
        else if (input[*i] == '"')
            handle_double_quote(input, i);
        else if (is_space(input[*i]) || is_operator(input[*i]))
            break;
        else
            (*i)++;
    }

    add_word_token(tokens, input, start, *i);
}
```

---

# 15.1 Handling Single Quotes

A simple version:

```c
void handle_single_quote(char *input, int *i)
{
    (*i)++;

    while (input[*i] && input[*i] != '\'')
        (*i)++;

    if (input[*i] == '\'')
        (*i)++;
}
```

While inside single quotes:

```text
space
|
<
>
```

must all be ignored as separators/operators.

---

# 15.2 Handling Double Quotes

```c
void handle_double_quote(char *input, int *i)
{
    (*i)++;

    while (input[*i] && input[*i] != '"')
        (*i)++;

    if (input[*i] == '"')
        (*i)++;
}
```

Later, double quotes will also need to interact correctly with parameter expansion such as:

```text
$USER
$HOME
$?
```

---

# 15.3 Better Architecture

A WORD can be considered a sequence of parts:

```text
WORD
 |
 +-- normal text
 |
 +-- single quoted text
 |
 +-- normal text
 |
 +-- double quoted text
 |
 +-- normal text
```

For example:

```bash
abc"hello"'world'xyz
```

can be represented as:

```text
WORD
 ├── abc
 ├── "hello"
 ├── 'world'
 └── xyz
```

The final value becomes:

```text
abchelloworldxyz
```

---

# 15.4 Why This Helps Minishell

This architecture makes the next stages easier:

```text
Lexer
  ↓
Parser
  ↓
Parameter Expansion
  ↓
Quote Removal
  ↓
Execution
```

For example:

```bash
echo "Hello $USER"
```

The Lexer understands that:

```text
Hello $USER
```

belongs to one WORD.

Later, the Expansion stage can process:

```text
$USER
```

---

# 16. Common Mistakes

## Mistake 1 — Splitting WORDs at spaces inside quotes

Wrong:

```bash
echo "hello world"
```

becomes:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

Correct:

```text
WORD("echo")
WORD("hello world")
```

---

# 16.1 Mistake 2 — Treating `|` Inside Quotes as an Operator

Wrong:

```bash
echo "hello|world"
```

becomes:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("world")
```

Correct:

```text
WORD("echo")
WORD("hello|world")
```

---

# 16.2 Mistake 3 — Treating the Closing Quote as the End of WORD

Wrong:

```bash
echo "hello"world
```

becomes:

```text
WORD("hello")
WORD("world")
```

Correct:

```text
WORD("helloworld")
```

---

# 16.3 Mistake 4 — Ignoring Empty Quotes

Wrong:

```bash
echo ""
```

becomes:

```text
WORD("echo")
```

Correct:

```text
WORD("echo")
WORD("")
```

---

# 16.4 Mistake 5 — Not Detecting Unclosed Quotes

Input:

```bash
echo "hello
```

must not simply be processed as normal text.

The Lexer must detect:

```text
UNCLOSED_DOUBLE_QUOTE
```

and handle it according to the project requirements.

---

# 16.5 Mistake 6 — Treating Quotes as Separate WORDs

Wrong:

```bash
echo "hello world"
```

becomes:

```text
WORD("echo")
QUOTE
WORD("hello world")
QUOTE
```

Unless your architecture explicitly uses quote tokens, quotes should be handled as part of WORD construction rather than becoming independent WORD tokens.

---

# 17. Testing

## Test 1 — Basic Double Quotes

```bash
echo "hello world"
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 2 — Basic Single Quotes

```bash
echo 'hello world'
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 3 — Multiple Spaces

```bash
echo "hello     world"
```

Expected:

```text
WORD("echo")
WORD("hello     world")
```

---

## Test 4 — Pipe Inside Quotes

```bash
echo "hello|world"
```

Expected:

```text
WORD("echo")
WORD("hello|world")
```

---

## Test 5 — Redirection Inside Quotes

```bash
echo "hello>world"
```

Expected:

```text
WORD("echo")
WORD("hello>world")
```

---

## Test 6 — HEREDOC Text Inside Quotes

```bash
echo "hello<<EOF"
```

Expected:

```text
WORD("echo")
WORD("hello<<EOF")
```

---

## Test 7 — Quotes in the Middle of a WORD

```bash
echo hello" world"
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 8 — Quotes Between Normal Characters

```bash
echo abc"def"ghi
```

Expected:

```text
WORD("echo")
WORD("abcdefghi")
```

---

## Test 9 — Mixed Quotes

```bash
echo "hello"'world'
```

Expected:

```text
WORD("echo")
WORD("helloworld")
```

---

## Test 10 — Empty Double Quotes

```bash
echo ""
```

Expected:

```text
WORD("echo")
WORD("")
```

---

## Test 11 — Empty Single Quotes

```bash
echo ''
```

Expected:

```text
WORD("echo")
WORD("")
```

---

## Test 12 — Empty Quotes Inside WORD

```bash
echo ab""cd
```

Expected:

```text
WORD("echo")
WORD("abcd")
```

---

## Test 13 — Single Quotes Inside Double Quotes

```bash
echo "hello 'world'"
```

Expected:

```text
WORD("echo")
WORD("hello 'world'")
```

---

## Test 14 — Double Quotes Inside Single Quotes

```bash
echo 'hello "world"'
```

Expected:

```text
WORD("echo")
WORD("hello \"world\"")
```

The actual value is:

```text
hello "world"
```

---

## Test 15 — Unclosed Double Quote

```bash
echo "hello
```

Expected:

```text
UNCLOSED_DOUBLE_QUOTE
```

---

## Test 16 — Unclosed Single Quote

```bash
echo 'hello
```

Expected:

```text
UNCLOSED_SINGLE_QUOTE
```

---

## Test 17 — Quotes + Operator

```bash
echo "hello" > file
```

Expected:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

## Test 18 — Operator Inside Quotes + Real Operator

```bash
echo "hello > world" > file
```

Expected:

```text
WORD("echo")
WORD("hello > world")
REDIR_OUT(">")
WORD("file")
```

Only the final `>` is an operator.

---

# 17.1 Test Table

| Input                         | Expected result                   |
| ----------------------------- | --------------------------------- |
| `echo "hello world"`          | One WORD for the quoted string    |
| `echo 'hello world'`          | One WORD for the quoted string    |
| `echo "hello\|world"`         | `\|` is not an operator           |
| `echo 'hello\|world'`         | `\|` is not an operator           |
| `echo "hello>world"`          | `>` is not an operator            |
| `echo hello" world"`          | One WORD                          |
| `echo abc"def"ghi`            | One WORD                          |
| `echo "hello"'world'`         | One WORD                          |
| `echo ""`                     | Empty WORD                        |
| `echo ''`                     | Empty WORD                        |
| `echo ab""cd`                 | `WORD("abcd")`                    |
| `echo "hello" > file`         | `WORD + REDIR + WORD`             |
| `echo "hello > world" > file` | Only the final `>` is an operator |

---

# 18. Final Checklist

## Single Quotes

* [ ] `'...'` are correctly recognized.
* [ ] Spaces inside `'...'` are preserved.
* [ ] `|` inside `'...'` is not an operator.
* [ ] `<` inside `'...'` is not an operator.
* [ ] `>` inside `'...'` is not an operator.
* [ ] `<<` inside `'...'` is not an operator.
* [ ] `>>` inside `'...'` is not an operator.

## Double Quotes

* [ ] `"..."` are correctly recognized.
* [ ] Spaces inside `"..."` are preserved.
* [ ] `|` inside `"..."` is not an operator.
* [ ] `<` inside `"..."` is not an operator.
* [ ] `>` inside `"..."` is not an operator.
* [ ] `<<` inside `"..."` is not an operator.
* [ ] `>>` inside `"..."` is not an operator.

## WORD Construction

* [ ] Quotes can appear in the middle of a WORD.
* [ ] WORD continues after closing quotes.
* [ ] `abc"def"ghi` is one WORD.
* [ ] `"abc"'def'` is one WORD.
* [ ] Empty quotes are preserved as an empty part of a WORD.
* [ ] `" "` creates a WORD containing a space.

## Errors

* [ ] Unclosed single quotes are detected.
* [ ] Unclosed double quotes are detected.
* [ ] WORD boundaries are correct.
* [ ] Operators inside quotes are never treated as operators.

## Memory

* [ ] Token strings are allocated correctly.
* [ ] All tokens can be freed.
* [ ] No memory leaks.
* [ ] No invalid memory reads.
* [ ] No buffer overflows.

---

# Main Concept

For the Lexer, it is essential to distinguish between:

```text
OUTSIDE QUOTES
```

and:

```text
INSIDE QUOTES
```

Outside quotes:

```text
space → separator
|     → operator
<     → operator
>     → operator
```

Inside quotes:

```text
space → normal character
|     → normal character
<     → normal character
>     → normal character
```

For example:

```bash
echo hello|wc
```

becomes:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

But:

```bash
echo "hello|wc"
```

becomes:

```text
WORD("echo")
WORD("hello|wc")
```

---

# Most Important Idea

Quotes do **not** necessarily surround an entire WORD.

For example:

```bash
echo abc"hello world"xyz
```

contains:

```text
abc
"hello world"
xyz
```

but these parts form:

```text
ONE WORD
```

with the final value:

```text
abchello worldxyz
```

Therefore, a WORD should conceptually be treated as a sequence of parts:

```text
WORD
 ├── normal text
 ├── single quoted text
 ├── normal text
 └── double quoted text
```

---

# Overall Minishell Architecture

After implementing this stage, the processing pipeline should look approximately like:

```text
                    RAW INPUT
                        |
                        v
                      LEXER
                        |
          +-------------+-------------+
          |             |             |
        spaces        quotes       operators
          |             |             |
          v             v             v
        skip         preserve       tokenize
                        |
                        v
                    WORD TOKENS
                        |
                        v
                      PARSER
                        |
                        v
                PARAMETER EXPANSION
                        |
                        v
                    QUOTE REMOVAL
                        |
                        v
                    EXECUTION
```

The key rule is:

> **The Lexer must preserve quoted strings as part of the same WORD and must not allow spaces or operators inside quotes to split that WORD.**

This is one of the fundamental parts of correct Shell parsing and is essential for building a reliable Minishell Lexer.
