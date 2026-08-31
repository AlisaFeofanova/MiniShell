# Parameter Expansion in Minishell

## Table of Contents

* [1. What Is Parameter Expansion?](#1-what-is-parameter-expansion)
* [2. Why Do We Need Parameter Expansion?](#2-why-do-we-need-parameter-expansion)
* [3. What Is a Parameter?](#3-what-is-a-parameter)
* [4. Basic `$VAR` Syntax](#4-basic-var-syntax)
* [5. Environment Variables](#5-environment-variables)
* [6. What Happens If a Variable Does Not Exist?](#6-what-happens-if-a-variable-does-not-exist)
* [7. Empty Variables](#7-empty-variables)
* [8. `$?` — Exit Status](#8---exit-status)
* [9. `$` Before an Ordinary Character](#9--before-an-ordinary-character)
* [10. Expansion Inside and Outside Quotes](#10-expansion-inside-and-outside-quotes)
* [11. Double Quotes](#11-double-quotes)
* [12. Single Quotes](#12-single-quotes)
* [13. Expansion and Spaces](#13-expansion-and-spaces)
* [14. Expansion and Word Splitting](#14-expansion-and-word-splitting)
* [15. Expansion and Empty Variables](#15-expansion-and-empty-variables)
* [16. Expansion and Command Arguments](#16-expansion-and-command-arguments)
* [17. Expansion After Redirection](#17-expansion-after-redirection)
* [18. Expansion in Heredoc](#18-expansion-in-heredoc)
* [19. Shell Processing Order](#19-shell-processing-order)
* [20. Parameter Expansion Algorithm](#20-parameter-expansion-algorithm)
* [21. Pseudocode](#21-pseudocode)
* [22. Data Structures](#22-data-structures)
* [23. Common Mistakes](#23-common-mistakes)
* [24. Testing](#24-testing)
* [25. Checklist](#25-checklist)
* [26. Knowledge Check Questions](#26-knowledge-check-questions)
* [27. Main Mental Model](#27-main-mental-model)

---

# 1. What Is Parameter Expansion?

**Parameter Expansion** is the process where the Shell replaces a parameter reference such as:

```bash
$USER
```

with the value of that parameter.

For example:

```bash
echo $USER
```

If:

```text
USER=alice
```

the result is:

```text
alice
```

Conceptually:

```text
$USER
  ↓
value of USER
```

---

# 2. Why Do We Need Parameter Expansion?

Shell allows us to store information inside variables.

For example:

```bash
NAME="Alice"
```

Then:

```bash
echo $NAME
```

is expanded into something equivalent to:

```bash
echo Alice
```

Before the command is executed, the Shell has to replace:

```text
$NAME
```

with:

```text
Alice
```

The general idea is:

```text
INPUT
  |
  v
echo $NAME
  |
  v
Parameter Expansion
  |
  v
echo Alice
  |
  v
Execution
```

---

# 3. What Is a Parameter?

A **parameter** is a value that the Shell can access using a special syntax.

In Minishell, the most important forms are:

```bash
$NAME
```

and:

```bash
$?
```

For example:

```bash
USER=Alice
```

Here:

```text
USER
```

is the parameter name.

And:

```text
Alice
```

is its value.

---

# 4. Basic `$VAR` Syntax

The basic syntax is:

```bash
$VARIABLE
```

For example:

```bash
echo $HOME
```

If:

```text
HOME=/home/alice
```

the command becomes conceptually:

```bash
echo /home/alice
```

---

## 4.1 How Does the Shell Find the Variable Name?

After `$`, the Shell looks for characters that can form a variable name.

For example:

```bash
$USER
```

contains the variable:

```text
USER
```

Similarly:

```bash
$HOME
```

contains:

```text
HOME
```

and:

```bash
$PATH
```

contains:

```text
PATH
```

---

## 4.2 Valid Variable Name Characters

Variable names generally contain:

```text
A-Z
a-z
0-9
_
```

A variable name cannot normally start with a digit when creating the variable.

Examples:

```text
USER
HOME
PATH
MY_VAR
VAR123
```

---

# 5. Environment Variables

Minishell receives the environment through something like:

```c
char **envp
```

For example:

```text
USER=alice
HOME=/home/alice
PATH=/usr/bin:/bin
SHELL=/bin/bash
```

Inside Minishell, the environment can be stored in a linked list:

```c
typedef struct s_env
{
    char            *key;
    char            *value;
    struct s_env     *next;
} t_env;
```

Conceptually:

```text
key       value
-------------------------
USER      alice
HOME      /home/alice
PATH      /usr/bin:/bin
```

When Minishell encounters:

```bash
$USER
```

it searches for:

```text
key = "USER"
```

and retrieves:

```text
value = "alice"
```

---

# 5.1 Looking Up a Variable

A simplified function could look like:

```c
char *get_env_value(t_env *env, char *key)
{
    while (env)
    {
        if (strcmp(env->key, key) == 0)
            return (env->value);
        env = env->next;
    }
    return (NULL);
}
```

For example:

```text
get_env_value(env, "HOME")
```

could return:

```text
/home/alice
```

---

# 6. What Happens If a Variable Does Not Exist?

Consider:

```bash
echo $NOT_EXIST
```

If:

```text
NOT_EXIST
```

does not exist in the environment, the expansion normally produces an empty value:

```text
""
```

So conceptually:

```bash
echo $NOT_EXIST
```

becomes:

```bash
echo
```

The important point is:

```text
$NOT_EXIST
      ↓
empty string
```

It does **not** become:

```text
NOT_EXIST
```

and it does not remain:

```text
$NOT_EXIST
```

in the normal expansion context.

---

# 7. Empty Variables

There is a difference between:

```text
variable is unset
```

and:

```text
variable exists but has an empty value
```

For example:

```bash
export NAME=
```

means:

```text
NAME=""
```

Then:

```bash
echo $NAME
```

produces an empty result.

For basic Parameter Expansion, both an unset variable and an empty variable can result in:

```text
""
```

However, internally they are different states.

---

# 8. `$?` — Exit Status

The special parameter:

```bash
$?
```

contains the exit status of the previous command.

For example:

```bash
echo hello
```

If the command succeeds:

```text
exit status = 0
```

Then:

```bash
echo $?
```

produces:

```text
0
```

---

## 8.1 Example With an Error

For example:

```bash
ls nonexistent_file
```

The command fails and returns a non-zero exit status.

Then:

```bash
echo $?
```

prints that status.

---

## 8.2 Why Is `$?` Special?

For:

```bash
$USER
```

Minishell searches the environment for:

```text
USER
```

But:

```bash
$?
```

does not mean that Minishell should search for a variable named `?`.

It is a **special Shell parameter**.

Therefore it requires separate handling:

```c
if (input[i] == '$' && input[i + 1] == '?')
{
    expand_exit_status();
}
```

---

# 8.3 `$?` Must Be Updated

Consider:

```bash
false
echo $?
```

After `false`:

```text
$? = 1
```

Then:

```bash
echo 1
```

runs successfully.

Therefore, after `echo`:

```text
$? = 0
```

If we then execute:

```bash
echo $?
```

we get:

```text
0
```

So `$?` always reflects the status of the most recently completed command or pipeline according to Shell semantics.

---

# 9. `$` Before an Ordinary Character

Not every `$` starts a parameter expansion.

For example:

```bash
echo $
```

contains a `$` with no valid parameter following it.

In a basic Minishell implementation, it should not be treated as a normal variable reference.

Similarly, some forms such as:

```bash
$!
$#
$-
```

are special parameters in full Bash.

However, Minishell 42 has a limited scope.

You should implement the features required by the project subject rather than trying to reproduce all of Bash.

---

# 9.1 `$` Followed by a Digit

For example:

```bash
echo $1
```

In a full Shell, this can refer to a positional parameter.

However, Minishell does not necessarily need to implement all Bash positional parameter behavior.

Always follow the exact requirements of your project subject.

---

# 10. Expansion Inside and Outside Quotes

This is one of the most important parts of Parameter Expansion.

Compare:

```bash
echo $USER
```

```bash
echo "$USER"
```

and:

```bash
echo '$USER'
```

These have different behavior.

---

## 10.1 Unquoted

```bash
echo $USER
```

Parameter Expansion is performed.

If:

```text
USER=Alice
```

the result is:

```text
Alice
```

---

## 10.2 Double Quotes

```bash
echo "$USER"
```

Parameter Expansion is also performed.

Result:

```text
Alice
```

So:

```text
"$USER"
   ↓
"Alice"
```

---

## 10.3 Single Quotes

```bash
echo '$USER'
```

Parameter Expansion is **not** performed.

The result is:

```text
$USER
```

Conceptually:

```text
'$USER'
    ↓
literal $USER
```

---

## 10.4 The Main Rule

Remember:

```text
Unquoted:
$VAR → expansion

Double quotes:
"$VAR" → expansion

Single quotes:
'$VAR' → NO expansion
```

This rule is fundamental for Minishell.

---

# 11. Double Quotes

Double quotes:

```bash
"..."
```

do **not** disable Parameter Expansion.

For example:

```bash
NAME="Alice"

echo "Hello $NAME"
```

produces:

```text
Hello Alice
```

---

## 11.1 Multiple Variables

For example:

```bash
echo "$USER lives in $HOME"
```

If:

```text
USER=alice
HOME=/home/alice
```

the result is:

```text
alice lives in /home/alice
```

---

# 12. Single Quotes

Single quotes:

```bash
'...'
```

protect their contents from Parameter Expansion.

For example:

```bash
echo '$HOME'
```

produces:

```text
$HOME
```

Compare:

```bash
echo "$HOME"
```

which produces something like:

```text
/home/alice
```

The difference is:

```text
"$HOME"
   ↓
Expansion

'$HOME'
   ↓
Literal text
```

---

# 13. Expansion and Spaces

This is an important part of Shell behavior.

Suppose:

```bash
NAME="Alice Bob"
```

Compare:

```bash
echo $NAME
```

and:

```bash
echo "$NAME"
```

They can behave differently because of **word splitting**.

---

## Unquoted Expansion

```bash
echo $NAME
```

The value:

```text
Alice Bob
```

may be split into two words:

```text
Alice
Bob
```

Conceptually:

```text
argv[0] = "echo"
argv[1] = "Alice"
argv[2] = "Bob"
```

---

## Double-Quoted Expansion

```bash
echo "$NAME"
```

The space is preserved.

The result is one argument:

```text
argv[0] = "echo"
argv[1] = "Alice Bob"
```

---

# 13.1 Why Is This Important?

A naive implementation such as:

```c
str_replace("$NAME", value);
```

is not enough.

The Shell needs to know whether the variable appeared:

```text
unquoted
```

or:

```text
inside double quotes
```

or:

```text
inside single quotes
```

because the resulting arguments can be different.

---

# 14. Expansion and Word Splitting

A simplified model is:

```text
$VAR
  |
  v
value
  |
  v
if unquoted → possible word splitting
```

But:

```text
"$VAR"
   |
   v
value
   |
   v
remains one word
```

---

## Example

Suppose:

```text
NAME="Alice Bob"
```

Then:

```bash
printf '<%s>\n' $NAME
```

can behave as if the command received:

```text
printf '<%s>\n' Alice Bob
```

While:

```bash
printf '<%s>\n' "$NAME"
```

behaves as if the command received:

```text
printf '<%s>\n' "Alice Bob"
```

---

# 15. Expansion and Empty Variables

Suppose:

```bash
export NAME=
```

Then:

```bash
echo $NAME
```

produces an empty result.

But:

```bash
echo "$NAME"
```

represents an explicit empty argument.

This distinction matters when building the final `argv`.

Conceptually:

```text
$NAME
 ↓
empty
 ↓
may disappear in unquoted context

"$NAME"
 ↓
empty string
 ↓
remains as an argument
```

This is one reason why Parameter Expansion cannot be implemented as a simple global string replacement.

---

# 16. Expansion and Command Arguments

Consider:

```bash
NAME=Alice
echo Hello $NAME
```

Before expansion:

```text
echo
Hello
$NAME
```

After expansion:

```text
echo
Hello
Alice
```

The executor can receive:

```c
argv[0] = "echo";
argv[1] = "Hello";
argv[2] = "Alice";
```

---

## 16.1 Multiple Variables

For example:

```bash
FIRST=Alice
LAST=Feofanova
```

Then:

```bash
echo $FIRST $LAST
```

becomes:

```bash
echo Alice Feofanova
```

---

## 16.2 Variable Inside a Word

For example:

```bash
echo hello$USER
```

If:

```text
USER=alice
```

the result is:

```text
helloalice
```

It does not automatically insert a space.

Parameter Expansion can occur in the middle of a word.

---

## 16.3 Prefix and Suffix

For example:

```bash
echo /home/$USER/file
```

becomes:

```text
/home/alice/file
```

Full Shells also support syntax such as:

```bash
${USER}123
```

which explicitly separates the variable name from the following characters.

If your Minishell subject does not require `${...}`, do not implement it unless you intentionally want to extend your project.

---

# 17. Expansion After Redirection

Consider:

```bash
OUTPUT=result.txt
echo hello > $OUTPUT
```

Parameter Expansion changes:

```text
$OUTPUT
```

into:

```text
result.txt
```

Conceptually:

```bash
echo hello > result.txt
```

The redirection then opens:

```text
result.txt
```

---

## Important

The Parser should first recognize:

```text
REDIRECTION
    |
    +-- output redirection
    |
    +-- target WORD
```

The target may contain a parameter reference:

```text
"$OUTPUT"
```

which is later expanded.

Conceptually:

```text
Lexer
  ↓
Parser
  ↓
Redirection structure
  ↓
Expansion
  ↓
Open file
```

---

## 17.1 Empty Redirection Variable

For example:

```bash
OUTPUT=
echo hello > $OUTPUT
```

The expansion produces an empty target.

This is an important edge case and must be handled according to the Shell semantics and the requirements of your Minishell subject.

Do not simply create a file whose name is an empty string.

---

# 18. Expansion in Heredoc

Heredoc has special rules.

Example:

```bash
cat << EOF
$USER
EOF
```

If:

```text
USER=Alice
```

the variable may be expanded:

```text
Alice
```

---

## 18.1 Quoted Heredoc Delimiter

Consider:

```bash
cat << 'EOF'
Hello $USER
EOF
```

Because the delimiter is quoted, expansion inside the heredoc is disabled.

The output is:

```text
Hello $USER
```

---

## 18.2 Why Does This Matter?

Minishell needs to preserve information about the heredoc delimiter.

Conceptually:

```c
typedef struct s_redir
{
    int     type;
    char    *file;
    int     heredoc_quoted;
} t_redir;
```

Then:

```text
HEREDOC
   |
   +-- delimiter
   |
   +-- quoted?
         |
         +-- yes → no expansion
         |
         +-- no → expansion
```

---

# 19. Shell Processing Order

Parameter Expansion is only one part of the Shell processing pipeline.

A simplified conceptual pipeline is:

```text
Raw Input
    |
    v
Lexical Analysis
    |
    v
Tokens
    |
    v
Syntax Validation
    |
    v
Parsing
    |
    v
Parameter Expansion
    |
    v
Word Splitting
    |
    v
Pathname Expansion
    |
    v
Redirections
    |
    v
Execution
```

The exact architecture of your Minishell may differ.

The important idea is:

> `$VAR` should not be blindly expanded before the Shell understands the structure and quoting context of the input.

---

# 19.1 Why Should We Not Expand Everything in the Lexer?

Consider:

```bash
echo '$USER'
```

If the Lexer immediately replaces:

```text
$USER → Alice
```

the token could incorrectly become:

```bash
echo 'Alice'
```

The correct behavior is:

```text
Lexer:
'$USER'
   ↓
WORD with single-quote context

Expansion:
single quote
   ↓
do NOT expand
```

Therefore, the Lexer needs to preserve enough information for later stages to know the quoting context.

---

# 20. Parameter Expansion Algorithm

A simplified algorithm:

```text
1. Get a WORD.

2. Scan the WORD character by character.

3. If a single quote is encountered:
       enter SINGLE_QUOTE context.

4. If a double quote is encountered:
       enter DOUBLE_QUOTE context.

5. If '$' is encountered:

       If current context == SINGLE_QUOTE:
           keep '$' as literal text.

       Otherwise:

           If next character == '?':
               replace with last exit status.

           Else if next character can start a variable name:
               read the variable name.
               search for it in the environment.
               replace it with its value.

           Otherwise:
               treat '$' as an ordinary character.

6. Continue processing the WORD.

7. After expansion, perform the next required
   processing stages, such as word splitting.

8. Pass the final result to the next stage.
```

---

# 20.1 Quote Context

You can represent quote state with:

```c
typedef enum e_quote
{
    NO_QUOTE,
    SINGLE_QUOTE,
    DOUBLE_QUOTE
} t_quote;
```

Conceptually:

```text
NO_QUOTE
   |
   +-- ' → SINGLE_QUOTE
   |
   +-- " → DOUBLE_QUOTE
```

The context determines whether `$` should trigger expansion.

---

# 20.2 Example of Scanning

Input:

```bash
echo "Hello $USER"
```

The Shell sees:

```text
"
↓
DOUBLE_QUOTE

Hello
↓
normal text

$USER
↓
expansion allowed

"
↓
NO_QUOTE
```

If:

```text
USER=Alice
```

the resulting text is:

```text
Hello Alice
```

---

# 21. Pseudocode

A conceptual implementation could look like:

```c
char *expand_word(char *word, t_shell *shell)
{
    char    *result;
    int     i;
    int     quote;

    result = init_string();
    i = 0;
    quote = NO_QUOTE;

    while (word[i])
    {
        update_quote_state(word[i], &quote);

        if (word[i] == '$' && quote != SINGLE_QUOTE)
        {
            if (word[i + 1] == '?')
            {
                append_exit_status(&result, shell->exit_status);
                i += 2;
                continue;
            }

            if (is_valid_var_start(word[i + 1]))
            {
                append_variable_value(
                    &result,
                    word,
                    &i,
                    shell->env
                );
                continue;
            }
        }

        append_char(&result, word[i]);
        i++;
    }

    return (result);
}
```

This is only conceptual pseudocode.

Your actual implementation depends on your Token structure and on when your project removes quote characters.

---

# 22. Data Structures

A useful Shell context can contain both the environment and the last exit status:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
} t_shell;
```

Conceptually:

```text
shell
 |
 +-- env
 |    |
 |    +-- USER=alice
 |    +-- HOME=/home/alice
 |    +-- PATH=/usr/bin:/bin
 |
 +-- exit_status = 0
```

When Minishell encounters:

```bash
$USER
```

it performs:

```text
shell->env
    ↓
find USER
    ↓
get alice
```

When it encounters:

```bash
$?
```

it uses:

```text
shell->exit_status
```

---

# 23. Common Mistakes

## Mistake 1 — Expanding Variables Inside Single Quotes

Wrong:

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

# Mistake 2 — Not Expanding Variables Inside Double Quotes

Wrong:

```bash
echo "$USER"
```

→

```text
$USER
```

Correct:

```text
Alice
```

---

# Mistake 3 — Treating `$?` as a Normal Environment Variable

Wrong:

```text
search_env("?")
```

Correct:

```text
$?
 ↓
shell->exit_status
```

---

# Mistake 4 — Using Simple String Replacement

For example:

```c
str_replace(input, "$USER", value);
```

This approach breaks in cases such as:

```bash
echo '$USER'
```

```bash
echo "$USER"
```

```bash
echo hello$USER
```

```bash
echo "$USER $HOME"
```

and heredocs.

The implementation must understand context.

---

# Mistake 5 — Ignoring Empty Values

For example:

```bash
export NAME=
echo "$NAME"
```

represents an empty argument.

---

# Mistake 6 — Confusing Parameter Expansion With Command Substitution

This:

```bash
$USER
```

is:

```text
Parameter Expansion
```

while:

```bash
$(whoami)
```

is:

```text
Command Substitution
```

These are different Shell mechanisms.

Do not mix them when designing your implementation.

---

# Mistake 7 — Trying to Implement All of Bash

Full Bash supports many parameter forms:

```bash
${VAR}
${VAR:-default}
${VAR:=default}
${VAR:+value}
${VAR:?error}
$1
$2
$#
$@
$*
$!
$$
```

Your Minishell project has a defined scope.

The correct approach is:

```text
Implement the subject requirements
        +
understand the Shell concepts
        +
avoid unnecessary Bash features
```

---

# 24. Testing

Create a dedicated test set for Parameter Expansion.

---

## Test 1 — Basic Variable

```bash
echo $USER
```

---

## Test 2 — HOME

```bash
echo $HOME
```

---

## Test 3 — PATH

```bash
echo $PATH
```

---

## Test 4 — Unknown Variable

```bash
echo $DOES_NOT_EXIST
```

Expected:

```text
empty
```

---

## Test 5 — Empty Variable

```bash
export TEST=
echo $TEST
```

---

## Test 6 — Double Quotes

```bash
echo "$USER"
```

---

## Test 7 — Single Quotes

```bash
echo '$USER'
```

Expected:

```text
$USER
```

---

## Test 8 — Text + Variable

```bash
echo hello$USER
```

---

## Test 9 — Variable + Text

```bash
echo $USER-hello
```

---

## Test 10 — Multiple Variables

```bash
echo $USER $HOME
```

---

## Test 11 — `$?`

```bash
true
echo $?
```

Expected:

```text
0
```

---

## Test 12 — `$?` After an Error

```bash
false
echo $?
```

Expected:

```text
non-zero status
```

---

## Test 13 — Variable Inside Double Quotes

```bash
echo "Hello $USER"
```

---

## Test 14 — Variable Inside Single Quotes

```bash
echo 'Hello $USER'
```

Expected:

```text
Hello $USER
```

---

## Test 15 — Spaces

Create:

```bash
export NAME="Alice Bob"
```

Then compare:

```bash
echo $NAME
```

with:

```bash
echo "$NAME"
```

This should demonstrate the difference between unquoted and quoted expansion.

---

## Test 16 — Redirection

```bash
export FILE=test.txt
echo hello > $FILE
```

---

## Test 17 — Heredoc

```bash
export NAME=Alice

cat << EOF
Hello $NAME
EOF
```

---

## Test 18 — Quoted Heredoc

```bash
export NAME=Alice

cat << 'EOF'
Hello $NAME
EOF
```

Expected:

```text
Hello $NAME
```

---

# 24.1 Test Table

| Input                 | Expansion? | Expected                |
| --------------------- | ---------- | ----------------------- |
| `$USER`               | Yes        | variable value          |
| `"$USER"`             | Yes        | variable value          |
| `'$USER'`             | No         | `$USER`                 |
| `$HOME`               | Yes        | home path               |
| `$NOT_EXIST`          | Yes        | empty                   |
| `$?`                  | Yes        | exit status             |
| `hello$USER`          | Yes        | `hello` + value         |
| `$USER-hello`         | Yes        | value + `-hello`        |
| `$NAME` with spaces   | Yes        | possible word splitting |
| `"$NAME"` with spaces | Yes        | one word                |

---

# 25. Checklist

Before considering Parameter Expansion understood, make sure you can check all of the following.

## Basic

* [ ] I understand what Parameter Expansion is.
* [ ] I understand `$VAR`.
* [ ] I understand how to find a variable in the environment.
* [ ] I understand what happens when a variable does not exist.
* [ ] I understand the difference between unset and empty variables.

---

## `$?`

* [ ] I understand what `$?` means.
* [ ] I know where to store the last exit status.
* [ ] I understand why `$?` is not searched in the environment.
* [ ] I understand when `$?` is updated.

---

## Quotes

* [ ] `$VAR` is expanded when unquoted.
* [ ] `"$VAR"` is expanded.
* [ ] `'$VAR'` is not expanded.
* [ ] I understand quote context.

---

## Word Splitting

* [ ] I understand the difference between `$VAR` and `"$VAR"`.
* [ ] I understand how spaces affect arguments.
* [ ] I understand empty expansions.

---

## Architecture

* [ ] I understand why Expansion should not destroy information needed by the Lexer/Parser.
* [ ] I understand the relationship between Lexer → Parser → Expansion → Execution.
* [ ] I know where the environment is stored.
* [ ] I know where the exit status is stored.

---

## Redirections / Heredoc

* [ ] I understand expansion in redirection targets.
* [ ] I understand expansion in heredocs.
* [ ] I understand the effect of a quoted heredoc delimiter.

---

# 26. Knowledge Check Questions

Try to answer these without looking at the explanations above.

## Basic

1. What is Parameter Expansion?
2. What does `$USER` mean?
3. Where does Minishell store `USER`?
4. What happens to `$ABC` if `ABC` does not exist?
5. What is the difference between an unset variable and an empty variable?

---

## Quotes

6. What does this produce?

```bash
echo "$USER"
```

7. What does this produce?

```bash
echo '$USER'
```

8. Why are the results different?
9. Is `$USER` expanded inside double quotes?
10. Is `$USER` expanded inside single quotes?

---

## Exit Status

11. What does `$?` mean?
12. Where should its value be stored?
13. Why should Minishell not search for `?` in the environment?
14. What happens here?

```bash
true
echo $?
```

15. What happens here?

```bash
false
echo $?
```

---

## Spaces

16. If:

```bash
NAME="Alice Bob"
```

what is the difference between:

```bash
echo $NAME
```

and:

```bash
echo "$NAME"
```

17. Why can quotes affect the number of arguments passed to a command?

---

## Architecture

18. Why is this approach insufficient?

```c
replace("$USER", value);
```

19. At what stage should quote context be considered?
20. Why is this a good test?

```bash
echo '$USER'
```

---

# 27. Main Mental Model

Remember the main processing pipeline:

```text
                    USER INPUT
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
                 +--------------+
                 |    PARSER    |
                 +--------------+
                        |
                        v
                COMMAND STRUCTURE
                        |
                        v
              +--------------------+
              | PARAMETER EXPANSION|
              +--------------------+
                        |
             +----------+----------+
             |                     |
             v                     v
         $VARIABLE               $?
             |                     |
             v                     v
          ENV VALUE          EXIT STATUS
             |                     |
             +----------+----------+
                        |
                        v
                  WORD SPLITTING
                        |
                        v
                   REDIRECTIONS
                        |
                        v
                    EXECUTION
```

---

# The Most Important Rules

## Rule 1 — `$VAR`

```bash
$VAR
```

means:

```text
find VAR
   ↓
get its value
   ↓
replace $VAR with the value
```

---

## Rule 2 — Double Quotes

```bash
"$VAR"
```

still performs Parameter Expansion.

---

## Rule 3 — Single Quotes

```bash
'$VAR'
```

do **not** perform Parameter Expansion.

---

## Rule 4 — `$?`

```bash
$?
```

is a special parameter containing the exit status of the previous command or pipeline.

---

## Rule 5 — Unknown Variables

```bash
echo $UNKNOWN
```

normally expands to:

```text
empty string
```

---

## Rule 6 — Context Matters

```text
unquoted
    ↓
parameter expansion
    ↓
possible word splitting

double quoted
    ↓
parameter expansion
    ↓
preserves the result as one word

single quoted
    ↓
no parameter expansion
```

---

# Final Concept

Parameter Expansion in Minishell is **not simply**:

```text
find "$"
    ↓
replace text
```

It is a context-sensitive process:

```text
             $VAR
               |
               v
        Is it inside quotes?
          /           \
         /             \
    single quote    other context
         |               |
         v               v
   NO EXPANSION      EXPANSION
                         |
              +----------+----------+
              |                     |
             $VAR                  $?
              |                     |
              v                     v
           env value          exit status
              |
              v
       word splitting
        if applicable
```

The main idea to remember is:

> **Parameter Expansion replaces `$VAR` with the value of the parameter, but the final result depends on the quoting context and on whether the parameter is a normal environment variable or a special parameter such as `$?`.**

For Minishell, you should be able to explain this complete flow:

```text
$USER
  ↓
Lexer preserves the necessary context
  ↓
Parser creates a WORD / command structure
  ↓
Expansion detects $USER
  ↓
Searches USER in the environment
  ↓
Retrieves its value
  ↓
Applies quote rules
  ↓
Produces the final argument(s)
  ↓
Executor runs the command
```

The next logical topics to study are:

```text
Parameter Expansion
        ↓
Word Splitting
        ↓
Pathname Expansion
        ↓
Redirections
        ↓
Execution
```

Together, these stages form a major part of the **Shell Processing Pipeline**.
