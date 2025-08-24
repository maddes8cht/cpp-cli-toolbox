# At is `On`

When trying to use the old `at` command in the Windows CMD shell, I realized that it was deprecated. It was recommended to use the Windows Task Scheduler instead. For some really simple tasks, this is way too much overhead, and I still wanted something as simple as the old `at` command.

### Solution

The old `at` command in Windows is deprecated and no longer works, only displaying a message directing users to use `schtasks.exe` instead. Since replacing `at` with a new tool of the same name is impractical due to its presence in the Windows system, I chose a different name: `on`. While `on` might sound like slightly "wrong" English, it’s short, memorable, and gets the job done.

### What it looks like
`On` may display a progress bar and/or a text display showing the remaining time like this:
```
>> on -o b -d 10
[################################\.................] Remaining: 00:00:03
```

### Usage

```
on [options] Time [Command [CommandArgs...]]
```

- **Time format**:
  - For clock time (default): `hh`, `hh:mm`, or `hh:mm:ss` (e.g., `12`, `12:30`, `12:30:45`).
  - For delay mode (`-d`): `hh:mm:ss`, `mm:ss`, or `ss` (e.g., `01:00:00`, `1:20`, `15`). The leading unit can exceed standard limits and will be normalized (e.g., `90` as 1:30, `120:00` as 2:00:00, `26:00:00` as 26:00:00); subsequent units must be 0-59.
- **Command**: Optional command to execute, followed by its arguments. Commands with spaces or special characters may require double quotes (`"`) depending on the shell. If no command is provided, the program only displays the countdown or progress bar (or nothing if `-o n` is used).
- **Options**:
  - `-h`, `--help`: Show the full help message.
  - `-d`, `--delay`: Interpret `Time` as a duration instead of a clock time.
  - `-c`, `--no-clear`: Disable in-place countdown, printing a new line for each update.
  - `-o`, `--output=MODE`: Set output mode: `time` (countdown timer), `progress` (progress bar), `both` (timer + bar), `none` (no output). Short forms: `t`, `p`, `b`, `n`.
  - `-l`, `--length=NUM`: Set progress bar length (default: 50, range: 5–300).
- The program can be interrupted at any time with `Ctrl+C`.

### Examples

- `on 12:30 "echo Hello World"`: Executes `echo Hello World` at 12:30 with a countdown timer.
- `on 14:20 ls -atl`: Executes `ls -atl` at 14:20 with a countdown timer.
- `on -d 15 dir`: Executes `dir` after 15 seconds with a countdown timer.
- `on -d 1:20 dir`: Executes `dir` after 1 minute and 20 seconds with a countdown timer.
- `on -o p -d 15 dir`: Executes `dir` after 15 seconds, showing a 50-character progress bar.
- `on -o p -l 100 -d 10`: Shows a 100-character progress bar for 10 seconds, no command executed.
- `on -o b -l 15 -d 1:20 dir`: Executes `dir` after 1 minute and 20 seconds, showing a 15-character progress bar and countdown timer.
- `on -o n -d 10`: Blocks the terminal for 10 seconds with no output, no command executed.
- `on -d 90`: Waits 1 minute and 30 seconds with a countdown timer, no command executed.
- `on -d 120:00`: Waits 2 hours with a countdown timer, no command executed.
- `on -d 26:00:00`: Waits 26 hours with a countdown timer, no command executed.
- `on --help`: Shows the full help message.

### Features

- **Simple and Lightweight**: Replaces the deprecated `at` command with minimal overhead.
- **Flexible Time Formats**: Supports both clock times and durations for scheduling, with normalization for large leading units in delay mode.
- **Visual Feedback**: Optional progress bar (updated every 125 ms) and countdown timer for a clear user experience.
- **Customizable**: Adjust the progress bar length and output mode to suit your needs.
- **Windows-Compatible**: Works in Windows environments. Should compile and work on any OS, as it only uses C++ with standard libraries.
- **Interruptible**: Can be stopped at any time with `Ctrl+C`.