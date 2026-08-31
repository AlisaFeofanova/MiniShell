# Handle Spaces Correctly

## Table of Contents

* [1. Goal of the Task](#1-goal-of-the-task)
* [2. Why Correct Space Handling Is Important](#2-why-correct-space-handling-is-important)
* [3. The Main Rule](#3-the-main-rule)
* [4. Spaces Outside Quotes](#4-spaces-outside-quotes)
* [5. Multiple Spaces](#5-multiple-spaces)
* [6. Tabs](#6-tabs)
* [7. Spaces Inside Quotes](#7-spaces-inside-quotes)
* [8. Mixed Quoted and Unquoted Parts](#8-mixed-quoted-and-unquoted-parts)
* [9. Spaces Around Operators](#9-spaces-around-operators)
* [10. Empty Quoted Words](#10-empty-quoted-words)
* [11. Space-Handling Algorithm](#11-space-handling-algorithm)
* [12. Implementation](#12-implementation)
* [13. Common Mistakes](#13-common-mistakes)
* [14. Testing](#14-testing)
* [15. Final Checklist](#15-final-checklist)

---

# 1. Goal of the Task

The task:

> **Handle spaces correctly**

means that the Lexer must correctly determine whether a space:

1. separates two tokens;
2. belongs to the current WORD;
3. is inside quotes and therefore does not separate the WORD.

For example:

```bash
echo hello world
```

must become:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

But:

```bash
echo "hello world"
```

must become:

```text
WORD("echo")
WORD("hello world")
```

---

# 2. Why Correct Space Handling Is Important

Spaces are one of the main mechanisms Shell uses to separate commands and arguments.

For example:

```bash
ls -la file.txt
```

The Lexer should produce:

```text
WORD("ls")
WORD("-la")
WORD("file.txt")
```

If spaces are handled incorrectly, the Parser will receive an incorrect token list.

This can cause problems later in:

* Parsing
* Expansion
* Redirections
* Pipes
* Execution

Therefore, space handling is one of the fundamental parts of the Lexer.

---

# 3. The Main Rule

The most important rule is:

> **A space outside quotes separates tokens. A space inside quotes is part of the current WORD.**

Compare:

```bash
echo hello world
```

with:

```bash
echo "hello world"
```

The first one:

```text
echo
hello
world
```

The second one:

```text
echo
hello world
```

---

# 4. Spaces Outside Quotes

In the normal Lexer state:

```text
NORMAL
```

a space acts as a separator.

For example:

```bash
echo hello
```

The Lexer reads:

```text
echo
```

Then encounters:

```text
' '
```

and finishes the current WORD.

After skipping the space, it starts reading:

```text
hello
```

Final result:

```text
WORD("echo")
WORD("hello")
```

---

# 4.1 `is_space()`

Create a helper function:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

Then:

```c
if (is_space(input[i]))
    i++;
```

can be used to detect whitespace.

---

# 5. Multiple Spaces

Shell allows any number of spaces between words.

For example:

```bash
echo     hello
```

must produce:

```text
WORD("echo")
WORD("hello")
```

It must NOT produce:

```text
WORD("echo")
WORD("")
WORD("")
WORD("")
WORD("hello")
```

---

## Correct Algorithm

When the Lexer finds whitespace:

```text
skip current space
       |
       v
skip next space
       |
       v
skip next space
       |
       v
continue
```

A simple implementation is:

```c
while (is_space(input[i]))
    i++;
```

---

# 5.1 Leading Spaces

Input:

```bash
     echo hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

Leading spaces do not create tokens.

---

# 5.2 Trailing Spaces

Input:

```bash
echo hello     
```

Expected:

```text
WORD("echo")
WORD("hello")
```

Trailing spaces do not create tokens either.

---

# 5.3 Only Spaces

Input:

```bash
     
```

The Lexer should return:

```text
NULL
```

or an empty token list.

It should NOT create:

```text
WORD("")
```

---

# 6. Tabs

Shell whitespace is not limited to ordinary spaces.

A tab:

```text
'\t'
```

can also separate words.

For example:

```bash
echo	hello
```

should behave like:

```bash
echo hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

Therefore:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

---

# 6.1 Extended Whitespace

If your implementation requires additional whitespace characters, you can use:

```c
int is_space(char c)
{
    return (c == ' '
        || c == '\t'
        || c == '\n'
        || c == '\v'
        || c == '\f'
        || c == '\r');
}
```

However, for Minishell, follow the exact requirements of your project and how input is received from `readline()`.

---

# 7. Spaces Inside Quotes

This is the most important part.

Consider:

```bash
echo "hello world"
```

The space between:

```text
hello world
```

is NOT a separator.

Why?

Because the Lexer is currently inside:

```text
DOUBLE_QUOTE
```

state.

Therefore:

```text
"hello world"
```

must be treated as one WORD.

Result:

```text
WORD("echo")
WORD("hello world")
```

---

# 7.1 Single Quotes

The same applies to single quotes:

```bash
echo 'hello world'
```

Result:

```text
WORD("echo")
WORD("hello world")
```

The space inside:

```text
'hello world'
```

does not split the token.

---

# 7.2 Why You Cannot Simply Search for Spaces

This implementation is incorrect:

```c
while (input[i] != ' ')
    i++;
```

It will break:

```bash
echo "hello world"
```

into something like:

```text
WORD("echo")
WORD("\"hello")
WORD("world\"")
```

The Lexer must understand quote context.

---

# 8. Mixed Quoted and Unquoted Parts

This is an important Shell behavior.

Consider:

```bash
echo hello"world"
```

The Shell treats this as **one WORD**:

```text
helloworld
```

The Lexer must continue reading the same WORD after the quoted section ends.

---

## Another Example

```bash
echo "hello"world
```

This is also one WORD:

```text
helloworld
```

---

## Another Example

```bash
echo hello" "world
```

The result is:

```text
hello world
```

But it is still **one WORD**.

Conceptually:

```text
hello
+
" "
+
world
=
hello world
```

---

# 8.1 A WORD Can Contain Multiple Parts

Think of a WORD as a sequence of sections:

```text
WORD
 |
 +-- unquoted: hello
 |
 +-- quoted: " "
 |
 +-- unquoted: world
```

Final value:

```text
hello world
```

The presence of a quoted space does not create a second WORD.

---

# 9. Spaces Around Operators

Spaces can appear around Shell operators:

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

Expected:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

---

# 9.1 Operators Without Spaces

Spaces around an operator are optional.

For example:

```bash
echo hello|wc
```

must become:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

Therefore, the Lexer must recognize operators independently of whitespace.

---

# 9.2 Redirection

Input:

```bash
cat < input.txt
```

Expected:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

The same command without spaces:

```bash
cat<input.txt
```

must produce:

```text
WORD("cat")
REDIR_IN("<")
WORD("input.txt")
```

---

# 9.3 Multiple Spaces Around Operators

Input:

```bash
echo hello     |     wc
```

Expected:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
```

The number of spaces does not matter.

---

# 10. Empty Quoted Words

This is an important edge case:

```bash
echo ""
```

There is an empty WORD after `echo`.

Conceptually:

```text
WORD("echo")
WORD("")
```

An empty quoted string is different from having no token.

---

## Single Quotes

The same applies to:

```bash
echo ''
```

Expected:

```text
WORD("echo")
WORD("")
```

This distinction becomes important later for:

* Expansion
* Quote removal
* Parsing
* Execution

---

# 11. Space-Handling Algorithm

The main Lexer loop can follow this structure:

```text
while input[i] != '\0'

    if current character is whitespace
        skip all whitespace

    else if current character is an operator
        create operator token

    else
        read WORD

end
```

---

# 11.1 Inside WORD

When reading a WORD:

```text
start WORD

while input[i] exists

    if current character is whitespace
        stop WORD

    if current character is an operator
        stop WORD

    if current character is single quote
        read single-quoted section
        continue WORD

    if current character is double quote
        read double-quoted section
        continue WORD

    otherwise
        add character
        continue

end
```

The key point is:

> **Whitespace terminates a WORD only when the Lexer is outside quotes.**

---

# 12. Implementation

## `is_space()`

Basic implementation:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

---

# 12.1 Skipping Whitespace

In the main Lexer:

```c
while (input[i])
{
    if (is_space(input[i]))
    {
        i++;
        continue;
    }

    /*
     * Handle token.
     */
}
```

Or skip all consecutive whitespace at once:

```c
while (input[i] && is_space(input[i]))
    i++;
```

---

# 12.2 Main Lexer Structure

For example:

```c
t_token *lexer(char *input)
{
    t_token *tokens;
    int     i;

    tokens = NULL;
    i = 0;

    while (input[i])
    {
        if (is_space(input[i]))
        {
            i++;
            continue;
        }

        if (is_operator(input[i]))
        {
            handle_operator(input, &i, &tokens);
            continue;
        }

        handle_word(input, &i, &tokens);
    }

    return (tokens);
}
```

---

# 12.3 Important Detail

Do not simply do this inside `handle_word()`:

```c
if (input[i] == ' ')
    stop;
```

without considering quotes.

Because:

```bash
echo "hello world"
```

must preserve:

```text
hello world
```

as one WORD.

---

# 12.4 Example `handle_word()`

Conceptually:

```c
void handle_word(char *input, int *i, t_token **tokens)
{
    int     start;
    char    *value;

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

    value = extract_word(input, start, *i);

    add_token(tokens, value, TOKEN_WORD);
}
```

This is only a skeleton. Quote handling must be implemented carefully.

---

# 12.5 Architectural Principle

Do not mix every processing stage together.

A clean architecture is:

```text
RAW INPUT
    |
    v
  LEXER
    |
    | determines WORD boundaries
    v
TOKEN LIST
    |
    v
EXPANSION / QUOTE HANDLING
```

The Lexer determines that:

```bash
echo "hello world"
```

contains:

```text
WORD
```

The later stages can then decide what happens with:

* quotes;
* `$`;
* wildcard expansion;
* quote removal.

---

# 13. Common Mistakes

## Mistake 1 — Splitting only on spaces

Incorrect:

```c
while (input[i] != ' ')
    i++;
```

This breaks:

```bash
echo "hello world"
```

---

## Mistake 2 — Creating SPACE tokens

Do not normally create:

```text
WORD("echo")
SPACE
SPACE
WORD("hello")
```

Spaces are separators, not usually independent tokens.

---

## Mistake 3 — Creating empty WORDs

Input:

```bash
echo     hello
```

Incorrect:

```text
WORD("echo")
WORD("")
WORD("")
WORD("")
WORD("hello")
```

Correct:

```text
WORD("echo")
WORD("hello")
```

---

## Mistake 4 — Ignoring Quotes

Input:

```bash
echo "hello world"
```

Incorrect:

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

## Mistake 5 — Stopping WORD After a Quoted Section

Input:

```bash
echo "hello"world
```

Incorrect:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

Correct:

```text
WORD("echo")
WORD("helloworld")
```

A quoted section can be part of a larger WORD.

---

# 14. Testing

Create a temporary debug function:

```c
void print_tokens(t_token *tokens)
{
    while (tokens)
    {
        printf("TYPE: %d | VALUE: [%s]\n",
            tokens->type,
            tokens->value);
        tokens = tokens->next;
    }
}
```

---

## Test 1 — One Space

```bash
echo hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 2 — Multiple Spaces

```bash
echo     hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 3 — Leading Spaces

```bash
     echo hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 4 — Trailing Spaces

```bash
echo hello     
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 5 — Only Spaces

```bash
       
```

Expected:

```text
NULL
```

---

## Test 6 — Tab

```bash
echo	hello
```

Expected:

```text
WORD("echo")
WORD("hello")
```

---

## Test 7 — Space Inside Double Quotes

```bash
echo "hello world"
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 8 — Space Inside Single Quotes

```bash
echo 'hello world'
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 9 — Mixed Word

```bash
echo hello" world"
```

Expected:

```text
WORD("echo")
WORD("hello world")
```

---

## Test 10 — Quote + Word

```bash
echo "hello"world
```

Expected:

```text
WORD("echo")
WORD("helloworld")
```

---

## Test 11 — Operator With Spaces

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

## Test 12 — Operator Without Spaces

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

## Test 13 — Redirection With Spaces

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

## Test 14 — Redirection Without Spaces

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

## Test 15 — Empty Double Quotes

```bash
echo ""
```

Expected:

```text
WORD("echo")
WORD("")
```

---

## Test 16 — Empty Single Quotes

```bash
echo ''
```

Expected:

```text
WORD("echo")
WORD("")
```

---

# 14.1 Test Table

| Input                | Expected tokens             |
| -------------------- | --------------------------- |
| `echo hello`         | `echo`, `hello`             |
| `echo     hello`     | `echo`, `hello`             |
| `   echo hello`      | `echo`, `hello`             |
| `echo hello   `      | `echo`, `hello`             |
| `echo "hello world"` | `echo`, `hello world`       |
| `echo 'hello world'` | `echo`, `hello world`       |
| `echo hello"world"`  | `echo`, `helloworld`        |
| `echo "hello"world`  | `echo`, `helloworld`        |
| `echo hello \| wc`   | `echo`, `hello`, `\|`, `wc` |
| `echo hello\|wc`     | `echo`, `hello`, `\|`, `wc` |
| `cat < file`         | `cat`, `<`, `file`          |
| `cat<file`           | `cat`, `<`, `file`          |
| `echo ""`            | `echo`, `""`                |
| `echo ''`            | `echo`, `''`                |

---

# 15. Final Checklist

## Spaces

* [ ] A space outside quotes separates WORDs.
* [ ] Multiple consecutive spaces are skipped.
* [ ] Leading spaces are skipped.
* [ ] Trailing spaces are skipped.
* [ ] Input containing only spaces produces an empty token list.
* [ ] Tabs are handled as whitespace.

## Quotes

* [ ] Spaces inside `'...'` do not split WORDs.
* [ ] Spaces inside `"..."` do not split WORDs.
* [ ] A WORD can contain both quoted and unquoted sections.
* [ ] `hello" world"` remains one WORD.
* [ ] `"hello"world` remains one WORD.
* [ ] Empty quotes create an empty WORD.

## Operators

* [ ] Spaces around `|` work.
* [ ] `|` without spaces works.
* [ ] Spaces around `<` work.
* [ ] `<` without spaces works.
* [ ] Spaces around `>` work.
* [ ] `>` without spaces works.
* [ ] `<<` works.
* [ ] `>>` works.

## Memory

* [ ] Every created token is freed.
* [ ] Every token value is freed.
* [ ] No memory leaks.
* [ ] No double free.
* [ ] No invalid memory access.

---

# Final Concept

Remember this rule:

```text
                    SPACE
                      |
              +-------+-------+
              |               |
          OUTSIDE           INSIDE
          QUOTES            QUOTES
              |               |
              v               v
       separates WORD      part of WORD
```

For example:

```bash
echo hello world
```

becomes:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

But:

```bash
echo "hello world"
```

becomes:

```text
WORD("echo")
WORD("hello world")
```

And:

```bash
echo hello" "world
```

becomes one WORD:

```text
WORD("hello world")
```

---

# Main Goal of This Task

> **The Lexer must treat whitespace as a separator only when it is outside quotes. Whitespace inside quoted sections must remain part of the current WORD.**

Correct space handling provides the foundation for:

```text
Spaces
   ↓
Quotes
   ↓
Words
   ↓
Operators
   ↓
Tokens
   ↓
Parser
   ↓
Expansion
   ↓
Executor
```

If this rule is implemented correctly, the Lexer will be able to provide the Parser with a reliable and predictable token list.
