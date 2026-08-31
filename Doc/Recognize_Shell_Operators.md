# Recognize Shell Operators

## Table of Contents

* [1. Goal of the Task](#1-goal-of-the-task)
* [2. What Is a Shell Operator?](#2-what-is-a-shell-operator)
* [3. Operators Required for Minishell](#3-operators-required-for-minishell)
* [4. Operator Recognition Rules](#4-operator-recognition-rules)
* [5. Single-Character Operators](#5-single-character-operators)
* [6. Multi-Character Operators](#6-multi-character-operators)
* [7. Pipe Operator `|`](#7-pipe-operator-)
* [8. Input Redirection `<`](#8-input-redirection-)
* [9. Output Redirection `>`](#9-output-redirection-)
* [10. Here-Document `<<`](#10-here-document-)
* [11. Append `>>`](#11-append-)
* [12. Operators Without Spaces](#12-operators-without-spaces)
* [13. Operators Inside Quotes](#13-operators-inside-quotes)
* [14. Operator Priority in the Lexer](#14-operator-priority-in-the-lexer)
* [15. Implementation](#15-implementation)
* [16. Common Mistakes](#16-common-mistakes)
* [17. Testing](#17-testing)
* [18. Final Checklist](#18-final-checklist)

---

# 1. Goal of the Task

The task:

> **Recognize shell operators**

means that the Lexer must detect special Shell characters and convert them into the appropriate token types.

For Minishell, the important operators are:

```text
|
<
>
<<
>>
```

For example:

```bash
echo hello | wc
```

must become:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

# 2. What Is a Shell Operator?

A Shell operator is a character or sequence of characters that has a special meaning for the Shell.

Operators are not ordinary text.

For example:

```bash
echo hello > output.txt
```

contains:

```text
echo
hello
>
output.txt
```

The `>` is not part of the filename or the previous WORD.

It tells the Shell:

> Redirect standard output to a file.

Therefore, the Lexer must create a special token for it.

---

# 3. Operators Required for Minishell

For the standard 42 Minishell project, the main operators are:

| Operator | Meaning            | Token             |              |
| -------- | ------------------ | ----------------- | ------------ |
| `        | `                  | Pipe              | `TOKEN_PIPE` |
| `<`      | Input redirection  | `TOKEN_REDIR_IN`  |              |
| `>`      | Output redirection | `TOKEN_REDIR_OUT` |              |
| `<<`     | Here-document      | `TOKEN_HEREDOC`   |              |
| `>>`     | Append output      | `TOKEN_APPEND`    |              |

A possible enum:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_HEREDOC,
    TOKEN_APPEND
}   t_token_type;
```

---

# 4. Operator Recognition Rules

The Lexer needs to answer:

> "Is the current character the beginning of an operator?"

For example:

```bash
echo hello|wc
```

When the Lexer reaches:

```text
|
```

it must stop reading the current WORD.

The result:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

# 4.1 Operators Are Delimiters

Operators terminate a WORD.

For example:

```bash
hello>file
```

must become:

```text
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

The operator does not need spaces around it.

---

# 4.2 Spaces Are Not Required

All of these are valid:

```bash
echo hello | wc
```

```bash
echo hello|wc
```

```bash
echo hello |wc
```

```bash
echo hello| wc
```

They should all produce:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

# 5. Single-Character Operators

The simplest operators are:

```text
|
<
>
```

Each one represents a separate token.

For example:

```bash
cat < input.txt
```

becomes:

```text
WORD("cat")
TOKEN_REDIR_IN("<")
WORD("input.txt")
```

And:

```bash
echo hello > output.txt
```

becomes:

```text
WORD("echo")
WORD("hello")
TOKEN_REDIR_OUT(">")
WORD("output.txt")
```

---

# 5.1 Recognizing `|`

If:

```c
input[i] == '|'
```

create:

```text
TOKEN_PIPE
```

Then move forward:

```c
i++;
```

Example:

```bash
ls|wc
```

Result:

```text
WORD("ls")
PIPE("|")
WORD("wc")
```

---

# 5.2 Recognizing `<`

If:

```c
input[i] == '<'
```

check whether the next character is also `<`.

If not:

```text
<
```

create:

```text
TOKEN_REDIR_IN
```

---

# 5.3 Recognizing `>`

Similarly, if:

```c
input[i] == '>'
```

check whether the next character is also `>`.

If not:

```text
>
```

create:

```text
TOKEN_REDIR_OUT
```

---

# 6. Multi-Character Operators

Two operators contain two characters:

```text
<<
>>
```

These are called multi-character operators.

The Lexer must check the next character.

For example:

```bash
cat << EOF
```

must become:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

# 6.1 Why Order Matters

Consider:

```bash
<<
```

If you check:

```c
if (input[i] == '<')
```

first, you might incorrectly create:

```text
TOKEN_REDIR_IN("<")
TOKEN_REDIR_IN("<")
```

Instead, check the two-character operator first:

```text
if current == '<' and next == '<'
    HEREDOC
else if current == '<'
    REDIR_IN
```

The same applies to `>>`.

---

# 6.2 Longest Match Principle

The general Lexer rule is:

> **When multiple operators begin with the same character, recognize the longest valid operator first.**

For `<`:

```text
<<
<
```

Check:

```text
<<
```

before:

```text
<
```

For `>`:

```text
>>
>
```

Check:

```text
>>
```

before:

```text
>
```

This principle is called:

> **Longest match**

---

# 7. Pipe Operator `|`

The pipe connects the output of one command to the input of another.

Example:

```bash
ls | wc
```

Lexer result:

```text
WORD("ls")
PIPE("|")
WORD("wc")
```

---

# 7.1 Multiple Pipes

Input:

```bash
ls | grep txt | wc
```

Result:

```text
WORD("ls")
PIPE("|")
WORD("grep")
WORD("txt")
PIPE("|")
WORD("wc")
```

The Parser will later determine whether the pipe placement is syntactically valid.

---

# 7.2 Pipe Without Spaces

Input:

```bash
ls|grep|wc
```

Result:

```text
WORD("ls")
PIPE("|")
WORD("grep")
PIPE("|")
WORD("wc")
```

Spaces are irrelevant for operator recognition.

---

# 8. Input Redirection `<`

Example:

```bash
cat < input.txt
```

Tokens:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

The Parser will later interpret:

```text
<
```

as input redirection.

The Lexer only needs to identify the operator.

---

# 8.1 No Spaces

Input:

```bash
cat<input.txt
```

Result:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

# 8.2 With Multiple Spaces

Input:

```bash
cat     <     input.txt
```

Result:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

# 9. Output Redirection `>`

Example:

```bash
echo hello > output.txt
```

Result:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

# 9.1 No Spaces

Input:

```bash
echo hello>output.txt
```

Result:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

# 9.2 Multiple Spaces

Input:

```bash
echo hello     >     output.txt
```

Result:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

---

# 10. Here-Document `<<`

Here-document syntax:

```bash
cat << EOF
hello
EOF
```

The Lexer only processes the command line:

```bash
cat << EOF
```

and produces:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

The actual here-document processing happens later during parsing/execution.

---

# 10.1 Recognizing `<<`

Algorithm:

```text
current == '<'
        |
        v
next == '<' ?
   /          \
 yes           no
 |              |
 v              v
HEREDOC       REDIR_IN
```

C:

```c
if (input[i] == '<' && input[i + 1] == '<')
{
    add_token(..., TOKEN_HEREDOC, "<<");
    i += 2;
}
else if (input[i] == '<')
{
    add_token(..., TOKEN_REDIR_IN, "<");
    i++;
}
```

---

# 11. Append `>>`

Append redirection:

```bash
echo hello >> output.txt
```

Result:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("output.txt")
```

The difference between:

```text
>
```

and:

```text
>>
```

will be handled during execution.

The Lexer only identifies the token type.

---

# 11.1 Recognizing `>>`

Algorithm:

```text
current == '>'
        |
        v
next == '>' ?
   /          \
 yes           no
 |              |
 v              v
APPEND       REDIR_OUT
```

C:

```c
if (input[i] == '>' && input[i + 1] == '>')
{
    add_token(..., TOKEN_APPEND, ">>");
    i += 2;
}
else if (input[i] == '>')
{
    add_token(..., TOKEN_REDIR_OUT, ">");
    i++;
}
```

---

# 12. Operators Without Spaces

This is extremely important.

The Lexer must recognize operators even when they touch WORDs.

Examples:

```bash
echo hello|wc
```

```bash
cat<input.txt
```

```bash
echo hello>file
```

```bash
cat<<EOF
```

```bash
echo hello>>file
```

Expected:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("file")
```

---

# 12.1 Operator Ends the Current WORD

Suppose the input is:

```bash
hello>world
```

The Lexer starts reading:

```text
hello
```

Then it encounters:

```text
>
```

Therefore:

```text
WORD("hello")
```

is completed.

Then:

```text
>
```

becomes:

```text
TOKEN_REDIR_OUT
```

Then:

```text
world
```

becomes:

```text
WORD("world")
```

Final result:

```text
WORD("hello")
REDIR_OUT(">")
WORD("world")
```

---

# 13. Operators Inside Quotes

Operators inside quotes are NOT operators.

This is critical.

For example:

```bash
echo "|"
```

The `|` is inside double quotes.

It belongs to the WORD.

Expected:

```text
WORD("echo")
WORD("|")
```

There should NOT be:

```text
WORD("echo")
PIPE("|")
```

---

# 13.1 `<` Inside Quotes

Input:

```bash
echo "<"
```

Expected:

```text
WORD("echo")
WORD("<")
```

Not:

```text
WORD("echo")
REDIR_IN("<")
```

---

# 13.2 `>` Inside Quotes

Input:

```bash
echo ">"
```

Expected:

```text
WORD("echo")
WORD(">")
```

---

# 13.3 `<<` Inside Quotes

Input:

```bash
echo "<<"
```

Expected:

```text
WORD("echo")
WORD("<<")
```

---

# 13.4 `>>` Inside Quotes

Input:

```bash
echo ">>"
```

Expected:

```text
WORD("echo")
WORD(">>")
```

---

# 13.5 Operators Inside Single Quotes

The same rule applies:

```bash
echo '|'
```

Expected:

```text
WORD("echo")
WORD("|")
```

And:

```bash
echo '<<'
```

Expected:

```text
WORD("echo")
WORD("<<")
```

Quotes change how the Lexer interprets special characters.

---

# 14. Operator Priority in the Lexer

A good Lexer loop should follow this order:

```text
1. Skip spaces
       |
       v
2. Check operators
       |
       v
3. Read WORD
```

But when reading a WORD, operators must also terminate it.

Conceptually:

```text
while input[i]

    skip spaces

    if operator
        read operator

    else
        read WORD
```

And inside `read WORD`:

```text
while input[i]

    if space outside quotes
        stop

    if operator outside quotes
        stop

    if quote
        process quote
        continue

    otherwise
        add character

end
```

---

# 14.1 Recommended Decision Tree

```text
                input[i]
                   |
                   v
              whitespace?
              /         \
            yes          no
             |            |
          skip it         v
                       operator?
                       /       \
                     yes        no
                      |          |
                read operator  read WORD
```

When checking operators:

```text
             input[i]
                |
                v
         '<' or '>' ?
          /          \
        yes           no
         |             |
         v             v
     next same?      '|'
      /     \          |
    yes      no        PIPE
     |        |
     v        v
  << / >>    < / >
```

---

# 15. Implementation

## 15.1 `is_operator()`

A basic function:

```c
int is_operator(char c)
{
    return (c == '|' || c == '<' || c == '>');
}
```

This only answers whether the first character can begin an operator.

---

# 15.2 Operator Token Type

Define:

```c
typedef enum e_token_type
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_HEREDOC,
    TOKEN_APPEND
}   t_token_type;
```

---

# 15.3 Operator Handler

A useful structure:

```c
void handle_operator(char *input, int *i, t_token **tokens)
{
    if (input[*i] == '|')
    {
        add_token(tokens, "|", TOKEN_PIPE);
        (*i)++;
    }
    else if (input[*i] == '<' && input[*i + 1] == '<')
    {
        add_token(tokens, "<<", TOKEN_HEREDOC);
        (*i) += 2;
    }
    else if (input[*i] == '>' && input[*i + 1] == '>')
    {
        add_token(tokens, ">>", TOKEN_APPEND);
        (*i) += 2;
    }
    else if (input[*i] == '<')
    {
        add_token(tokens, "<", TOKEN_REDIR_IN);
        (*i)++;
    }
    else if (input[*i] == '>')
    {
        add_token(tokens, ">", TOKEN_REDIR_OUT);
        (*i)++;
    }
}
```

---

# 15.4 Better Structure

For a larger project, separate detection from token creation:

```c
t_token_type get_operator_type(char *input, int i)
{
    if (input[i] == '|')
        return (TOKEN_PIPE);

    if (input[i] == '<' && input[i + 1] == '<')
        return (TOKEN_HEREDOC);

    if (input[i] == '>' && input[i + 1] == '>')
        return (TOKEN_APPEND);

    if (input[i] == '<')
        return (TOKEN_REDIR_IN);

    if (input[i] == '>')
        return (TOKEN_REDIR_OUT);

    return (TOKEN_WORD);
}
```

Then:

```c
void handle_operator(char *input, int *i, t_token **tokens)
{
    t_token_type type;

    type = get_operator_type(input, *i);

    if (type == TOKEN_HEREDOC || type == TOKEN_APPEND)
    {
        add_operator_token(...);
        *i += 2;
    }
    else
    {
        add_operator_token(...);
        (*i)++;
    }
}
```

This can make the Lexer easier to maintain.

---

# 15.5 Main Lexer

A basic structure:

```c
t_token *lexer(char *input)
{
    t_token *tokens;
    int     i;

    tokens = NULL;
    i = 0;

    while (input[i])
    {
        while (input[i] && is_space(input[i]))
            i++;

        if (!input[i])
            break;

        if (is_operator(input[i]))
            handle_operator(input, &i, &tokens);
        else
            handle_word(input, &i, &tokens);
    }

    return (tokens);
}
```

---

# 15.6 `handle_word()`

Remember that operators must stop WORD parsing:

```c
void handle_word(char *input, int *i, t_token **tokens)
{
    int start;

    start = *i;

    while (input[*i])
    {
        if (is_space(input[*i]))
            break;

        if (is_operator(input[*i]))
            break;

        if (input[*i] == '\'')
            skip_single_quote(input, i);
        else if (input[*i] == '"')
            skip_double_quote(input, i);
        else
            (*i)++;
    }

    add_word_token(tokens, input, start, *i);
}
```

The quote functions must ensure that operators inside quotes are ignored.

---

# 16. Common Mistakes

## Mistake 1 — Checking `<` before `<<`

Incorrect:

```c
if (input[i] == '<')
    TOKEN_REDIR_IN;
else if (input[i] == '<' && input[i + 1] == '<')
    TOKEN_HEREDOC;
```

The `<<` condition will never be reached.

Correct:

```c
if (input[i] == '<' && input[i + 1] == '<')
    TOKEN_HEREDOC;
else if (input[i] == '<')
    TOKEN_REDIR_IN;
```

Always check the longer operator first.

---

# 16.1 Mistake 2 — Same Problem With `>>`

Incorrect:

```c
if (input[i] == '>')
    TOKEN_REDIR_OUT;
else if (input[i] == '>' && input[i + 1] == '>')
    TOKEN_APPEND;
```

Correct:

```c
if (input[i] == '>' && input[i + 1] == '>')
    TOKEN_APPEND;
else if (input[i] == '>')
    TOKEN_REDIR_OUT;
```

---

# 16.2 Mistake 3 — Treating Operators Inside Quotes as Operators

Input:

```bash
echo "hello|world"
```

Incorrect:

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

# 16.3 Mistake 4 — Requiring Spaces Around Operators

Incorrect assumption:

```text
echo hello | wc
```

works, but:

```text
echo hello|wc
```

does not.

This is wrong.

Operators do not require spaces.

---

# 16.4 Mistake 5 — Making `<<` Two Tokens

Input:

```bash
cat << EOF
```

Incorrect:

```text
WORD("cat")
REDIR_IN("<")
REDIR_IN("<")
WORD("EOF")
```

Correct:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

# 16.5 Mistake 6 — Letting the Operator Become Part of WORD

Input:

```bash
hello>world
```

Incorrect:

```text
WORD("hello>world")
```

Correct:

```text
WORD("hello")
REDIR_OUT(">")
WORD("world")
```

---

# 17. Testing

You should test operators both with and without spaces.

---

## Test 1 — Pipe

```bash
echo hello | wc
```

Expected:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

## Test 2 — Pipe Without Spaces

```bash
echo hello|wc
```

Expected:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

## Test 3 — Input Redirection

```bash
cat < input.txt
```

Expected:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

## Test 4 — Input Redirection Without Spaces

```bash
cat<input.txt
```

Expected:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

## Test 5 — Output Redirection

```bash
echo hello > file
```

Expected:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

## Test 6 — Output Redirection Without Spaces

```bash
echo hello>file
```

Expected:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("file")
```

---

## Test 7 — Here-Document

```bash
cat << EOF
```

Expected:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

## Test 8 — Here-Document Without Spaces

```bash
cat<<EOF
```

Expected:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
```

---

## Test 9 — Append

```bash
echo hello >> file
```

Expected:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("file")
```

---

## Test 10 — Append Without Spaces

```bash
echo hello>>file
```

Expected:

```text
WORD("echo")
WORD("hello")
APPEND(">>")
WORD("file")
```

---

## Test 11 — Operators Inside Double Quotes

```bash
echo "| < > << >>"
```

Expected:

```text
WORD("echo")
WORD("| < > << >>")
```

No operator tokens should be created.

---

## Test 12 — Operators Inside Single Quotes

```bash
echo '| < > << >>'
```

Expected:

```text
WORD("echo")
WORD("| < > << >>")
```

---

## Test 13 — Multiple Operators

```bash
cat < input | grep hello > output
```

Expected:

```text
WORD("cat")
REDIR_IN("<")
WORD("input")
PIPE("|")
WORD("grep")
WORD("hello")
REDIR_OUT(">")
WORD("output")
```

---

## Test 14 — Complex Command

```bash
cat<<EOF|grep hello>>output
```

Expected:

```text
WORD("cat")
HEREDOC("<<")
WORD("EOF")
PIPE("|")
WORD("grep")
WORD("hello")
APPEND(">>")
WORD("output")
```

---

# 17.1 Test Table

| Input               | Expected operators |
| ------------------- | ------------------ |
| `echo hello \| wc`  | `PIPE`             |
| `echo hello\|wc`    | `PIPE`             |
| `cat < file`        | `REDIR_IN`         |
| `cat<file`          | `REDIR_IN`         |
| `echo hello > file` | `REDIR_OUT`        |
| `echo hello>file`   | `REDIR_OUT`        |
| `cat << EOF`        | `HEREDOC`          |
| `cat<<EOF`          | `HEREDOC`          |
| `echo hi >> file`   | `APPEND`           |
| `echo hi>>file`     | `APPEND`           |
| `echo "a\|b"`       | No operator        |
| `echo "< >"`        | No operator        |
| `echo '<< >>'`      | No operator        |

---

# 18. Final Checklist

## Basic Operators

* [ ] `|` is recognized.
* [ ] `<` is recognized.
* [ ] `>` is recognized.
* [ ] `<<` is recognized.
* [ ] `>>` is recognized.

## Longest Match

* [ ] `<<` is checked before `<`.
* [ ] `>>` is checked before `>`.
* [ ] Multi-character operators become one token.

## Spaces

* [ ] Operators work with spaces.
* [ ] Operators work without spaces.
* [ ] Multiple spaces around operators work.

## WORD Boundaries

* [ ] Operators terminate WORD parsing.
* [ ] Operators do not become part of WORD.
* [ ] The WORD after an operator is parsed correctly.

## Quotes

* [ ] `|` inside quotes is not an operator.
* [ ] `<` inside quotes is not an operator.
* [ ] `>` inside quotes is not an operator.
* [ ] `<<` inside quotes is not an operator.
* [ ] `>>` inside quotes is not an operator.
* [ ] Single quotes are respected.
* [ ] Double quotes are respected.

## Memory

* [ ] Operator token values are correctly allocated.
* [ ] Every token can be freed.
* [ ] No memory leaks.
* [ ] No invalid memory access.
* [ ] No double free.

---

# Final Concept

The Lexer should distinguish between:

```text
NORMAL TEXT
    |
    +---- WORD
    |
    +---- OPERATOR
```

while respecting quotes:

```text
                    INPUT
                      |
                      v
                 Inside quotes?
                 /            \
               YES             NO
                |               |
                v               v
        treat | < > << >>     operator?
        as normal text        /       \
                             YES       NO
                              |         |
                              v         v
                         OPERATOR      WORD
```

The most important rule is:

> **An operator is special only when it appears outside quotes.**

And when operators share the same first character:

```text
<<  >  <
```

or:

```text
>>  >  >
```

always use the **longest valid match first**.

The Lexer should therefore transform:

```bash
cat < input | grep hello >> output
```

into:

```text
WORD("cat")
REDIR_IN("<")
WORD("input")
PIPE("|")
WORD("grep")
WORD("hello")
APPEND(">>")
WORD("output")
```

At this stage, the Lexer does **not** need to execute the operators or understand their full semantic meaning. Its responsibility is simply to **recognize them and produce the correct token types**. The Parser and Executor will use these tokens later.
