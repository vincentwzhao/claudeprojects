# taskbot

A personal recurring task/reminder bot, as a local CLI application. Tasks are
stored in a local SQLite database; taskbot can remind you with desktop
notifications or email when something is due.

## Setup

Requires Python 3.9+.

```bash
cd recurring-task-bot
python3 -m venv .venv
source .venv/bin/activate        # Windows: .venv\Scripts\activate
pip install -e .
```

This installs the `taskbot` command (and its one dependency, [plyer](https://github.com/kivy/plyer),
used for cross-platform desktop notifications). Everything else is Python
standard library.

All state lives in `~/.taskbot/` by default (`tasks.db` and, optionally,
`config.ini`). Set the `TASKBOT_HOME` environment variable to use a
different location.

## Commands

```bash
taskbot add                       # interactive: prompts for name, recurrence, notification method
taskbot add --name "Water plants" --recurrence daily --notify-method desktop

taskbot list                      # all tasks, soonest due first

taskbot done <id>                 # mark complete; auto-computes the next due date
taskbot done <id> --date 2026-01-15   # backdate the completion

taskbot edit <id>                 # interactive edit (Enter keeps current value)
taskbot edit <id> --name "New name" --notify-method email

taskbot delete <id>               # prompts for confirmation unless -y is given

taskbot snooze <id> <days>        # push the due date back N days, without marking done

taskbot check                     # scan for due/overdue tasks and fire notifications
taskbot check --force             # re-notify even if already notified today

taskbot run                       # run a single check (same as `taskbot check`)
taskbot run --daemon              # loop forever, checking every --interval minutes (default 60)
taskbot run --daemon --interval 15
```

Run any command with `--help` for the full flag list, e.g. `taskbot add --help`.

### Recurrence types

- `daily` — every day
- `weekly` — a specific weekday: `--recurrence-value mon` (or `0`-`6`, `0`=Monday)
- `monthly` — a specific day of month: `--recurrence-value 15`. If the day
  doesn't exist in a given month (e.g. day 31 in April, or day 29 in a
  non-leap February), it's clamped to that month's last day.
- `custom` — every N days: `--recurrence-value 3` for every 3 days

The next due date is always computed from the date you mark a task `done`
(defaulting to today), not from the previous due date — so completing a
task early or late doesn't cause the schedule to drift.

## Notifications

### Desktop

Desktop notifications use [plyer](https://github.com/kivy/plyer), which
wraps the native notification system on macOS, Windows, and Linux
(via `notify-send`/D-Bus). If plyer can't find a backend (e.g. a
headless Linux box with no notification daemon), taskbot falls back to a
native call (`osascript` on macOS, `notify-send` on Linux, a PowerShell
balloon tip on Windows) and prints a clear warning if none of that works,
rather than crashing.

### Email

Copy `config.example.ini` to `~/.taskbot/config.ini` (or `$TASKBOT_HOME/config.ini`)
and fill in your SMTP details:

```ini
[email]
smtp_host = smtp.gmail.com
smtp_port = 587
username = your_email@gmail.com
password = your_app_password
from_addr = your_email@gmail.com
to_addr = your_email@gmail.com
use_tls = true
```

For Gmail, use an [App Password](https://myaccount.google.com/apppasswords)
(Google Account → Security → 2-Step Verification → App passwords) rather
than your normal account password — Gmail rejects plain-password SMTP
logins. Any SMTP provider works the same way (host, port, credentials).

`config.ini` is gitignored since it contains a credential — don't commit it.

Create tasks with `--notify-method email` (or choose it interactively) to
have `check`/`run` email you when they're due. If `config.ini` is missing
or incomplete, `check` prints a warning naming the missing keys instead of
failing silently or crashing.

## Scheduling automatic checks

`taskbot check` only sends notifications when it's run — it needs to be
triggered periodically. Pick one:

### Option A: built-in daemon

```bash
taskbot run --daemon --interval 30   # checks every 30 minutes
```

Simplest option; just needs a terminal (or a background process /
terminal multiplexer like `tmux`/`screen`) left running.

### Option B: cron (macOS/Linux)

Run `crontab -e` and add a line to check every 30 minutes (adjust the path
to your `taskbot` executable, e.g. from `which taskbot` inside your venv):

```cron
*/30 * * * * /path/to/.venv/bin/taskbot check >> /tmp/taskbot.log 2>&1
```

### Option C: Task Scheduler (Windows)

1. Open Task Scheduler → **Create Basic Task**.
2. Trigger: **Daily**, then edit the trigger to **repeat every 30 minutes**.
3. Action: **Start a program**, program/script = path to `taskbot.exe`
   (inside your venv's `Scripts/` folder), arguments = `check`.

## Project layout

```
recurring-task-bot/
  taskbot/
    cli.py         # argparse commands (add/list/done/edit/delete/snooze/check/run)
    db.py          # SQLite schema + queries
    recurrence.py  # recurrence parsing and next-due-date math
    notify.py      # desktop (plyer) and email (smtplib) notification backends
    config.py      # data directory + email config loading
  config.example.ini
  pyproject.toml
  requirements.txt
```

## Database schema

```sql
CREATE TABLE tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    recurrence_type TEXT NOT NULL CHECK(recurrence_type IN ('daily', 'weekly', 'monthly', 'custom')),
    recurrence_value TEXT,           -- weekday / day-of-month / interval-in-days, per type
    next_due_date TEXT NOT NULL,     -- ISO date, YYYY-MM-DD
    last_completed_date TEXT,
    notification_method TEXT NOT NULL DEFAULT 'desktop' CHECK(notification_method IN ('desktop', 'email', 'none')),
    last_notified_date TEXT,         -- prevents duplicate notifications on the same day
    created_at TEXT NOT NULL
);
```
