"""get_git_history: commit log, optionally scoped to a file/path."""

from __future__ import annotations

import subprocess
from pathlib import Path

from app.paths import safe_join

_SEP = "\x1f"  # unlikely-to-collide field separator


def get_git_history(root: Path, path: str | None = None, max_commits: int = 20) -> dict:
    if not (root / ".git").exists():
        return {"error": "Not a git repository (no .git directory found)"}

    cmd = [
        "git", "log",
        f"-n{max(1, min(max_commits, 100))}",
        f"--pretty=format:%h{_SEP}%an{_SEP}%ad{_SEP}%s",
        "--date=short",
    ]

    if path:
        try:
            target = safe_join(root, path)
        except Exception as exc:
            return {"error": str(exc)}
        cmd += ["--", str(target.relative_to(root.resolve()))]

    try:
        proc = subprocess.run(cmd, cwd=root, capture_output=True, text=True, timeout=15)
    except subprocess.TimeoutExpired:
        return {"error": "git log timed out"}

    if proc.returncode != 0:
        return {"error": f"git log failed: {proc.stderr.strip()[:300]}"}

    commits = []
    for line in proc.stdout.splitlines():
        parts = line.split(_SEP)
        if len(parts) != 4:
            continue
        commit_hash, author, date, subject = parts
        commits.append({"hash": commit_hash, "author": author, "date": date, "message": subject})

    return {"path": path, "count": len(commits), "commits": commits}
