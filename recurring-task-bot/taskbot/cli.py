"""Command-line interface for taskbot."""

from __future__ import annotations

import argparse
import sys
import time
from datetime import date, datetime, timedelta
from typing import Optional

from . import db
from .config import ConfigError
from .notify import NotificationError, notify
from .recurrence import (
    RECURRENCE_TYPES,
    RecurrenceError,
    compute_next_due,
    describe_recurrence,
    validate_recurrence,
)

NOTIFICATION_METHODS = ("desktop", "email", "none")


def eprint(*args, **kwargs) -> None:
    print(*args, file=sys.stderr, **kwargs)


def parse_date(value: str) -> date:
    try:
        return date.fromisoformat(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            f"Invalid date {value!r}, expected YYYY-MM-DD"
        ) from exc


# ---------------------------------------------------------------------------
# Interactive prompts (used by `add` / `edit` when fields aren't passed as flags)
# ---------------------------------------------------------------------------

def prompt_nonempty(label: str, default: Optional[str] = None) -> str:
    suffix = f" [{default}]" if default is not None else ""
    while True:
        value = input(f"{label}{suffix}: ").strip()
        if not value and default is not None:
            return default
        if value:
            return value
        print("This field is required.")


def prompt_recurrence(default_type: Optional[str] = None, default_value: Optional[str] = None) -> tuple[str, Optional[str]]:
    while True:
        type_default = f" [{default_type}]" if default_type else ""
        recurrence_type = input(
            f"Recurrence ({'/'.join(RECURRENCE_TYPES)}){type_default}: "
        ).strip().lower()
        if not recurrence_type and default_type:
            recurrence_type = default_type
        if recurrence_type not in RECURRENCE_TYPES:
            print(f"Please enter one of: {', '.join(RECURRENCE_TYPES)}")
            continue

        value_prompt = {
            "daily": None,
            "weekly": "Weekday (0=Monday .. 6=Sunday, or name like 'tue')",
            "monthly": "Day of month (1-31)",
            "custom": "Repeat every how many days?",
        }[recurrence_type]

        recurrence_value = None
        if value_prompt:
            value_default = f" [{default_value}]" if default_value else ""
            recurrence_value = input(f"{value_prompt}{value_default}: ").strip()
            if not recurrence_value and default_value:
                recurrence_value = default_value

        try:
            normalized = validate_recurrence(recurrence_type, recurrence_value)
            return recurrence_type, normalized
        except RecurrenceError as exc:
            print(f"  {exc}")


def prompt_notification_method(default: Optional[str] = None) -> str:
    suffix = f" [{default}]" if default else ""
    while True:
        value = input(f"Notification method ({'/'.join(NOTIFICATION_METHODS)}){suffix}: ").strip().lower()
        if not value and default:
            return default
        if value in NOTIFICATION_METHODS:
            return value
        print(f"Please enter one of: {', '.join(NOTIFICATION_METHODS)}")


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_add(args: argparse.Namespace) -> int:
    # Non-interactive when name is passed as a flag; otherwise prompt for
    # every field so `taskbot add` with no arguments walks the user through it.
    non_interactive = bool(args.name)

    if args.name:
        name = args.name
    else:
        name = prompt_nonempty("Task name")

    if args.description is not None:
        description = args.description
    elif non_interactive:
        description = ""
    else:
        description = input("Description (optional): ").strip()

    if args.recurrence:
        recurrence_type = args.recurrence
        try:
            recurrence_value = validate_recurrence(recurrence_type, args.recurrence_value)
        except RecurrenceError as exc:
            eprint(f"Error: {exc}")
            return 1
    else:
        recurrence_type, recurrence_value = prompt_recurrence()

    if args.notify_method:
        notification_method = args.notify_method
    else:
        notification_method = prompt_notification_method(default="desktop")

    # First occurrence: if the caller gave --start-date, use it verbatim;
    # otherwise the first due date is simply today.
    next_due = args.start_date if args.start_date else date.today()

    with db.get_conn() as conn:
        task = db.add_task(
            conn,
            name=name,
            description=description,
            recurrence_type=recurrence_type,
            recurrence_value=recurrence_value,
            next_due_date=next_due,
            notification_method=notification_method,
        )

    print(f"Added task #{task.id}: {task.name} ({describe_recurrence(recurrence_type, recurrence_value)})")
    print(f"  Next due: {task.next_due_date}")
    return 0


def _format_task_row(task: db.Task, today: date) -> str:
    due = task.next_due
    if due < today:
        status = f"OVERDUE ({(today - due).days}d)"
    elif due == today:
        status = "DUE TODAY"
    else:
        status = f"in {(due - today).days}d"
    recurrence = describe_recurrence(task.recurrence_type, task.recurrence_value)
    return (
        f"#{task.id:<4} {task.name:<30} due {task.next_due_date}  [{status}]  "
        f"{recurrence}  notify={task.notification_method}"
    )


def cmd_list(args: argparse.Namespace) -> int:
    with db.get_conn() as conn:
        tasks = db.list_tasks(conn)

    if not tasks:
        print("No tasks yet. Add one with `taskbot add`.")
        return 0

    today = date.today()
    for task in tasks:
        print(_format_task_row(task, today))
    return 0


def cmd_done(args: argparse.Namespace) -> int:
    completion_date = args.date or date.today()
    with db.get_conn() as conn:
        task = db.get_task(conn, args.id)
        if not task:
            eprint(f"Error: no task with id {args.id}")
            return 1

        next_due = compute_next_due(task.recurrence_type, task.recurrence_value, completion_date)
        task = db.update_task(
            conn,
            task.id,
            last_completed_date=completion_date.isoformat(),
            next_due_date=next_due.isoformat(),
            last_notified_date=None,
        )

    print(f"Marked #{task.id} '{task.name}' complete on {completion_date.isoformat()}.")
    print(f"  Next due: {task.next_due_date}")
    return 0


def cmd_edit(args: argparse.Namespace) -> int:
    with db.get_conn() as conn:
        task = db.get_task(conn, args.id)
        if not task:
            eprint(f"Error: no task with id {args.id}")
            return 1

        fields: dict = {}
        interactive = not any(
            [args.name, args.description is not None, args.recurrence, args.notify_method, args.next_due_date]
        )

        if interactive:
            print(f"Editing #{task.id} (press Enter to keep the current value)")
            new_name = prompt_nonempty("Task name", default=task.name)
            if new_name != task.name:
                fields["name"] = new_name

            new_description = input(f"Description [{task.description}]: ").strip()
            if new_description and new_description != task.description:
                fields["description"] = new_description

            new_type, new_value = prompt_recurrence(default_type=task.recurrence_type, default_value=task.recurrence_value)
            if new_type != task.recurrence_type or new_value != task.recurrence_value:
                fields["recurrence_type"] = new_type
                fields["recurrence_value"] = new_value

            new_method = prompt_notification_method(default=task.notification_method)
            if new_method != task.notification_method:
                fields["notification_method"] = new_method
        else:
            if args.name:
                fields["name"] = args.name
            if args.description is not None:
                fields["description"] = args.description
            if args.recurrence:
                try:
                    fields["recurrence_value"] = validate_recurrence(args.recurrence, args.recurrence_value)
                except RecurrenceError as exc:
                    eprint(f"Error: {exc}")
                    return 1
                fields["recurrence_type"] = args.recurrence
            if args.notify_method:
                fields["notification_method"] = args.notify_method
            if args.next_due_date:
                fields["next_due_date"] = args.next_due_date

        if not fields:
            print("Nothing to change.")
            return 0

        task = db.update_task(conn, task.id, **fields)

    print(f"Updated #{task.id}: {task.name}")
    return 0


def cmd_delete(args: argparse.Namespace) -> int:
    with db.get_conn() as conn:
        task = db.get_task(conn, args.id)
        if not task:
            eprint(f"Error: no task with id {args.id}")
            return 1
        if not args.yes:
            confirm = input(f"Delete #{task.id} '{task.name}'? [y/N]: ").strip().lower()
            if confirm != "y":
                print("Cancelled.")
                return 0
        db.delete_task(conn, task.id)

    print(f"Deleted #{task.id} '{task.name}'.")
    return 0


def cmd_snooze(args: argparse.Namespace) -> int:
    if args.days < 1:
        eprint("Error: snooze days must be at least 1")
        return 1

    with db.get_conn() as conn:
        task = db.get_task(conn, args.id)
        if not task:
            eprint(f"Error: no task with id {args.id}")
            return 1

        new_due = task.next_due + timedelta(days=args.days)
        task = db.update_task(conn, task.id, next_due_date=new_due.isoformat(), last_notified_date=None)

    print(f"Snoozed #{task.id} '{task.name}' to {task.next_due_date}.")
    return 0


def cmd_check(args: argparse.Namespace) -> int:
    today = date.today()
    with db.get_conn() as conn:
        due_tasks = db.list_due_tasks(conn, as_of=today)
        fired = 0
        for task in due_tasks:
            if task.last_notified_date == today.isoformat() and not args.force:
                continue
            if task.notification_method == "none":
                continue

            due = task.next_due
            when = "overdue" if due < today else "due today"
            title = f"Task reminder: {task.name}"
            message = f"{task.name} is {when} ({task.next_due_date})."
            if task.description:
                message += f"\n{task.description}"

            try:
                notify(task.notification_method, title, message)
                db.update_task(conn, task.id, last_notified_date=today.isoformat())
                fired += 1
                print(f"Notified for #{task.id} '{task.name}' via {task.notification_method}.")
            except (NotificationError, ConfigError) as exc:
                eprint(f"Warning: could not notify for #{task.id} '{task.name}': {exc}")

    if fired == 0:
        print("No new notifications to send.")
    return 0


def cmd_run(args: argparse.Namespace) -> int:
    if not args.daemon:
        return cmd_check(args)

    interval = args.interval
    if interval < 1:
        eprint("Error: --interval must be at least 1 minute")
        return 1

    print(f"Starting taskbot daemon: checking every {interval} minute(s). Press Ctrl+C to stop.")
    try:
        while True:
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            print(f"[{timestamp}] Running check...")
            cmd_check(args)
            time.sleep(interval * 60)
    except KeyboardInterrupt:
        print("\nStopped.")
    return 0


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="taskbot", description="A personal recurring task/reminder bot.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    p_add = subparsers.add_parser("add", help="Add a new recurring task")
    p_add.add_argument("--name", help="Task name")
    p_add.add_argument("--description", help="Task description")
    p_add.add_argument("--recurrence", choices=RECURRENCE_TYPES, help="Recurrence type")
    p_add.add_argument(
        "--recurrence-value",
        help="Recurrence detail: weekday (weekly), day-of-month (monthly), or interval in days (custom)",
    )
    p_add.add_argument("--notify-method", choices=NOTIFICATION_METHODS, help="Notification method")
    p_add.add_argument("--start-date", type=parse_date, help="First due date (default: today), YYYY-MM-DD")
    p_add.set_defaults(func=cmd_add)

    p_list = subparsers.add_parser("list", help="List all tasks, soonest due first")
    p_list.set_defaults(func=cmd_list)

    p_done = subparsers.add_parser("done", help="Mark a task complete and roll its due date forward")
    p_done.add_argument("id", type=int, help="Task id")
    p_done.add_argument("--date", type=parse_date, help="Completion date (default: today), YYYY-MM-DD")
    p_done.set_defaults(func=cmd_done)

    p_edit = subparsers.add_parser("edit", help="Edit a task's fields")
    p_edit.add_argument("id", type=int, help="Task id")
    p_edit.add_argument("--name", help="New task name")
    p_edit.add_argument("--description", help="New description")
    p_edit.add_argument("--recurrence", choices=RECURRENCE_TYPES, help="New recurrence type")
    p_edit.add_argument("--recurrence-value", help="New recurrence detail (see `add --help`)")
    p_edit.add_argument("--notify-method", choices=NOTIFICATION_METHODS, help="New notification method")
    p_edit.add_argument("--next-due-date", type=parse_date, help="Manually set the next due date, YYYY-MM-DD")
    p_edit.set_defaults(func=cmd_edit)

    p_delete = subparsers.add_parser("delete", help="Delete a task")
    p_delete.add_argument("id", type=int, help="Task id")
    p_delete.add_argument("-y", "--yes", action="store_true", help="Skip confirmation prompt")
    p_delete.set_defaults(func=cmd_delete)

    p_snooze = subparsers.add_parser("snooze", help="Push a task's due date back temporarily")
    p_snooze.add_argument("id", type=int, help="Task id")
    p_snooze.add_argument("days", type=int, help="Number of days to push the due date back")
    p_snooze.set_defaults(func=cmd_snooze)

    p_check = subparsers.add_parser("check", help="Scan for due/overdue tasks and fire notifications")
    p_check.add_argument("--force", action="store_true", help="Re-notify even if already notified today")
    p_check.set_defaults(func=cmd_check)

    p_run = subparsers.add_parser("run", help="Run checks now, or continuously with --daemon")
    p_run.add_argument("--daemon", action="store_true", help="Loop forever, checking every --interval minutes")
    p_run.add_argument("--interval", type=int, default=60, help="Minutes between checks in daemon mode (default: 60)")
    p_run.add_argument("--force", action="store_true", help="Re-notify even if already notified today")
    p_run.set_defaults(func=cmd_run)

    return parser


def main(argv: Optional[list[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except (RecurrenceError, ConfigError) as exc:
        eprint(f"Error: {exc}")
        return 1
    except KeyboardInterrupt:
        eprint("\nAborted.")
        return 130


if __name__ == "__main__":
    sys.exit(main())
