"""Filesystem inspection tools: search_files, read_file, list_directory."""

from __future__ import annotations

import fnmatch
from pathlib import Path

from app.config import settings
from app.paths import EXCLUDED_DIRS, iter_source_files, safe_join, to_relative


def search_files(root: Path, pattern: str, max_results: int = 50) -> dict:
    """Find files whose relative path matches a glob-like pattern or plain
    substring (case-insensitive). Directories like node_modules/.git are
    skipped.
    """
    pattern = pattern.strip()
    if not pattern:
        return {"error": "pattern must not be empty"}

    has_glob = any(ch in pattern for ch in "*?[]")
    needle = pattern.lower()
    matches: list[str] = []

    for full_path in iter_source_files(root, limit=settings.max_repo_index_files):
        rel = to_relative(root, full_path)
        if has_glob:
            if fnmatch.fnmatch(rel, pattern) or fnmatch.fnmatch(full_path.name, pattern):
                matches.append(rel)
        else:
            if needle in rel.lower():
                matches.append(rel)
        if len(matches) >= max_results:
            break

    return {"pattern": pattern, "count": len(matches), "files": matches}


def read_file(root: Path, path: str, start_line: int | None = None, end_line: int | None = None) -> dict:
    """Read a file's contents (optionally a line range), with line numbers.
    Truncates very large files to keep tool results small and cheap.
    """
    try:
        target = safe_join(root, path)
    except Exception as exc:  # PathSecurityError
        return {"error": str(exc)}

    if not target.exists():
        return {"error": f"File not found: {path}"}
    if target.is_dir():
        return {"error": f"'{path}' is a directory, not a file"}

    try:
        raw = target.read_bytes()
    except OSError as exc:
        return {"error": f"Could not read file: {exc}"}

    if b"\x00" in raw[:4096]:
        return {"error": f"'{path}' appears to be a binary file"}

    truncated = False
    if len(raw) > settings.max_file_read_bytes:
        raw = raw[: settings.max_file_read_bytes]
        truncated = True

    text = raw.decode("utf-8", errors="replace")
    lines = text.splitlines()

    if start_line or end_line:
        start = max((start_line or 1) - 1, 0)
        end = end_line if end_line else len(lines)
        lines = lines[start:end]
        offset = start
    else:
        offset = 0

    numbered = "\n".join(f"{i + offset + 1:>5} | {line}" for i, line in enumerate(lines))

    return {
        "path": path,
        "content": numbered,
        "total_lines": len(text.splitlines()),
        "truncated": truncated,
    }


def list_directory(root: Path, path: str = ".") -> dict:
    """List immediate contents of a directory relative to the repo root."""
    try:
        target = safe_join(root, path)
    except Exception as exc:
        return {"error": str(exc)}

    if not target.exists() or not target.is_dir():
        return {"error": f"'{path}' is not a directory"}

    entries = []
    try:
        for child in sorted(target.iterdir(), key=lambda p: (p.is_file(), p.name.lower())):
            if child.name in EXCLUDED_DIRS:
                continue
            entries.append({
                "name": child.name,
                "type": "dir" if child.is_dir() else "file",
            })
    except OSError as exc:
        return {"error": str(exc)}

    return {"path": path, "entries": entries}
