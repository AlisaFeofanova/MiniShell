# Understanding `$?` in Minishell

## Table of Contents

* [1. What is `$?`?](#1-what-is-)
* [2. What is an Exit Status?](#2-what-is-an-exit-status)
* [3. Basic Examples](#3-basic-examples)
* [4. `$?` Is a Special Parameter](#4--is-a-special-parameter)
* [5. How `$?` Works](#5-how--works)
* [6. The `exit_status` Variable](#6-the-exit_status-variable)
* [7. When Must `$?` Be Updated?](#7-when-must--be-updated)
* [8. Builtins and `$?`](#8-builtins-and-)
* [9. External Commands and `$?`](#9-external-commands-and-)
* [10. Pipelines and `$?`](#10-pipelines-and-)
* [11. `$?` Inside Quotes](#11--inside-quotes)
* [12. `$?` Inside Single Quotes](#12--inside-single-quotes)
* [13. `$?` Inside Double Quotes](#13--inside-double-quotes)
* [14. `$?` With Other Text](#14--with-other-text)
* [15. `$?` and Command Execution](#15--and-command-execution)
* [16. `$?` After `echo`](#16--after-echo)
* [17. `$?` After `cd`](#17--after-cd)
* [18. `$?` After `export`](#18--after-export)
* [19. `$?` After `unset`](#19--after-unset)
* [20. `$?` After `exit`](#20--after-exit)
* [21. `$?` and Errors](#21--and-errors)
* [22. `$?` and Signals](#22--and-signals)
* [23. `$?` in Minishell Architecture](#23--in-minishell-architecture)
* [24. Implementation Strategy](#24-implementation-strategy)
* [25. Pseudocode](#25-pseudocode)
* [26. Common Mistakes](#26-common-mistakes)
* [27. Testing](#27-testing)
* [28. Checklist](#28-checklist)
* [29. Knowledge Check](#29-knowledge-check)
* [30. Main Mental Model](#30-main-mental-model)

---

# 1. What is `$?`

`$?` is a **special Shell parameter**.

It contains the **exit status of the most recently executed command or pipeline**.

Example:

```bash
true
echo $?
```

Output:

```text
0
```

Why?

Because:

```bash
true
```

successfully finished.

Its exit status is:

```text
0
```

Therefore:

```text
$?
 ↓
0
```

---

# 2. What is an Exit Status?

Every command executed by the Shell finishes with a numeric status.

This status tells the Shell whether the command succeeded or failed.

The most important convention is:

```text
0       → success
non-zero → error / failure
```

For example:

```bash
true
```

returns:

```text
0
```

while:

```bash
false
```

returns:

```text
1
```

Therefore:

```bash
false
echo $?
```

produces:

```text
1
```

---

# 2.1 Exit Status Is an Integer

Conceptually:

```c
int exit_status;
```

For example:

```text
exit_status = 0;
```

or:

```text
exit_status = 1;
```

or:

```text
exit_status = 127;
```

The Shell stores this value and makes it available through:

```bash
$?
```

---

# 3. Basic Examples

## Successful Command

```bash
echo hello
echo $?
```

The first `echo` normally succeeds:

```text
0
```

So:

```text
$? → 0
```

---

## Failed Command

```bash
ls /does/not/exist
echo $?
```

The `ls` command fails.

Therefore `$?` contains a non-zero value.

---

## `true`

```bash
true
echo $?
```

Result:

```text
0
```

---

## `false`

```bash
false
echo $?
```

Result:

```text
1
```

---

# 4. `$?` Is a Special Parameter

It is very important to understand that:

```bash
$?
```

is **not** an ordinary environment variable.

For example:

```bash
$USER
```

means:

```text
find the variable USER
```

But:

```bash
$?
```

means:

```text
get the Shell's current exit status
```

Therefore, Minishell should **not** do:

```c
get_env_value(env, "?");
```

Instead:

```c
shell->exit_status;
```

should be used.

---

# 4.1 Normal Variable vs `$?`

Compare:

```bash
echo $USER
```

with:

```bash
echo $?
```

For `$USER`:

```text
$USER
  ↓
search environment
  ↓
USER
  ↓
value
```

For `$?`:

```text
$?
 ↓
Shell state
 ↓
exit_status
```

---

# 5. How `$?` Works

Imagine Minishell has:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
} t_shell;
```

Initially:

```text
exit_status = 0
```

Then the user executes:

```bash
false
```

The command returns:

```text
1
```

Minishell updates:

```text
shell->exit_status = 1;
```

Now the user executes:

```bash
echo $?
```

During Parameter Expansion:

```text
$?
 ↓
shell->exit_status
 ↓
1
```

The command becomes conceptually:

```bash
echo 1
```

Then `echo` itself succeeds.

Therefore:

```text
shell->exit_status = 0
```

after `echo`.

---

# 6. The `exit_status` Variable

A very useful design is to keep the last exit status in the main Shell structure.

For example:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
    int     last_pid;
} t_shell;
```

The important field here is:

```c
int exit_status;
```

This represents:

```text
the status available through $?
```

---

## Example

After:

```bash
false
```

the Shell state is:

```text
shell
 |
 +-- exit_status = 1
```

Then:

```bash
echo $?
```

becomes:

```text
echo 1
```

After `echo` succeeds:

```text
shell
 |
 +-- exit_status = 0
```

---

# 7. When Must `$?` Be Updated?

The exit status must be updated after commands execute.

For example:

```bash
true
```

updates:

```text
exit_status = 0
```

Then:

```bash
false
```

updates:

```text
exit_status = 1
```

Then:

```bash
echo hello
```

normally updates:

```text
exit_status = 0
```

So the value changes continuously.

---

## Important Rule

The next command can overwrite `$?`.

For example:

```bash
false
echo hello
echo $?
```

The result is normally:

```text
0
```

not:

```text
1
```

Why?

Because:

```text
false
 ↓
exit_status = 1

echo hello
 ↓
exit_status = 0

echo $?
 ↓
0
```

---

# 8. Builtins and `$?`

Builtins also have exit statuses.

Important Minishell builtins include:

```text
echo
cd
pwd
export
unset
env
exit
```

Each builtin must produce an appropriate status.

---

## Example: `echo`

```bash
echo hello
echo $?
```

Normally:

```text
0
```

---

## Example: `cd`

Successful:

```bash
cd /tmp
echo $?
```

Normally:

```text
0
```

Failed:

```bash
cd /does/not/exist
echo $?
```

The result should be non-zero.

---

## Example: `export`

A valid export:

```bash
export NAME=Alice
echo $?
```

normally gives:

```text
0
```

An invalid export argument should produce a non-zero status.

---

# 9. External Commands and `$?`

External programs also return exit statuses.

For example:

```bash
/bin/true
echo $?
```

returns:

```text
0
```

And:

```bash
/bin/false
echo $?
```

returns:

```text
1
```

Minishell gets the child's termination information using:

```c
waitpid()
```

For example:

```c
int status;

waitpid(pid, &status, 0);
```

Then macros such as:

```c
WIFEXITED(status)
WEXITSTATUS(status)
```

can be used.

Conceptually:

```text
child process
     |
     v
   exits
     |
     v
 parent waitpid()
     |
     v
extract exit status
     |
     v
shell->exit_status
```

---

# 10. Pipelines and `$?`

Pipelines are important.

Consider:

```bash
false | true
echo $?
```

The Shell executes:

```text
false
  |
  v
true
```

For normal Shell pipeline semantics, `$?` represents the status of the **last command in the pipeline**.

Therefore:

```text
false | true
           ↑
      last command
```

`true` returns:

```text
0
```

So:

```text
$? → 0
```

---

## Another Example

```bash
true | false
echo $?
```

The last command is:

```text
false
```

which returns:

```text
1
```

Therefore:

```text
$? → 1
```

---

# 10.1 Pipeline Mental Model

For:

```bash
cmd1 | cmd2 | cmd3
```

think:

```text
cmd1 → status1
cmd2 → status2
cmd3 → status3

          ↓

      pipeline status

          ↓

       status3
```

For a basic Minishell implementation:

```text
pipeline exit status
        =
status of last command
```

---

# 11. `$?` Inside Quotes

`$?` follows the same important quote rules as other parameter expansions.

---

## Unquoted

```bash
echo $?
```

Expansion occurs.

---

## Double Quotes

```bash
echo "$?"
```

Expansion also occurs.

For example:

```text
exit_status = 42
```

then:

```bash
echo "$?"
```

becomes conceptually:

```bash
echo "42"
```

---

## Single Quotes

```bash
echo '$?'
```

Expansion does **not** occur.

The result is:

```text
$?
```

This is exactly the same principle as:

```bash
echo '$USER'
```

---

# 12. `$?` Inside Single Quotes

Example:

```bash
false
echo '$?'
```

Output:

```text
$?
```

Why?

Because everything inside single quotes is treated literally.

Conceptually:

```text
'$?'
  ↓
literal text
  ↓
$?
```

There is no Parameter Expansion.

---

# 13. `$?` Inside Double Quotes

Example:

```bash
false
echo "$?"
```

The value is expanded.

If:

```text
exit_status = 1
```

then:

```text
"$?"
```

becomes:

```text
"1"
```

Result:

```text
1
```

---

# 14. `$?` With Other Text

Parameter Expansion can occur in the middle of a word.

For example:

```bash
false
echo status=$?
```

If:

```text
$? = 1
```

the result is:

```text
status=1
```

Another example:

```bash
echo "exit status: $?"
```

produces:

```text
exit status: 1
```

---

## Prefix and Suffix

```bash
echo xxx$?yyy
```

The Shell must determine the `$?` expansion and preserve the surrounding text.

Conceptually:

```text
xxx + exit_status + yyy
```

If the status is `1`:

```text
xxx1yyy
```

---

# 15. `$?` and Command Execution

A useful way to understand Minishell is:

```text
USER INPUT
    |
    v
LEXER
    |
    v
PARSER
    |
    v
EXPANSION
    |
    +---- $?
    |      |
    |      v
    |   exit_status
    |
    v
FINAL ARGUMENTS
    |
    v
EXECUTION
    |
    v
NEW EXIT STATUS
    |
    v
shell->exit_status
```

The important cycle is:

```text
old exit status
       |
       v
    expand $?
       |
       v
 execute command
       |
       v
new exit status
       |
       v
store it
```

---

# 16. `$?` After `echo`

Consider:

```bash
false
echo $?
```

Before `echo`:

```text
exit_status = 1
```

Parameter Expansion:

```text
$?
 ↓
1
```

Command becomes:

```bash
echo 1
```

`echo` succeeds.

Therefore after execution:

```text
exit_status = 0
```

This is extremely important.

---

# 16.1 Another Example

```bash
false
echo $?
echo $?
```

First:

```bash
false
```

gives:

```text
exit_status = 1
```

First `echo` expands:

```text
echo 1
```

and succeeds.

Therefore the next `$?` sees:

```text
0
```

Output:

```text
1
0
```

---

# 17. `$?` After `cd`

Successful:

```bash
cd /tmp
echo $?
```

Expected:

```text
0
```

Failed:

```bash
cd /does/not/exist
echo $?
```

Expected:

```text
non-zero
```

The exact error message is separate from the exit status.

Think:

```text
cd
 |
 +-- prints error if needed
 |
 +-- returns status
       |
       v
shell->exit_status
```

---

# 18. `$?` After `export`

Valid:

```bash
export TEST=hello
echo $?
```

Normally:

```text
0
```

Invalid:

```bash
export 123=hello
echo $?
```

should produce a non-zero status.

The important thing is that the builtin must communicate its result back to the Shell.

---

# 19. `$?` After `unset`

For example:

```bash
unset TEST
echo $?
```

If the command succeeds:

```text
0
```

Again:

```text
builtin
   |
   v
return status
   |
   v
shell->exit_status
```

---

# 20. `$?` After `exit`

`exit` is special because it terminates Minishell.

For example:

```bash
exit 42
```

the Shell terminates with status:

```text
42
```

You will not normally get another interactive command:

```bash
echo $?
```

inside the same Shell because the Shell has exited.

However, the parent process or test framework can observe the exit status.

---

# 21. `$?` and Errors

Different errors can produce different non-zero statuses.

Examples include:

```text
command not found
permission denied
file not found
invalid builtin argument
syntax error
```

For Minishell, you need to implement the statuses required by the project and reproduce Bash behavior where the subject/tests expect it.

---

## Command Not Found

For example:

```bash
does_not_exist
echo $?
```

A common Shell result is:

```text
127
```

This is conventionally used for:

```text
command not found
```

---

## Permission Denied

For example, attempting to execute something without the required permissions can produce a different non-zero status, commonly:

```text
126
```

The important distinction is:

```text
127 → command could not be found
126 → command was found but could not be executed
```

Your Minishell should handle these cases consistently with the expected Shell behavior.

---

# 22. `$?` and Signals

Signals introduce another important situation.

A process can terminate because of a signal rather than by calling:

```c
exit()
```

For example:

```text
SIGINT
SIGQUIT
```

Minishell needs to handle these according to the project requirements.

For child processes, you can inspect:

```c
WIFSIGNALED(status)
```

and:

```c
WTERMSIG(status)
```

Conceptually:

```text
child
  |
  +-- normal exit
  |      |
  |      v
  |   WEXITSTATUS()
  |
  +-- signal
         |
         v
     WTERMSIG()
```

The Shell then converts that information into the appropriate `$?` value.

---

# 22. `$?` in Minishell Architecture

A clean architecture could look like:

```text
                     +----------------+
                     |    t_shell     |
                     +----------------+
                     | env            |
                     | exit_status    |
                     +----------------+
                              |
                              |
             +----------------+----------------+
             |                                 |
             v                                 v
       Parameter Expansion                Execution
             |                                 |
             |                                 v
             |                           command runs
             |                                 |
             |                                 v
             |                           command status
             |                                 |
             +-------------<-------------------+
                           update
```

---

# 22.1 Suggested Structure

For example:

```c
typedef struct s_shell
{
    t_env   *env;
    int     exit_status;
} t_shell;
```

Then:

```c
shell->exit_status
```

is the central source of truth for:

```bash
$?
```

---

# 23. Implementation Strategy

The cleanest approach is to handle `$?` during Parameter Expansion.

Suppose the input contains:

```bash
echo $?
```

The expansion code detects:

```text
$
+
?
```

and replaces the pair with the current status.

Conceptually:

```c
if (str[i] == '$' && str[i + 1] == '?')
{
    append_number(&result, shell->exit_status);
    i += 2;
}
```

---

# 23.1 Why Handle `$?` Separately?

Because `$?` is not an environment variable.

For:

```text
$USER
```

you need:

```text
environment lookup
```

For:

```text
$?
```

you need:

```text
shell state lookup
```

Therefore:

```c
if (next == '?')
    expand_exit_status();
else
    expand_environment_variable();
```

---

# 24. Pseudocode

A simplified implementation:

```c
char *expand_word(char *word, t_shell *shell)
{
    char    *result;
    int     i;

    result = create_empty_string();
    i = 0;

    while (word[i])
    {
        if (word[i] == '$' && word[i + 1] == '?')
        {
            append_number(&result, shell->exit_status);
            i += 2;
        }
        else
        {
            append_char(&result, word[i]);
            i++;
        }
    }

    return (result);
}
```

This is intentionally simplified.

A real implementation must also handle:

```text
single quotes
double quotes
normal variables
empty variables
word boundaries
```

---

# 24.1 Quote-Aware Version

Conceptually:

```c
while (word[i])
{
    if (word[i] == '\'' && !double_quoted)
    {
        toggle_single_quote();
        i++;
    }
    else if (word[i] == '"' && !single_quoted)
    {
        toggle_double_quote();
        i++;
    }
    else if (word[i] == '$'
        && word[i + 1] == '?'
        && !single_quoted)
    {
        append_number(&result, shell->exit_status);
        i += 2;
    }
    else
    {
        append_char(&result, word[i]);
        i++;
    }
}
```

The key condition is:

```c
!single_quoted
```

because `$?` must not expand inside single quotes.

---

# 25. Common Mistakes

## Mistake 1 — Treating `$?` as an Environment Variable

Wrong:

```c
get_env_value(env, "?");
```

Correct:

```c
shell->exit_status;
```

---

## Mistake 2 — Forgetting to Update the Status

Wrong architecture:

```text
command executes
     |
     v
status is calculated
     |
     X
not stored
```

Correct:

```text
command executes
     |
     v
status
     |
     v
shell->exit_status
```

---

## Mistake 3 — Updating `$?` Too Early

Suppose:

```bash
false
echo $?
```

You must expand `$?` using:

```text
1
```

**before** `echo` changes the Shell's status.

Correct sequence:

```text
false
 ↓
status = 1
 ↓
expand $?
 ↓
echo 1
 ↓
echo succeeds
 ↓
status = 0
```

---

## Mistake 4 — Expanding `$?` Inside Single Quotes

Wrong:

```bash
echo '$?'
```

→

```text
0
```

Correct:

```text
$?
```

---

## Mistake 5 — Not Updating After Builtins

For example:

```bash
cd /invalid
echo $?
```

If `cd` fails, Minishell must store the failure status.

---

## Mistake 6 — Incorrect Pipeline Status

For:

```bash
false | true
echo $?
```

the pipeline status should correspond to the last command under the standard Minishell/Bash behavior:

```text
true → 0
```

---

## Mistake 7 — Confusing Printed Errors With Exit Status

These are different:

```text
stderr message
```

and:

```text
exit status
```

For example:

```text
cd: no such file or directory
```

is an error message.

The corresponding numeric result is a separate piece of information.

---

# 26. Testing

Build a dedicated test suite.

---

## Test 1 — `true`

```bash
true
echo $?
```

Expected:

```text
0
```

---

## Test 2 — `false`

```bash
false
echo $?
```

Expected:

```text
1
```

---

## Test 3 — Successful `echo`

```bash
echo hello
echo $?
```

Expected:

```text
0
```

---

## Test 4 — Unknown Command

```bash
command_that_does_not_exist
echo $?
```

Check that your result matches the expected Shell behavior.

---

## Test 5 — Single Quotes

```bash
false
echo '$?'
```

Expected:

```text
$?
```

---

## Test 6 — Double Quotes

```bash
false
echo "$?"
```

Expected:

```text
1
```

---

## Test 7 — Text Around `$?`

```bash
false
echo status=$?
```

Expected:

```text
status=1
```

---

## Test 8 — Multiple `$?`

```bash
false
echo "$?" "$?"
```

Both expansions use the same status before `echo` executes.

Expected:

```text
1 1
```

Then:

```bash
echo $?
```

should normally produce:

```text
0
```

because the previous `echo` succeeded.

---

## Test 9 — Pipeline

```bash
false | true
echo $?
```

Expected:

```text
0
```

---

## Test 10 — Reverse Pipeline

```bash
true | false
echo $?
```

Expected:

```text
1
```

---

## Test 11 — `cd`

```bash
cd /tmp
echo $?
```

Expected:

```text
0
```

---

## Test 12 — Failed `cd`

```bash
cd /does/not/exist
echo $?
```

Expected:

```text
non-zero
```

---

## Test 13 — Empty Command

Test how your Minishell handles:

```bash
echo $?
```

immediately after starting the Shell.

The initial status should be initialized consistently with your project expectations.

---

# 27. Test Table

| Command           | Meaning               |    Typical `$?` |
| ----------------- | --------------------- | --------------: |
| `true`            | success               |             `0` |
| `false`           | failure               |             `1` |
| `echo hello`      | successful builtin    |             `0` |
| `echo "$?"`       | print previous status | previous status |
| `'$?'`            | literal text          |    no expansion |
| `"$?"`            | expand status         | previous status |
| `false \| true`   | pipeline              |             `0` |
| `true \| false`   | pipeline              |             `1` |
| command not found | command missing       |           `127` |
| permission denied | cannot execute        |  commonly `126` |

Exact behavior should be checked against your 42 subject/test expectations.

---

# 28. Checklist

## Basic Understanding

* [ ] I know what `$?` means.
* [ ] I understand what an exit status is.
* [ ] I know that `0` normally means success.
* [ ] I know that non-zero means failure/error.

---

## Implementation

* [ ] I have an `exit_status` field in my Shell state.
* [ ] `$?` does not use environment lookup.
* [ ] `$?` is handled during Parameter Expansion.
* [ ] The current status is converted to a string.
* [ ] The resulting string is inserted into the command.

---

## Quotes

* [ ] `$?` expands without quotes.
* [ ] `$?` expands inside double quotes.
* [ ] `$?` does not expand inside single quotes.

---

## Execution

* [ ] Builtins update the exit status.
* [ ] External commands update the exit status.
* [ ] `waitpid()` is used correctly.
* [ ] Child exit status is extracted correctly.
* [ ] Pipeline status is handled correctly.
* [ ] Signal termination is handled.

---

## Timing

* [ ] `$?` is expanded using the status from the previous command.
* [ ] The status is updated only after the current command finishes.
* [ ] I understand why `echo $?` prints the previous command's status.
* [ ] I understand why the next `$?` usually becomes `0` after a successful `echo`.

---

# 29. Knowledge Check

Try answering these without looking above.

### Question 1

What does:

```bash
$?
```

represent?

---

### Question 2

Is `$?` an environment variable?

---

### Question 3

Where should Minishell store the value of `$?`?

---

### Question 4

What is the result?

```bash
true
echo $?
```

---

### Question 5

What is the result?

```bash
false
echo $?
```

---

### Question 6

What happens here?

```bash
false
echo $?
echo $?
```

Why?

---

### Question 7

Does this expand `$?`?

```bash
echo '$?'
```

---

### Question 8

Does this expand `$?`?

```bash
echo "$?"
```

---

### Question 9

Why can't you implement `$?` with:

```c
get_env_value(env, "?");
```

---

### Question 10

What should happen conceptually here?

```bash
false | true
echo $?
```

---

### Question 11

What should happen conceptually here?

```bash
true | false
echo $?
```

---

### Question 12

Why must `$?` be expanded before the `echo` command updates the Shell's status?

---

# 30. Main Mental Model

The most important thing to remember is this:

```text
                 COMMAND
                    |
                    v
                 execute
                    |
                    v
              exit status
                    |
                    v
          shell->exit_status
                    |
                    v
                  $?
```

Then the next command uses that value:

```text
false
  |
  v
exit_status = 1
  |
  v
echo $?
  |
  v
Parameter Expansion
  |
  v
echo 1
  |
  v
echo succeeds
  |
  v
exit_status = 0
```

---

# The Complete Cycle

Think about `$?` as a **one-step memory of command execution**:

```text
┌───────────────────────┐
│   Execute command     │
└───────────┬───────────┘
            ↓
┌───────────────────────┐
│  Get command status   │
└───────────┬───────────┘
            ↓
┌───────────────────────┐
│ shell->exit_status    │
└───────────┬───────────┘
            ↓
        user types
            ↓
┌───────────────────────┐
│        echo $?        │
└───────────┬───────────┘
            ↓
┌───────────────────────┐
│ Parameter Expansion   │
└───────────┬───────────┘
            ↓
       replace `$?`
            ↓
┌───────────────────────┐
│       echo 1          │
└───────────┬───────────┘
            ↓
┌───────────────────────┐
│  command succeeds     │
└───────────┬───────────┘
            ↓
┌───────────────────────┐
│ exit_status = 0       │
└───────────────────────┘
```

## One Sentence to Remember

> **`$?` is a special Shell parameter that contains the exit status of the previous command or pipeline, and Minishell must store that status in its Shell state and expand `$?` before executing the next command.**

For Minishell, the core implementation is:

```text
command execution
       ↓
obtain status
       ↓
shell->exit_status
       ↓
parameter expansion
       ↓
$? → string representation of exit_status
       ↓
execute next command
       ↓
update exit_status again
```
