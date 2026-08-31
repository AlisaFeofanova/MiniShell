# Shell Quoting Rules — Understanding Quoting in Minishell

## Table of Contents

* [1. What is Shell Quoting?](#1-what-is-shell-quoting)
* [2. Why Does the Shell Need Quotes?](#2-why-does-the-shell-need-quotes)
* [3. The Three Main Quoting Mechanisms](#3-the-three-main-quoting-mechanisms)
* [4. Unquoted Text](#4-unquoted-text)
* [5. Single Quotes](#5-single-quotes)
* [6. Double Quotes](#6-double-quotes)
* [7. Single vs Double Quotes](#7-single-vs-double-quotes)
* [8. Quotes and Tokenization](#8-quotes-and-tokenization)
* [9. Quotes Do Not Necessarily Create Arguments](#9-quotes-do-not-necessarily-create-arguments)
* [10. Concatenating Quoted and Unquoted Text](#10-concatenating-quoted-and-unquoted-text)
* [11. Empty Quotes](#11-empty-quotes)
* [12. Spaces Inside Quotes](#12-spaces-inside-quotes)
* [13. Environment Variables and Quotes](#13-environment-variables-and-quotes)
* [14. `$` Inside Single Quotes](#14--inside-single-quotes)
* [15. `$` Inside Double Quotes](#15--inside-double-quotes)
* [16. Backslash](#16-backslash)
* [17. Backslash Outside Quotes](#17-backslash-outside-quotes)
* [18. Backslash Inside Double Quotes](#18-backslash-inside-double-quotes)
* [19. Backslash Inside Single Quotes](#19-backslash-inside-single-quotes)
* [20. Quotes Inside Quotes](#20-quotes-inside-quotes)
* [21. Unclosed Quotes](#21-unclosed-quotes)
* [22. Quote Removal](#22-quote-removal)
* [23. Quoting and Operators](#23-quoting-and-operators)
* [24. Quoting and Redirections](#24-quoting-and-redirections)
* [25. Quoting and Pipes](#25-quoting-and-pipes)
* [26. Quoting and Expansion](#26-quoting-and-expansion)
* [27. Important Examples](#27-important-examples)
* [28. Lexer Responsibilities](#28-lexer-responsibilities)
* [29. Expansion Responsibilities](#29-expansion-responsibilities)
* [30. Common Implementation Mistakes](#30-common-implementation-mistakes)
* [31. Mini Test Suite](#31-mini-test-suite)
* [32. Team Checklist](#32-team-checklist)
* [33. Questions You Should Be Able to Answer](#33-questions-you-should-be-able-to-answer)
* [34. Key Mental Model](#34-key-mental-model)

---

# 1. What is Shell Quoting?

**Quoting** is a mechanism used by the shell to control how characters are interpreted.

Quotes can tell the shell:

* do not treat spaces as separators;
* do not interpret special characters;
* keep multiple pieces of text together;
* allow or prevent variable expansion;
* prevent operators from being recognized as operators.

For example:

```bash
echo hello world
```

contains two arguments after `echo`:

```text
hello
world
```

But:

```bash
echo "hello world"
```

contains one argument:

```text
hello world
```

So quotes change how the shell interprets the command line.

---

# 2. Why Does the Shell Need Quotes?

Many characters have special meanings in the shell.

For example:

```text
space
|
<
>
$
'
"
*
?
```

Without quoting, these characters may have special meanings.

For example:

```bash
echo hello world
```

The space separates arguments.

But:

```bash
echo "hello world"
```

the space is treated as part of the argument.

Similarly:

```bash
echo |
```

contains a pipe operator.

But:

```bash
echo "|"
```

contains a normal word containing `|`.

Therefore:

> Quoting changes the interpretation of characters.

---

# 3. The Three Main Quoting Mechanisms

For `minishell`, the most important quoting mechanisms are:

```text
1. Unquoted text
2. Single quotes: '
3. Double quotes: "
```

Additionally, shell syntax includes:

```text
4. Backslash: \
```

The behavior is different in each context.

---

# 4. Unquoted Text

Normal shell input is initially **unquoted**.

For example:

```bash
echo hello
```

The shell interprets:

```text
echo
hello
```

as separate words.

---

## 4.1 Spaces

In unquoted context:

```bash
echo hello world
```

becomes:

```text
echo
hello
world
```

because spaces separate words.

---

## 4.2 Operators

In unquoted context:

```bash
echo hello | grep hello
```

the `|` is an operator.

Tokens:

```text
WORD "echo"
WORD "hello"
PIPE "|"
WORD "grep"
WORD "hello"
```

---

## 4.3 Special Characters

Unquoted characters can have shell-specific meanings.

Examples:

```text
|
<
>
$
'
"
\
*
?
```

The exact behavior depends on the shell feature being implemented.

For the mandatory `minishell`, focus first on the syntax required by the project.

---

# 5. Single Quotes

Single quotes are:

```bash
'...'
```

Everything inside single quotes is treated literally.

For example:

```bash
echo '$USER'
```

The shell does **not** expand `$USER`.

The resulting argument is:

```text
$USER
```

not:

```text
alice
```

---

# 5.1 Basic Example

Input:

```bash
echo 'hello'
```

Argument:

```text
hello
```

The quotes themselves are not part of the final argument.

---

# 5.2 Spaces

Input:

```bash
echo 'hello world'
```

Result:

```text
hello world
```

It is one argument.

The space is preserved because it is inside single quotes.

---

# 5.3 Special Characters

Inside single quotes:

```bash
echo '$HOME'
```

produces:

```text
$HOME
```

Similarly:

```bash
echo '|'
```

produces:

```text
|
```

The `|` is not a pipe.

And:

```bash
echo '>'
```

produces:

```text
>
```

The `>` is not a redirection operator.

---

# 5.4 Single Quote Inside Single Quotes

A single quote cannot simply appear unescaped inside a single-quoted section.

For example:

```bash
'hello'world'
```

does not mean:

```text
hello'world
```

The quote closes the single-quoted section.

To include a literal single quote, you need to leave the single-quoted context and construct the argument differently.

For example:

```bash
echo 'hello'"'"'world'
```

conceptually produces:

```text
hello'world
```

This is an advanced but important example of how quote sections can be concatenated.

---

# 6. Double Quotes

Double quotes are:

```bash
"..."
```

They also prevent spaces from separating words.

For example:

```bash
echo "hello world"
```

produces one argument:

```text
hello world
```

But double quotes do **not** behave exactly like single quotes.

---

# 6.1 Variable Expansion Inside Double Quotes

Consider:

```bash
echo "$USER"
```

If:

```text
USER=alice
```

the result is:

```text
alice
```

Therefore:

```text
'$USER'
```

and:

```text
"$USER"
```

have different behavior.

---

# 6.2 Spaces Inside Double Quotes

Input:

```bash
echo "hello     world"
```

Result:

```text
hello     world
```

All spaces are preserved.

---

# 6.3 Operators Inside Double Quotes

Consider:

```bash
echo "|"
```

The `|` is not a pipe.

It is part of the argument:

```text
|
```

Similarly:

```bash
echo ">"
```

produces:

```text
>
```

and does not perform redirection.

---

# 7. Single vs Double Quotes

This is one of the most important things to understand.

| Feature                         | Single Quotes `'...'` | Double Quotes `"..."` |    |
| ------------------------------- | --------------------- | --------------------- | -- |
| Preserve spaces                 | Yes                   | Yes                   |    |
| Preserve literal text           | Yes                   | Mostly                |    |
| `$VARIABLE` expansion           | No                    | Yes                   |    |
| `                               | ` treated as operator | No                    | No |
| `<` treated as operator         | No                    | No                    |    |
| `>` treated as operator         | No                    | No                    |    |
| Quotes removed later            | Yes                   | Yes                   |    |
| Can contain same quote directly | No                    | No                    |    |

The most important difference:

```bash
'$USER'
```

means literal:

```text
$USER
```

while:

```bash
"$USER"
```

allows:

```text
alice
```

---

# 8. Quotes and Tokenization

Quotes affect how the Lexer groups characters.

Consider:

```bash
echo hello world
```

Tokens:

```text
WORD "echo"
WORD "hello"
WORD "world"
```

Now:

```bash
echo "hello world"
```

Tokens:

```text
WORD "echo"
WORD "hello world"
```

The quotes prevent the space from acting as a separator.

---

# 8.1 Quotes Do Not Automatically Become Tokens

This:

```bash
echo "hello"
```

does not mean:

```text
WORD "echo"
QUOTE
WORD "hello"
QUOTE
```

From the perspective of the command arguments, it is:

```text
WORD "echo"
WORD "hello"
```

The quotes control interpretation.

They are not normally part of the final argument.

---

# 9. Quotes Do Not Necessarily Create Arguments

This is extremely important.

Consider:

```bash
echo "hello"
```

There is one argument:

```text
hello
```

Now:

```bash
echo hello"world"
```

There is still one argument:

```text
helloworld
```

And:

```bash
echo "hello"'world'
```

also produces:

```text
helloworld
```

Therefore:

> Quotes do not necessarily separate arguments. They can be used to construct one argument from multiple pieces.

---

# 10. Concatenating Quoted and Unquoted Text

The shell can concatenate different sections into one word.

Example:

```bash
echo abc"def"ghi
```

Conceptually:

```text
abc
+
def
+
ghi
```

Result:

```text
abcdefghi
```

---

## 10.1 Different Quote Types

```bash
echo 'hello'"world"
```

Result:

```text
helloworld
```

---

## 10.2 Multiple Sections

```bash
echo a'b'c"d"e
```

Result:

```text
abcde
```

Everything belongs to one word.

---

# 11. Empty Quotes

Empty quotes create an empty string.

Example:

```bash
echo ""
```

The command receives:

```text
argv[0] = "echo"
argv[1] = ""
```

The second argument exists, but its value is empty.

---

## 11.1 Empty Single Quotes

```bash
echo ''
```

also creates:

```text
argv[1] = ""
```

---

## 11.2 Why This Matters

Compare:

```bash
echo ""
```

with:

```bash
echo
```

The first contains an empty argument.

The second contains no additional argument.

This difference is important when implementing argument parsing.

---

# 12. Spaces Inside Quotes

Unquoted:

```bash
echo hello world
```

means:

```text
hello
world
```

Quoted:

```bash
echo "hello world"
```

means:

```text
hello world
```

one argument.

---

## 12.1 Multiple Spaces

```bash
echo "hello     world"
```

preserves all spaces:

```text
hello     world
```

---

## 12.2 Spaces Around Quotes

Consider:

```bash
echo hello" world "
```

There is no separator between `hello` and the opening quote.

Therefore the result is one word:

```text
hello world
```

with the spaces inside the quotes preserved.

---

# 13. Environment Variables and Quotes

The shell performs variable expansion depending on context.

Example:

```bash
USER=Alice
```

Then:

```bash
echo $USER
```

can produce:

```text
Alice
```

Double quotes:

```bash
echo "$USER"
```

also allow expansion:

```text
Alice
```

Single quotes:

```bash
echo '$USER'
```

produce:

```text
$USER
```

---

# 14. `$` Inside Single Quotes

Inside:

```bash
'$USER'
```

the `$` is literal.

The shell must not interpret:

```text
$USER
```

as a variable reference.

Example:

```bash
echo '$USER'
```

Output:

```text
$USER
```

---

# 15. `$` Inside Double Quotes

Inside:

```bash
"$USER"
```

the variable can be expanded.

Example:

```bash
USER=Alice
echo "$USER"
```

Output:

```text
Alice
```

Therefore:

```text
Single quotes:
'$USER'
     |
     v
  literal


Double quotes:
"$USER"
     |
     v
 expansion
```

---

# 16. Backslash

Backslash:

```text
\
```

is another shell quoting mechanism.

Its behavior depends on the context.

In general, it can be used to prevent the next character from being interpreted specially.

For example:

```bash
echo hello\ world
```

produces:

```text
hello world
```

The escaped space does not separate the argument.

---

# 17. Backslash Outside Quotes

In unquoted context:

```bash
echo hello\ world
```

the backslash protects the space.

Instead of:

```text
WORD "hello"
WORD "world"
```

we get:

```text
WORD "hello world"
```

---

## 17.1 Escaping Special Characters

For example:

```bash
echo \|
```

produces a literal:

```text
|
```

instead of creating a `PIPE` token.

Similarly:

```bash
echo \>
```

produces:

```text
>
```

instead of a redirection operator.

---

# 18. Backslash Inside Double Quotes

Backslash behavior inside double quotes is more restricted than outside.

For a Bash-compatible shell, backslash retains special meaning before certain characters, including:

```text
"
\
$
`
newline
```

For example:

```bash
echo "hello \$USER"
```

produces:

```text
hello $USER
```

instead of expanding `$USER`.

Similarly:

```bash
echo "hello \"world\""
```

can produce:

```text
hello "world"
```

---

# 19. Backslash Inside Single Quotes

Inside single quotes:

```bash
'...'
```

backslash has no special escaping meaning.

For example:

```bash
echo '\$USER'
```

produces:

```text
\$USER
```

The backslash is literal.

This is one of the key differences between single and double quotes.

---

# 20. Quotes Inside Quotes

Quotes can appear inside another quote type.

## Single Quote Inside Double Quotes

```bash
echo "it's working"
```

The single quote is just a normal character.

Result:

```text
it's working
```

---

## Double Quote Inside Single Quotes

```bash
echo 'he said "hello"'
```

The double quotes are literal.

Result:

```text
he said "hello"
```

---

## Same Quote Type

The same quote type closes the current quote context.

For example:

```bash
echo 'hello'
```

The second `'` closes the first.

Likewise:

```bash
echo "hello"
```

The second `"` closes the first.

---

# 21. Unclosed Quotes

An opening quote must eventually be closed.

For example:

```bash
echo "hello
```

contains:

```text
"
```

without a matching closing quote.

Similarly:

```bash
echo 'hello
```

contains an unclosed single quote.

A correct shell must not simply treat this as a normal completed command.

---

# 21.1 Lexer State

The Lexer should track the current quote state:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

For:

```bash
echo "hello world"
```

the state changes like this:

```text
NORMAL
   |
   | "
   v
DOUBLE_QUOTE
   |
   | "
   v
NORMAL
```

---

# 22. Quote Removal

Quotes are generally syntax characters, not part of the final argument.

For:

```bash
echo "hello"
```

the command receives:

```text
hello
```

not:

```text
"hello"
```

Similarly:

```bash
echo 'hello'
```

produces:

```text
hello
```

---

# 22.1 Quote Removal Happens After Their Job Is Done

Conceptually:

```text
Input
 |
 v
Quote recognition
 |
 v
Expansion
 |
 v
Quote removal
 |
 v
Final argument
```

The exact internal ordering can vary depending on your implementation, but the important thing is:

> Quotes affect interpretation before they disappear from the final argument.

---

# 23. Quoting and Operators

Quoting can prevent operators from being recognized.

Unquoted:

```bash
echo |
```

contains:

```text
PIPE
```

But:

```bash
echo "|"
```

contains:

```text
WORD "|"
```

Similarly:

```bash
echo >
```

contains:

```text
REDIR_OUT
```

while:

```bash
echo ">"
```

contains:

```text
WORD ">"
```

---

# 24. Quoting and Redirections

Consider:

```bash
cat < input.txt
```

The `<` is an operator:

```text
REDIR_IN
```

But:

```bash
echo "< input.txt"
```

contains the entire string as an argument:

```text
< input.txt
```

No redirection is performed because the `<` is quoted.

---

# 24.1 Quoting the Filename

Consider:

```bash
cat < "my file.txt"
```

The filename contains a space.

The shell interprets:

```text
REDIR_IN
WORD "my file.txt"
```

This is a very common use of quotes with redirections.

---

# 25. Quoting and Pipes

Unquoted:

```bash
echo hello | grep hello
```

contains:

```text
PIPE
```

But:

```bash
echo "hello | world"
```

does not contain a pipe operator.

The `|` is part of the argument:

```text
hello | world
```

---

# 26. Quoting and Expansion

Quoting strongly affects expansion.

Consider:

```bash
NAME=Alice
```

### Unquoted

```bash
echo $NAME
```

Possible result:

```text
Alice
```

### Double quoted

```bash
echo "$NAME"
```

Result:

```text
Alice
```

### Single quoted

```bash
echo '$NAME'
```

Result:

```text
$NAME
```

The basic rule:

```text
'...'  -> no variable expansion
"..."  -> variable expansion allowed
```

---

# 27. Important Examples

## Example 1

```bash
echo hello world
```

Arguments:

```text
hello
world
```

---

## Example 2

```bash
echo "hello world"
```

Arguments:

```text
hello world
```

---

## Example 3

```bash
echo 'hello world'
```

Arguments:

```text
hello world
```

---

## Example 4

```bash
echo hello"world"
```

Argument:

```text
helloworld
```

---

## Example 5

```bash
echo "hello"'world'
```

Argument:

```text
helloworld
```

---

## Example 6

```bash
echo "$USER"
```

Variable is expanded.

---

## Example 7

```bash
echo '$USER'
```

Variable is not expanded.

---

## Example 8

```bash
echo "|"
```

Argument:

```text
|
```

No pipe.

---

## Example 9

```bash
echo '|'
```

Argument:

```text
|
```

No pipe.

---

## Example 10

```bash
echo ""
```

One empty argument:

```text
argv[1] = ""
```

---

## Example 11

```bash
echo "hello     world"
```

Argument:

```text
hello     world
```

All spaces are preserved.

---

## Example 12

```bash
echo hello\ world
```

Argument:

```text
hello world
```

---

## Example 13

```bash
echo "hello \$USER"
```

Result:

```text
hello $USER
```

---

## Example 14

```bash
echo "it's working"
```

Result:

```text
it's working
```

The single quote is literal because we are inside double quotes.

---

## Example 15

```bash
echo 'he said "hello"'
```

Result:

```text
he said "hello"
```

The double quotes are literal because we are inside single quotes.

---

# 28. Lexer Responsibilities

The Lexer should understand quote context.

Its responsibilities include:

```text
✔ Detect '
✔ Detect "
✔ Track quote state
✔ Keep quoted spaces inside the same WORD
✔ Prevent quoted operators from becoming operators
✔ Group quoted and unquoted text into one WORD
✔ Detect unclosed quotes
✔ Preserve information needed by later stages
```

For example:

```bash
echo hello"world"
```

must be recognized as one word.

---

# 29. Expansion Responsibilities

Expansion should handle things such as:

```text
$USER
$HOME
$?
```

according to the shell rules and the quote context.

For example:

```bash
echo '$USER'
```

must not expand `$USER`.

While:

```bash
echo "$USER"
```

can expand it.

Therefore the expansion stage needs to know the quoting context or receive enough information from the Lexer/parser to reproduce the correct shell semantics.

---

# 30. Common Implementation Mistakes

## Mistake 1 — Using `split()` by spaces

This fails:

```bash
echo "hello world"
```

because the quoted space should not split the argument.

---

## Mistake 2 — Treating Quotes as Separate Arguments

Incorrect:

```text
WORD "hello"
WORD "world"
```

for:

```bash
echo "hello world"
```

Correct:

```text
WORD "hello world"
```

---

## Mistake 3 — Treating Quotes as Part of the Final Argument

Incorrect final argument:

```text
"hello"
```

Correct:

```text
hello
```

The quotes are syntax.

---

## Mistake 4 — Expanding Variables Inside Single Quotes

Incorrect:

```bash
echo '$USER'
```

→

```text
Alice
```

Correct:

```text
$USER
```

---

## Mistake 5 — Treating Quoted Operators as Operators

Incorrect:

```bash
echo "|"
```

→

```text
PIPE
```

Correct:

```text
WORD "|"
```

---

## Mistake 6 — Splitting Concatenated Text

Incorrect:

```bash
echo hello"world"
```

into:

```text
hello
world
```

Correct:

```text
helloworld
```

---

## Mistake 7 — Ignoring Empty Arguments

Incorrectly treating:

```bash
echo ""
```

as:

```text
echo
```

The correct result contains an empty argument.

---

## Mistake 8 — Ignoring Quote State

Consider:

```bash
echo "hello | world"
```

If the Lexer does not know that it is inside double quotes, it may incorrectly identify:

```text
|
```

as a pipe.

---

## Mistake 9 — Removing Quotes Too Early

If quotes are removed before the shell understands their meaning, later stages may lose important information.

For example:

```bash
'$USER'
```

and:

```bash
"$USER"
```

cannot be treated identically.

---

# 31. Mini Test Suite

A good Lexer/quoting implementation should be tested with cases like these.

## Basic Quotes

```bash
echo "hello"
echo 'hello'
echo "hello world"
echo 'hello world'
```

Expected:

```text
hello
hello
hello world
hello world
```

---

## Concatenation

```bash
echo hello"world"
echo "hello"world
echo "hello"'world'
echo a'b'c"d"e
```

Expected:

```text
helloworld
helloworld
helloworld
abcde
```

---

## Variables

```bash
echo $USER
echo "$USER"
echo '$USER'
```

Expected conceptually:

```text
Alice
Alice
$USER
```

assuming:

```text
USER=Alice
```

---

## Operators

```bash
echo "|"
echo '|'
echo ">"
echo '<'
```

None of the quoted operators should be interpreted as shell operators.

---

## Spaces

```bash
echo hello world
echo "hello world"
echo 'hello world'
echo hello\ world
```

Expected argument counts:

```text
2 arguments
1 argument
1 argument
1 argument
```

after `echo`.

---

## Empty Arguments

```bash
echo ""
echo ''
```

Both should create an empty argument.

---

## Unclosed Quotes

Test:

```bash
echo "hello
```

and:

```bash
echo 'hello
```

The shell must detect the unclosed quote rather than silently producing a normal completed command.

---

# 32. Team Checklist

Every team member working on `minishell` should understand:

## Basic Concepts

* [ ] What quoting is.
* [ ] Why the shell needs quoting.
* [ ] The difference between syntax and literal characters.
* [ ] How quotes affect tokenization.

---

## Single Quotes

* [ ] `'hello'`
* [ ] `'hello world'`
* [ ] `'$USER'`
* [ ] `'|'`
* [ ] `'>'`
* [ ] Why expansion does not happen inside single quotes.

---

## Double Quotes

* [ ] `"hello"`
* [ ] `"hello world"`
* [ ] `"$USER"`
* [ ] `"|"`
* [ ] `">"`
* [ ] Why expansion can happen inside double quotes.

---

## Concatenation

Understand:

```bash
echo hello"world"
```

```bash
echo "hello"'world'
```

```bash
echo a'b'c"d"e
```

All of them create one argument.

---

## Empty Quotes

Understand:

```bash
echo ""
```

and:

```bash
echo ''
```

both create an empty argument.

---

## Operators

Understand why:

```bash
echo |
```

is different from:

```bash
echo "|"
```

and why:

```bash
echo >
```

is different from:

```bash
echo ">"
```

---

## Backslash

Understand:

```bash
echo hello\ world
```

and the different behavior of backslash:

```text
outside quotes
inside double quotes
inside single quotes
```

---

## Quote State

Understand:

```text
NORMAL
SINGLE_QUOTE
DOUBLE_QUOTE
```

and how the Lexer moves between them.

---

# 33. Questions You Should Be Able to Answer

Before considering Shell Quoting understood, you should be able to answer:

### Basic

1. What is shell quoting?
2. Why are quotes necessary?
3. What is the difference between single and double quotes?
4. Are quotes part of the final argument?
5. Why can't we simply split the command by spaces?

### Single Quotes

6. What happens to `$USER` inside `'...'`?
7. What happens to spaces inside `'...'`?
8. What happens to `|` inside `'...'`?
9. What happens to `>` inside `'...'`?

### Double Quotes

10. Can `$USER` be expanded inside `"..."`?
11. Are spaces preserved inside `"..."`?
12. Is `|` treated as a pipe inside `"..."`?
13. What happens to `>` inside `"..."`?

### Concatenation

14. What is the result of:

```bash
echo hello"world"
```

15. What is the result of:

```bash
echo "hello"'world'
```

16. Why are these one argument instead of two?

### Empty Quotes

17. What is the difference between:

```bash
echo
```

and:

```bash
echo ""
```

18. Why is an empty argument important?

### Backslash

19. What does this do?

```bash
echo hello\ world
```

20. How does backslash behave inside single quotes?
21. How does it behave inside double quotes?

### Lexer

22. What should the Lexer do when it sees `'`?
23. What should it do when it sees `"`?
24. How does the Lexer know whether `|` is an operator or literal text?
25. How does the Lexer detect an unclosed quote?

---

# 34. Key Mental Model

The most important idea is:

> **Quotes change how the shell interprets characters.**

Think of the command line as being processed in different contexts:

```text
                    INPUT
                      |
                      v
               +--------------+
               |    NORMAL    |
               +--------------+
                 |          |
               ' |          | "
                 v          v
          +-----------+  +-----------+
          |  SINGLE   |  |  DOUBLE   |
          |  QUOTE    |  |  QUOTE    |
          +-----------+  +-----------+
                 |          |
                 +----+-----+
                      |
                      v
                 WORD / TOKENS
                      |
                      v
                  EXPANSION
                      |
                      v
                QUOTE REMOVAL
                      |
                      v
                FINAL ARGUMENTS
```

---

# The Most Important Rules

Remember these rules:

### Rule 1

```bash
"hello world"
```

is one argument.

### Rule 2

```bash
'hello world'
```

is one argument.

### Rule 3

```bash
'$USER'
```

does not expand `$USER`.

### Rule 4

```bash
"$USER"
```

allows `$USER` expansion.

### Rule 5

```bash
hello"world"
```

is one word:

```text
helloworld
```

### Rule 6

```bash
"hello"'world'
```

is one word:

```text
helloworld
```

### Rule 7

```bash
"|"
```

is a normal argument, not a pipe.

### Rule 8

```bash
">"
```

is a normal argument, not a redirection.

### Rule 9

```bash
""
```

creates an empty argument.

### Rule 10

```bash
echo hello\ world
```

creates one argument:

```text
hello world
```

---

# Final Example

Consider this command:

```bash
echo "Hello $USER" '$HOME' hello" world" | grep "Hello"
```

The Lexer must understand:

```text
WORD "echo"
WORD "Hello $USER"
WORD "$HOME"
WORD "hello world"
PIPE "|"
WORD "grep"
WORD "Hello"
```

Then Expansion applies the appropriate rules:

```text
"Hello $USER"
      |
      v
"Hello Alice"

"$HOME"
   |
   v
"$HOME"      <- no expansion because it was single quoted
```

The final command structure conceptually becomes:

```text
Command 1:
    echo
    "Hello Alice"
    "$HOME"
    "hello world"

       |
       | PIPE
       v

Command 2:
    grep
    "Hello"
```

This is why **quoting is fundamental to the Shell processing pipeline**:

```text
Raw Input
    |
    v
Lexical Analysis
    |
    v
Quote Context
    |
    v
Tokens
    |
    v
Parsing
    |
    v
Expansion
    |
    v
Quote Removal
    |
    v
Final Arguments
    |
    v
Execution
```

If you understand **quote context, token boundaries, concatenation, expansion, operators, and quote removal**, you understand the core of Shell Quoting required for `minishell`.
