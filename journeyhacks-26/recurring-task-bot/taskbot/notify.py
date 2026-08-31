"""Notification backends: desktop popups and email."""

from __future__ import annotations

import platform
import shutil
import smtplib
import subprocess
from email.mime.text import MIMEText

from .config import ConfigError, load_email_config


class NotificationError(Exception):
    """Raised when a notification could not be delivered."""


def send_desktop_notification(title: str, message: str) -> None:
    """Show a desktop notification, preferring the cross-platform `plyer`
    library and falling back to a native OS call if it's unavailable or
    fails (e.g. no notification daemon running)."""
    try:
        from plyer import notification as plyer_notification

        plyer_notification.notify(title=title, message=message, app_name="taskbot", timeout=10)
        return
    except Exception:
        pass

    system = platform.system()
    try:
        if system == "Linux" and shutil.which("notify-send"):
            subprocess.run(["notify-send", title, message], check=True)
            return
        if system == "Darwin":
            script = f'display notification "{_escape_applescript(message)}" with title "{_escape_applescript(title)}"'
            subprocess.run(["osascript", "-e", script], check=True)
            return
        if system == "Windows":
            ps_script = (
                "Add-Type -AssemblyName System.Windows.Forms; "
                "$n = New-Object System.Windows.Forms.NotifyIcon; "
                "$n.Icon = [System.Drawing.SystemIcons]::Information; "
                "$n.Visible = $true; "
                f'$n.ShowBalloonTip(10000, "{title}", "{message}", '
                "[System.Windows.Forms.ToolTipIcon]::Info)"
            )
            subprocess.run(["powershell", "-Command", ps_script], check=True)
            return
    except Exception as exc:
        raise NotificationError(f"Could not show a desktop notification: {exc}") from exc

    raise NotificationError(
        "No working desktop notification backend found for this system "
        "(plyer failed and no native fallback is available)."
    )


def _escape_applescript(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def send_email_notification(subject: str, body: str) -> None:
    """Send an email notification using SMTP settings from config.ini.

    Raises ConfigError if email isn't configured, or NotificationError if
    sending fails.
    """
    cfg = load_email_config()

    msg = MIMEText(body)
    msg["Subject"] = subject
    msg["From"] = cfg.from_addr
    msg["To"] = cfg.to_addr

    try:
        with smtplib.SMTP(cfg.smtp_host, cfg.smtp_port, timeout=15) as server:
            if cfg.use_tls:
                server.starttls()
            server.login(cfg.username, cfg.password)
            server.sendmail(cfg.from_addr, [cfg.to_addr], msg.as_string())
    except ConfigError:
        raise
    except Exception as exc:
        raise NotificationError(f"Could not send email notification: {exc}") from exc


def notify(method: str, title: str, message: str) -> None:
    if method == "none":
        return
    if method == "desktop":
        send_desktop_notification(title, message)
    elif method == "email":
        send_email_notification(title, message)
    else:
        raise NotificationError(f"Unknown notification method {method!r}")
