"""Paths and configuration handling for taskbot.

All persistent state (the SQLite database and the optional email config)
lives in a single "home" directory so the CLI behaves the same no matter
which directory it's invoked from. The location can be overridden with the
TASKBOT_HOME environment variable, which is handy for tests.
"""

from __future__ import annotations

import configparser
import os
from dataclasses import dataclass
from pathlib import Path


def get_home_dir() -> Path:
    home = os.environ.get("TASKBOT_HOME")
    path = Path(home).expanduser() if home else Path.home() / ".taskbot"
    path.mkdir(parents=True, exist_ok=True)
    return path


def get_db_path() -> Path:
    return get_home_dir() / "tasks.db"


def get_config_path() -> Path:
    return get_home_dir() / "config.ini"


@dataclass
class EmailConfig:
    smtp_host: str
    smtp_port: int
    username: str
    password: str
    from_addr: str
    to_addr: str
    use_tls: bool = True


class ConfigError(Exception):
    """Raised when required configuration is missing or invalid."""


def load_email_config() -> EmailConfig:
    """Load email settings from config.ini's [email] section.

    Raises ConfigError with a helpful message if the file or a required
    key is missing, so callers can surface a clear error instead of a
    raw traceback.
    """
    path = get_config_path()
    if not path.exists():
        raise ConfigError(
            f"No config file found at {path}.\n"
            "Create one (see README.md 'Configuring email') before using "
            "email notifications."
        )

    parser = configparser.ConfigParser()
    parser.read(path)

    if "email" not in parser:
        raise ConfigError(
            f"{path} has no [email] section. See README.md 'Configuring email'."
        )

    section = parser["email"]
    required = ["smtp_host", "smtp_port", "username", "password", "from_addr", "to_addr"]
    missing = [key for key in required if not section.get(key)]
    if missing:
        raise ConfigError(
            f"{path} is missing required [email] keys: {', '.join(missing)}. "
            "See README.md 'Configuring email'."
        )

    try:
        port = int(section["smtp_port"])
    except ValueError as exc:
        raise ConfigError(f"[email] smtp_port must be an integer, got {section['smtp_port']!r}") from exc

    return EmailConfig(
        smtp_host=section["smtp_host"],
        smtp_port=port,
        username=section["username"],
        password=section["password"],
        from_addr=section["from_addr"],
        to_addr=section["to_addr"],
        use_tls=section.getboolean("use_tls", fallback=True),
    )
