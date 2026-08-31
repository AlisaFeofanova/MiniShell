# Create a Basic Lexer

## Table of Contents

* [1. What Is a Lexer?](#1-what-is-a-lexer)
* [2. Where Does the Lexer Fit?](#2-where-does-the-lexer-fit)
* [3. Main Responsibility](#3-main-responsibility)
* [4. Input and Output](#4-input-and-output)
* [5. Characters the Lexer Must Recognize](#5-characters-the-lexer-must-recognize)
* [6. Lexer States](#6-lexer-states)
* [7. Basic Lexer Algorithm](#7-basic-lexer-algorithm)
* [8. Reading Spaces](#8-reading-spaces)
* [9. Reading Words](#9-reading-words)
* [10. Reading Operators](#10-reading-operators)
* [11. Handling Single Quotes](#11-handling-single-quotes)
* [12. Handling Double Quotes](#12-handling-double-quotes)
* [13. Building a Token](#13-building-a-token)
* [14. Complete Basic Lexer Structure](#14-complete-basic-lexer-structure)
* [15. Testing](#15-testing)
* [16. Error Cases](#16-error-cases)
* [17. What the Basic Lexer Should NOT Do](#17-what-the-basic-lexer-should-not-do)
* [18. Implementation Plan](#18-implementation-plan)
* [19. Final Checklist](#19-final-checklist)

---

# 1. What Is a Lexer?

A **Lexer** is the part of Minishell that reads the raw input string and breaks it into meaningful tokens.

For example:

```bash
echo hello | wc -l
```

The Lexer transforms it into:

```text
WORD("echo")
WORD("hello")
PIPE("|")
WORD("wc")
WORD("-l")
```

The Lexer does not execute anything.

It only answers:

> "What are the individual syntactic elements in this input?"

---

# 2. Where Does the Lexer Fit?

The shell processing pipeline is:

```text
                 USER INPUT
                     |
                     v
              +--------------+
              |    LEXER     |
              +------+-------+
                     |
                     v
                TOKEN LIST
                     |
                     v
              +--------------+
              |    PARSER    |
              +------+-------+
                     |
                     v
              COMMAND STRUCTURE
                     |
                     v
              +--------------+
              |   EXPANSION  |
              +------+-------+
                     |
                     v
              +--------------+
              |   EXECUTOR   |
              +--------------+
```

The Lexer is the **first real processing stage** after reading the command line.

---

# 3. Main Responsibility

The basic Lexer should:

* Read the input character by character.
* Ignore spaces outside quotes.
* Recognize words.
* Recognize operators.
* Handle single quotes.
* Handle double quotes.
* Create tokens.
* Add tokens to the token list.
* Detect unterminated quotes.
* Preserve enough information for later parsing and expansion.

The Lexer should **not execute commands**.

---

# 4. Input and Output

## Input

For example:

```bash
echo hello > output.txt
```

The Lexer receives:

```c
char *input;
```

with:

```text
echo hello > output.txt
```

## Output

It creates:

```text
WORD("echo")
WORD("hello")
REDIR_OUT(">")
WORD("output.txt")
```

represented internally as:

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
| >           |
| REDIR_OUT   |
+------+------+
       |
       v
+-------------+
| output.txt  |
| WORD        |
+-------------+
```

---

# 5. Characters the Lexer Must Recognize

For Minishell, the basic lexer needs to recognize:

## Whitespace

```text
' '
'\t'
```

Depending on your implementation, you may also consider other whitespace characters.

Whitespace normally separates words.

Example:

```bash
echo hello
```

becomes:

```text
WORD("echo")
WORD("hello")
```

---

## Operators

The important shell operators for Minishell are:

```text
|
<
>
<<
>>
```

They must become separate token types.

---

## Quotes

The Lexer must recognize:

```text
'
"
```

Quotes affect how characters are interpreted.

---

# 6. Lexer States

A very useful way to implement the Lexer is to think in terms of states.

The basic states are:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

### NORMAL

Normal shell processing.

Example:

```text
echo hello
```

Spaces separate words.

Operators are recognized.

---

### SINGLE_QUOTE

Inside:

```bash
'hello world'
```

Everything is treated literally until the next `'`.

For example:

```bash
echo '$USER'
```

The `$USER` should not be expanded inside single quotes.

The Lexer itself does not necessarily perform the expansion, but it must preserve the quote context for later stages.

---

### DOUBLE_QUOTE

Inside:

```bash
"hello world"
```

Spaces do not terminate the word.

For example:

```bash
echo "hello world"
```

must produce:

```text
WORD("echo")
WORD("hello world")
```

not:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

---

# 7. Basic Lexer Algorithm

The main loop can look conceptually like:

```text
while input[i] != '\0'

    if current character is whitespace
        skip it

    else if current character is '|'
        create PIPE token

    else if current character is '<'
        check for <<
        create appropriate token

    else if current character is '>'
        check for >>
        create appropriate token

    else
        read a WORD

end
```

Conceptually:

```text
                input[i]
                   |
          +--------+--------+
          |        |        |
       space    operator    word
          |        |        |
          v        v        v
        skip     token    read word
                            |
                            v
                          token
```

---

# 8. Reading Spaces

Outside quotes, spaces separate tokens.

Example:

```bash
echo    hello
```

should produce:

```text
WORD("echo")
WORD("hello")
```

Multiple spaces should not create empty tokens.

Implementation:

```c
while (input[i] == ' ' || input[i] == '\t')
    i++;
```

Do this only when you are in the normal state.

---

# 9. Reading Words

A WORD is more complicated than simply "characters until a space."

For example:

```bash
echo "hello world"
```

The second token is:

```text
hello world
```

even though it contains a space.

Another example:

```bash
echo hello"world"
```

The shell considers this one word:

```text
helloworld
```

So the Lexer should continue reading a word through quoted sections.

---

## Basic Word Algorithm

When the current character is not whitespace or an operator:

```text
start word

while:
    not whitespace
    AND not operator
    AND not end of input

    if character == '
        read until next '

    else if character == "
        read until next "

    else
        add character

end word
```

---

# 10. Reading Operators

The operators are:

```text
|
<
>
<<
>>
```

There is an important detail:

```text
<
<<
```

are different operators.

Likewise:

```text
>
>>
```

are different operators.

Therefore, when the Lexer sees `<`, it should check the next character.

---

## `<`

Input:

```bash
cat < file
```

Token:

```text
TOKEN_REDIR_IN("<")
```

---

## `<<`

Input:

```bash
cat << EOF
```

Token:

```text
TOKEN_HEREDOC("<<")
```

---

## `>`

Input:

```bash
echo hello > file
```

Token:

```text
TOKEN_REDIR_OUT(">")
```

---

## `>>`

Input:

```bash
echo hello >> file
```

Token:

```text
TOKEN_APPEND(">>")
```

---

# 10.1 Operator Detection

Conceptually:

```c
if (input[i] == '<')
{
    if (input[i + 1] == '<')
    {
        create_token("<<", TOKEN_HEREDOC);
        i += 2;
    }
    else
    {
        create_token("<", TOKEN_REDIR_IN);
        i++;
    }
}
```

Similarly:

```c
if (input[i] == '>')
{
    if (input[i + 1] == '>')
    {
        create_token(">>", TOKEN_APPEND);
        i += 2;
    }
    else
    {
        create_token(">", TOKEN_REDIR_OUT);
        i++;
    }
}
```

And:

```c
if (input[i] == '|')
{
    create_token("|", TOKEN_PIPE);
    i++;
}
```

---

# 11. Handling Single Quotes

Single quotes:

```bash
'
```

preserve the literal contents.

Example:

```bash
echo '$USER'
```

The content is:

```text
$USER
```

The `$USER` should not be interpreted as an environment variable because it is inside single quotes.

For the Lexer, the important rule is:

```text
'
 |
 +-- everything until the next '
```

Example:

```bash
echo 'hello world'
```

The entire quoted section belongs to one WORD.

Conceptually:

```text
echo
 |
 v
'hello world'
 |
 v
WORD("hello world")
```

---

# 11.1 Unterminated Single Quote

Input:

```bash
echo 'hello
```

There is no closing:

```text
'
```

The Lexer must detect this.

Do not silently continue.

Return an error such as:

```text
Unclosed quote
```

Your Minishell can then set the appropriate exit status according to the project requirements.

---

# 12. Handling Double Quotes

Double quotes:

```bash
"
```

also prevent spaces from splitting the word.

Example:

```bash
echo "hello world"
```

should produce:

```text
WORD("echo")
WORD("hello world")
```

not:

```text
WORD("echo")
WORD("hello")
WORD("world")
```

---

## Double Quotes and `$`

Inside double quotes, parameter expansion can still be relevant:

```bash
echo "$USER"
```

The Lexer should not perform the actual variable expansion.

It should identify the quoted word correctly and leave expansion for the appropriate later stage.

This separation is important:

```text
Lexer
    ↓
recognizes quoted WORD

Expansion
    ↓
processes $USER
```

---

# 12.1 Unterminated Double Quote

Input:

```bash
echo "hello
```

There is no closing:

```text
"
```

The Lexer should report an unclosed quote error.

---

# 13. Building a Token

Assuming you already implemented:

```c
t_token *token_new(char *value, t_token_type type);
```

the Lexer can create tokens using:

```c
token_new(value, type);
```

and add them:

```c
token_add_back(&tokens, token);
```

For example:

```c
token = token_new(ft_strdup("echo"), TOKEN_WORD);
token_add_back(&tokens, token);
```

---

# 13.1 Important Memory Rule

If `token_new()` takes ownership of the allocated string, make that ownership clear.

For example:

```c
char *value;

value = ft_strdup("echo");
token = token_new(value, TOKEN_WORD);
```

After that, the Token owns:

```text
value
```

and `free_tokens()` should release it.

---

# 14. Complete Basic Lexer Structure

A good basic architecture is:

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
            i++;

        else if (input[i] == '|')
            handle_pipe(input, &i, &tokens);

        else if (input[i] == '<')
            handle_input_redirection(input, &i, &tokens);

        else if (input[i] == '>')
            handle_output_redirection(input, &i, &tokens);

        else
            handle_word(input, &i, &tokens);
    }

    return (tokens);
}
```

This is much easier to maintain than putting everything into one huge function.

---

# 14.1 Helper Functions

A possible design:

```c
int     is_space(char c);
int     is_operator(char c);

void    handle_pipe(
            char *input,
            int *i,
            t_token **tokens
        );

void    handle_input_redirection(
            char *input,
            int *i,
            t_token **tokens
        );

void    handle_output_redirection(
            char *input,
            int *i,
            t_token **tokens
        );

int     handle_word(
            char *input,
            int *i,
            t_token **tokens
        );
```

You can adapt the names to your project's coding style.

---

# 14.2 `is_operator()`

A simple helper:

```c
int is_operator(char c)
{
    return (c == '|' || c == '<' || c == '>');
}
```

Notice that:

```text
<<
>>
```

are still detected by looking at the next character.

---

# 14.3 `is_space()`

For a basic implementation:

```c
int is_space(char c)
{
    return (c == ' ' || c == '\t');
}
```

You can extend this if your project requirements need additional whitespace handling.

---

# 14.4 Reading a Word

Conceptually:

```c
char *read_word(char *input, int *i)
{
    int     start;
    char    *word;

    start = *i;

    while (input[*i]
        && !is_space(input[*i])
        && !is_operator(input[*i]))
    {
        if (input[*i] == '\'' || input[*i] == '"')
        {
            /*
             * Handle quoted section.
             */
        }
        else
            (*i)++;
    }

    word = substring_from_input(input, start, *i);

    return (word);
}
```

This is only a skeleton.

The quote handling needs to be implemented carefully.

---

# 15. Testing

Testing is one of the most important parts of Lexer development.

Create a small test program that prints:

```text
TYPE
VALUE
```

for every token.

---

## Test 1 — Simple command

Input:

```bash
echo hello
```

Expected:

```text
[WORD] echo
[WORD] hello
```

---

## Test 2 — Multiple spaces

Input:

```bash
echo     hello
```

Expected:

```text
[WORD] echo
[WORD] hello
```

---

## Test 3 — Pipe

Input:

```bash
echo hello | wc
```

Expected:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
```

---

## Test 4 — Input redirection

Input:

```bash
cat < input.txt
```

Expected:

```text
[WORD] cat
[REDIR_IN] <
[WORD] input.txt
```

---

## Test 5 — Output redirection

Input:

```bash
echo hello > output.txt
```

Expected:

```text
[WORD] echo
[WORD] hello
[REDIR_OUT] >
[WORD] output.txt
```

---

## Test 6 — Append

Input:

```bash
echo hello >> output.txt
```

Expected:

```text
[WORD] echo
[WORD] hello
[APPEND] >>
[WORD] output.txt
```

---

## Test 7 — Heredoc

Input:

```bash
cat << EOF
```

Expected:

```text
[WORD] cat
[HEREDOC] <<
[WORD] EOF
```

---

# 15.1 Quote Tests

## Single quotes

Input:

```bash
echo 'hello world'
```

Expected:

```text
[WORD] echo
[WORD] hello world
```

---

## Double quotes

Input:

```bash
echo "hello world"
```

Expected:

```text
[WORD] echo
[WORD] hello world
```

---

## Mixed quotes

Input:

```bash
echo 'hello'"world"
```

This should be understood as one WORD.

Conceptually:

```text
hello + world
```

Result:

```text
[WORD] helloworld
```

The exact handling depends on your quote-removal architecture, but the important point is that the two quoted sections form one Shell word.

---

# 15.2 Empty Quotes

Test:

```bash
echo ""
```

The quoted empty string represents an empty word.

Therefore, the Lexer/parser architecture must be able to distinguish:

```text
no token
```

from:

```text
WORD("")
```

This is an important edge case.

Similarly:

```bash
echo ''
```

should produce an empty WORD.

---

# 15.3 Operators Without Spaces

The Lexer must not require spaces around operators.

For example:

```bash
echo hello>file
```

should become:

```text
[WORD] echo
[WORD] hello
[REDIR_OUT] >
[WORD] file
```

Similarly:

```bash
echo hello|wc
```

becomes:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
```

This is why the Lexer must recognize operators independently of whitespace.

---

# 16. Error Cases

The Lexer should identify at least these problems.

## Unterminated single quote

```bash
echo 'hello
```

Error:

```text
unclosed single quote
```

---

## Unterminated double quote

```bash
echo "hello
```

Error:

```text
unclosed double quote
```

---

## Lexer vs Parser Errors

It is important to understand that not every invalid input is a Lexer error.

For example:

```bash
echo hello |
```

The Lexer can successfully produce:

```text
WORD("echo")
WORD("hello")
PIPE("|")
```

There is nothing wrong with tokenization.

The problem is the **syntax**.

Therefore, the Parser should detect:

```text
PIPE at the end
```

and report a syntax error.

---

# 16. What the Basic Lexer Should NOT Do

Keep responsibilities separated.

The Lexer should NOT:

### Execute commands

It should not call:

```c
execve()
```

---

### Create pipes

It should not call:

```c
pipe()
```

---

### Fork processes

It should not call:

```c
fork()
```

---

### Execute builtins

It should not execute:

```text
cd
echo
pwd
export
unset
env
exit
```

---

### Perform actual command execution

That belongs to the Executor.

---

### Decide the final command structure

That belongs to the Parser.

---

### Perform execution-time redirections

The Lexer only identifies:

```text
>
>>
<
<<
```

The Executor later performs the actual file descriptor operations.

---

# 16.1 What About Expansion?

Do not mix the Lexer and expansion unnecessarily.

For example:

```bash
echo $USER
```

The Lexer should identify:

```text
WORD("echo")
WORD("$USER")
```

Then the expansion stage can process:

```text
$USER
```

into the corresponding environment value.

Similarly:

```bash
echo "$USER"
```

should remain one WORD during lexical processing.

---

# 17. Implementation Plan

## Step 1 — Create Lexer files

Possible structure:

```text
src/
├── lexer/
│   ├── lexer.c
│   ├── lexer_utils.c
│   ├── lexer_word.c
│   └── lexer_quotes.c
│
├── token/
│   ├── token_new.c
│   ├── token_add_back.c
│   └── token_free.c
│
├── parser/
│
└── executor/
```

You can organize the files differently according to your team's style and the 42 Norm.

---

## Step 2 — Implement character helpers

Create:

```c
int is_space(char c);
int is_operator(char c);
```

Test them independently.

---

## Step 3 — Implement operator recognition

Handle:

```text
|
<
>
<<
>>
```

Verify that:

```text
<
<<
>
>>
```

are recognized correctly.

---

## Step 4 — Implement basic WORD parsing

Start with:

```bash
echo hello world
```

Then test:

```bash
echo hello
echo hello world
ls -la
```

---

## Step 5 — Add quote handling

Implement:

```text
'
"
```

Test:

```bash
echo 'hello world'
echo "hello world"
```

Then:

```bash
echo hello"world"
echo "hello"'world'
```

---

## Step 6 — Add error detection

Test:

```bash
echo 'hello
```

and:

```bash
echo "hello
```

The Lexer must detect the missing closing quote.

---

## Step 7 — Add debug output

Use:

```text
[WORD] echo
[WORD] hello
[PIPE] |
[WORD] wc
```

This will make debugging much easier.

---

# 18. Example: Complete Lexical Process

Input:

```bash
echo "hello world" > output.txt | cat
```

The Lexer starts at:

```text
e
```

Recognizes:

```text
WORD("echo")
```

Then skips the space.

It sees:

```text
"
```

and enters:

```text
DOUBLE_QUOTE
```

It reads:

```text
hello world
```

until the closing quote.

Creates:

```text
WORD("hello world")
```

Then sees:

```text
>
```

Creates:

```text
REDIR_OUT(">")
```

Then reads:

```text
output.txt
```

Creates:

```text
WORD("output.txt")
```

Then sees:

```text
|
```

Creates:

```text
PIPE("|")
```

Finally:

```text
cat
```

becomes:

```text
WORD("cat")
```

Final token list:

```text
WORD("echo")
    |
    v
WORD("hello world")
    |
    v
REDIR_OUT(">")
    |
    v
WORD("output.txt")
    |
    v
PIPE("|")
    |
    v
WORD("cat")
    |
    v
NULL
```

---

# 19. Lexer Pseudocode

A useful pseudocode version:

```text
function lexer(input):

    tokens = NULL
    i = 0

    while input[i] != '\0':

        if input[i] is whitespace:
            skip whitespace

        else if input[i] == '|':
            create PIPE
            i++

        else if input[i] == '<':
            if next character is '<':
                create HEREDOC
                i += 2
            else:
                create REDIR_IN
                i++

        else if input[i] == '>':
            if next character is '>':
                create APPEND
                i += 2
            else:
                create REDIR_OUT
                i++

        else:
            read WORD
            handle quotes
            create WORD token

    return tokens
```

---

# 20. Final Lexer Architecture

Your Lexer should eventually behave like this:

```text
                      RAW INPUT
                          |
                          v
                 +----------------+
                 |     LEXER      |
                 +----------------+
                          |
             +------------+------------+
             |            |            |
             v            v            v
          WORDS       OPERATORS      QUOTES
             |            |            |
             +------------+------------+
                          |
                          v
                    TOKEN LIST
                          |
                          v
                 +----------------+
                 |     PARSER     |
                 +----------------+
```

---

# 21. Final Checklist

## Basic Lexer

* [ ] Lexer receives raw input.
* [ ] Lexer iterates character by character.
* [ ] Spaces are skipped outside quotes.
* [ ] Words are recognized.
* [ ] `|` is recognized.
* [ ] `<` is recognized.
* [ ] `>` is recognized.
* [ ] `<<` is recognized.
* [ ] `>>` is recognized.

## Quotes

* [ ] Single quotes are recognized.
* [ ] Double quotes are recognized.
* [ ] Spaces inside quotes do not split words.
* [ ] Single-quoted content remains literal.
* [ ] Double-quoted content remains one word.
* [ ] Unterminated single quotes are detected.
* [ ] Unterminated double quotes are detected.
* [ ] Empty quoted strings are handled.

## Token Creation

* [ ] Lexer creates `TOKEN_WORD`.
* [ ] Lexer creates `TOKEN_PIPE`.
* [ ] Lexer creates `TOKEN_REDIR_IN`.
* [ ] Lexer creates `TOKEN_REDIR_OUT`.
* [ ] Lexer creates `TOKEN_APPEND`.
* [ ] Lexer creates `TOKEN_HEREDOC`.
* [ ] Tokens are linked correctly.
* [ ] Token values are correctly allocated.
* [ ] Memory ownership is clear.

## Testing

* [ ] `echo hello`
* [ ] `echo hello world`
* [ ] `echo hello | wc`
* [ ] `cat < file`
* [ ] `echo hello > file`
* [ ] `echo hello >> file`
* [ ] `cat << EOF`
* [ ] `echo "hello world"`
* [ ] `echo 'hello world'`
* [ ] `echo hello"world"`
* [ ] `echo ""`
* [ ] `echo ''`
* [ ] `echo hello>file`
* [ ] `echo hello|wc`
* [ ] Unclosed `'`
* [ ] Unclosed `"`

---

# The Main Concept to Remember

The Lexer converts:

```bash
echo "hello world" > file | cat
```

into:

```text
WORD("echo")
WORD("hello world")
REDIR_OUT(">")
WORD("file")
PIPE("|")
WORD("cat")
```

It does **not** execute the command.

The responsibilities are separated:

```text
             RAW INPUT
                 |
                 v
              LEXER
                 |
                 |  "What tokens are here?"
                 v
            TOKEN LIST
                 |
                 v
              PARSER
                 |
                 |  "What does this syntax mean?"
                 v
          COMMAND STRUCTURE
                 |
                 v
             EXPANSION
                 |
                 v
             EXECUTOR
                 |
                 |  "Run it"
                 v
             PROCESSES
```

### The key responsibility of this task

> **Build a basic Lexer that reliably converts raw shell input into an ordered list of typed tokens while correctly recognizing words, operators, whitespace, and quotes.**

The Lexer is the foundation of the Parser. If tokenization is incorrect, every stage after it will receive incorrect information.
