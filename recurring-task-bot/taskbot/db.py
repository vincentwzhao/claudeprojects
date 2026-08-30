"""SQLite storage layer for taskbot."""

from __future__ import annotations

import sqlite3
from contextlib import contextmanager
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Iterator, Optional

from .config import get_db_path

NOTIFICATION_METHODS = ("desktop", "email", "none")

SCHEMA = """
CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    recurrence_type TEXT NOT NULL CHECK(recurrence_type IN ('daily', 'weekly', 'monthly', 'custom')),
    recurrence_value TEXT,
    next_due_date TEXT NOT NULL,
    last_completed_date TEXT,
    notification_method TEXT NOT NULL DEFAULT 'desktop' CHECK(notification_method IN ('desktop', 'email', 'none')),
    last_notified_date TEXT,
    created_at TEXT NOT NULL
);
"""


@dataclass
class Task:
    id: int
    name: str
    description: str
    recurrence_type: str
    recurrence_value: Optional[str]
    next_due_date: str
    last_completed_date: Optional[str]
    notification_method: str
    last_notified_date: Optional[str]
    created_at: str

    @property
    def next_due(self) -> date:
        return date.fromisoformat(self.next_due_date)

    @classmethod
    def from_row(cls, row: sqlite3.Row) -> "Task":
        return cls(**{key: row[key] for key in row.keys()})


def _connect(db_path: Optional[Path] = None) -> sqlite3.Connection:
    conn = sqlite3.connect(db_path or get_db_path())
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    return conn


def init_db(db_path: Optional[Path] = None) -> None:
    with _connect(db_path) as conn:
        conn.executescript(SCHEMA)


@contextmanager
def get_conn(db_path: Optional[Path] = None) -> Iterator[sqlite3.Connection]:
    init_db(db_path)
    conn = _connect(db_path)
    try:
        yield conn
        conn.commit()
    finally:
        conn.close()


def add_task(
    conn: sqlite3.Connection,
    name: str,
    description: str,
    recurrence_type: str,
    recurrence_value: Optional[str],
    next_due_date: date,
    notification_method: str,
) -> Task:
    cur = conn.execute(
        """
        INSERT INTO tasks
            (name, description, recurrence_type, recurrence_value,
             next_due_date, notification_method, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        """,
        (
            name,
            description,
            recurrence_type,
            recurrence_value,
            next_due_date.isoformat(),
            notification_method,
            date.today().isoformat(),
        ),
    )
    return get_task(conn, cur.lastrowid)


def get_task(conn: sqlite3.Connection, task_id: int) -> Optional[Task]:
    row = conn.execute("SELECT * FROM tasks WHERE id = ?", (task_id,)).fetchone()
    return Task.from_row(row) if row else None


def list_tasks(conn: sqlite3.Connection) -> list[Task]:
    rows = conn.execute("SELECT * FROM tasks ORDER BY next_due_date ASC, id ASC").fetchall()
    return [Task.from_row(row) for row in rows]


def list_due_tasks(conn: sqlite3.Connection, as_of: Optional[date] = None) -> list[Task]:
    as_of = as_of or date.today()
    rows = conn.execute(
        "SELECT * FROM tasks WHERE next_due_date <= ? ORDER BY next_due_date ASC, id ASC",
        (as_of.isoformat(),),
    ).fetchall()
    return [Task.from_row(row) for row in rows]


def update_task(conn: sqlite3.Connection, task_id: int, **fields) -> Optional[Task]:
    if not fields:
        return get_task(conn, task_id)
    columns = ", ".join(f"{key} = ?" for key in fields)
    values = [
        value.isoformat() if isinstance(value, date) else value
        for value in fields.values()
    ]
    conn.execute(f"UPDATE tasks SET {columns} WHERE id = ?", (*values, task_id))
    return get_task(conn, task_id)


def delete_task(conn: sqlite3.Connection, task_id: int) -> bool:
    cur = conn.execute("DELETE FROM tasks WHERE id = ?", (task_id,))
    return cur.rowcount > 0
