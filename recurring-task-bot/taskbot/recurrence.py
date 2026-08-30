"""Recurrence-rule parsing and next-due-date calculation.

A recurrence rule is stored as two columns:
  recurrence_type: "daily" | "weekly" | "monthly" | "custom"
  recurrence_value:
    daily   -> unused (None)
    weekly  -> weekday, "0".."6" (0=Monday .. 6=Sunday, matches date.weekday())
    monthly -> day of month, "1".."31"
    custom  -> N, the number of days between occurrences ("3" = every 3 days)
"""

from __future__ import annotations

import calendar
from datetime import date, timedelta

RECURRENCE_TYPES = ("daily", "weekly", "monthly", "custom")
WEEKDAY_NAMES = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]


class RecurrenceError(Exception):
    """Raised for an invalid recurrence type/value combination."""


def validate_recurrence(recurrence_type: str, recurrence_value: str | None) -> str | None:
    """Validate a recurrence type/value pair and return a normalized value.

    Raises RecurrenceError with a human-readable message on invalid input.
    """
    if recurrence_type not in RECURRENCE_TYPES:
        raise RecurrenceError(
            f"Unknown recurrence type {recurrence_type!r}. Must be one of: {', '.join(RECURRENCE_TYPES)}"
        )

    if recurrence_type == "daily":
        return None

    if recurrence_type == "weekly":
        if recurrence_value is None:
            raise RecurrenceError("Weekly recurrence needs a weekday (0=Monday .. 6=Sunday, or a name like 'tue').")
        weekday = _parse_weekday(recurrence_value)
        return str(weekday)

    if recurrence_type == "monthly":
        if recurrence_value is None:
            raise RecurrenceError("Monthly recurrence needs a day of month (1-31).")
        try:
            day = int(recurrence_value)
        except ValueError as exc:
            raise RecurrenceError(f"Day of month must be an integer, got {recurrence_value!r}") from exc
        if not 1 <= day <= 31:
            raise RecurrenceError(f"Day of month must be between 1 and 31, got {day}")
        return str(day)

    if recurrence_type == "custom":
        if recurrence_value is None:
            raise RecurrenceError("Custom recurrence needs an interval in days, e.g. '3' for every 3 days.")
        try:
            n = int(recurrence_value)
        except ValueError as exc:
            raise RecurrenceError(f"Custom interval must be an integer number of days, got {recurrence_value!r}") from exc
        if n < 1:
            raise RecurrenceError(f"Custom interval must be at least 1 day, got {n}")
        return str(n)

    raise RecurrenceError(f"Unknown recurrence type {recurrence_type!r}")


def _parse_weekday(value: str) -> int:
    value = value.strip().lower()
    try:
        weekday = int(value)
        if not 0 <= weekday <= 6:
            raise RecurrenceError(f"Weekday must be 0-6 (0=Monday..6=Sunday), got {weekday}")
        return weekday
    except ValueError:
        pass
    for i, name in enumerate(WEEKDAY_NAMES):
        if name.lower().startswith(value) and value:
            return i
    raise RecurrenceError(
        f"Could not parse weekday {value!r}. Use 0-6 (0=Monday) or a name/abbreviation like 'monday'/'mon'."
    )


def describe_recurrence(recurrence_type: str, recurrence_value: str | None) -> str:
    if recurrence_type == "daily":
        return "daily"
    if recurrence_type == "weekly":
        return f"weekly on {WEEKDAY_NAMES[int(recurrence_value)]}"
    if recurrence_type == "monthly":
        return f"monthly on day {recurrence_value}"
    if recurrence_type == "custom":
        return f"every {recurrence_value} day(s)"
    return recurrence_type


def compute_next_due(recurrence_type: str, recurrence_value: str | None, from_date: date) -> date:
    """Compute the next due date strictly after from_date.

    from_date is normally "today" (or the completion date), so completing
    a task early or late doesn't cause drift for daily/custom/weekly
    schedules -- the next occurrence is always relative to when it was
    actually completed.
    """
    if recurrence_type == "daily":
        return from_date + timedelta(days=1)

    if recurrence_type == "custom":
        n = int(recurrence_value)
        return from_date + timedelta(days=n)

    if recurrence_type == "weekly":
        target_weekday = int(recurrence_value)
        days_ahead = (target_weekday - from_date.weekday()) % 7
        if days_ahead == 0:
            days_ahead = 7
        return from_date + timedelta(days=days_ahead)

    if recurrence_type == "monthly":
        day = int(recurrence_value)
        month = from_date.month + 1
        year = from_date.year
        if month > 12:
            month = 1
            year += 1
        last_day_of_month = calendar.monthrange(year, month)[1]
        actual_day = min(day, last_day_of_month)
        return date(year, month, actual_day)

    raise RecurrenceError(f"Unknown recurrence type {recurrence_type!r}")
