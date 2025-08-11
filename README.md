# UNIX-Shell: An Operating Systems Project

This project is a custom UNIX shell implemented in C, developed as a practical assignment for the Operating Systems course at the University of Málaga. The shell demonstrates core concepts of process and job control, signal handling, I/O redirection, and inter-process communication.

It serves as a command-line interface that allows a user to execute programs and manage tasks, supporting both foreground and background execution, job suspension and resumption, and several advanced features.

## Project File Structure

*   `Shell_project.c`: The main source file containing the shell's primary loop, command parsing, and logic for handling user input and launching processes.
*   `job_control.c`: Implements the data structures and functions for managing the job list (adding, deleting, searching for jobs) and handling process states.
*   `job_control.h`: The header file for the job control module, defining the `job` structure, function prototypes, and useful macros.
*   `parse_redir.h`: A helper header containing the function to parse I/O redirection operators (`<` and `>`) from the command line.
  
## Features

This shell implements a range of standard and custom functionalities:

### Core Shell Features
*   **Command Execution**: Executes external commands in both foreground and background modes (`&`).
*   **Process Control**: Utilizes `fork()` and `execvp()` to create and run new processes.
*   **Job Control**: A complete job control system to manage processes:
    *   `jobs`: Lists all currently running background and stopped jobs.
    *   `fg`: Resumes a stopped or background job and brings it to the foreground.
    *   `bg`: Resumes a stopped job and puts it into the background.
*   **Signal Handling**: Manages signals like `SIGINT`, `SIGTSTP`, and `SIGCHLD` to handle process interruptions, stops, and terminations gracefully.
*   **I/O Redirection**: Supports standard input (`<`) and standard output (`>`) redirection to and from files.
*   **Built-in Commands**: Includes the internal command `cd` to change the current working directory.

### Implemented Extensions
*   **Respawnable Mode (`+`)**: A special background execution mode. If a job launched with a `+` at the end terminates for any reason, the shell will automatically restart it with the same command and arguments.
*   **Process Timeouts (`alarm-thread`)**: An internal command that launches a job with a watchdog timer. A detached pthread sends a `SIGKILL` signal to terminate the process if it exceeds a specified time limit, preventing hung or runaway processes.
*   **Delayed Command Execution (`delay-thread`)**: An internal command that executes a job in the background after a specified number of seconds. This is implemented using detached pthreads to avoid blocking the shell.
*   **Signal Masking (`mask`)**: An internal command that executes a given command while masking (blocking) a specified list of signals. This prevents the executed process from being affected by those signals.

## How to Compile and Run

### Compilation
To compile the shell, you need a C compiler (`gcc`) and to link the pthreads library (for the `delay-thread` feature). Run the following command in your terminal:

```bash
gcc Shell_project.c job_control.c -o shell 
```

### Execution
To start the shell, run the compiled executable:

```bash
./shell
```
You will then be greeted by the `COMMAND->` prompt. To exit the shell, type `Ctrl+D`.

## Usage Examples

Here are some examples of how to use the shell's features:

### Execute 'ls -l' in the foreground
COMMAND->ls -l

### Execute 'xclock' in the background
COMMAND->xclock &
```

**Job Control**
```bash
# List all jobs
COMMAND->jobs

# Bring the most recent job to the foreground
COMMAND->fg

# Bring job number 2 to the foreground
COMMAND->fg 2

# Resume the most recent stopped job in the background
COMMAND->bg
```

**Respawnable Mode**
```bash
# Run xclock in respawnable mode. If you close the clock, it will reopen.
COMMAND->xclock -update 1 +
```

**Process Timeout**

```bash
#  Run xclock that will be automatically killed after 30 seconds if it's still running.
COMMAND-> alarm-thread 30 xclock –update 1
```

**Delayed Execution**
```bash
# Launch xeyes after a 10-second delay
COMMAND->delay-thread 10 xeyes
```

**Signal Masking**
```bash
# Run a command while blocking the SIGINT signal (Ctrl+C, signal no. 2)
# The command will not terminate if you press Ctrl+C
COMMAND->mask 2 -c some_command
```


