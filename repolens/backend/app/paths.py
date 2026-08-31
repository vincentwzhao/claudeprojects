"""Shared filesystem helpers: directory exclusion rules and safe path resolution.

Every tool that touches the filesystem on behalf of the LLM must resolve
paths through `safe_join` so a hallucinated or adversarial path (e.g.
"../../etc/passwd") can never escape the repository root.
"""

from __future__ import annotations

import os
from pathlib import Path

EXCLUDED_DIRS = {
    ".git",
    "node_modules",
    "__pycache__",
    ".venv",
    "venv",
    "env",
    "dist",
    "build",
    "target",
    "vendor",
    ".next",
    ".nuxt",
    "coverage",
    ".pytest_cache",
    ".mypy_cache",
    ".idea",
    ".vscode",
    "bin",
    "obj",
    ".gradle",
    ".tox",
    "site-packages",
    "egg-info",
}

BINARY_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".gif", ".ico", ".svg", ".webp", ".bmp",
    ".pdf", ".zip", ".tar", ".gz", ".7z", ".rar",
    ".woff", ".woff2", ".ttf", ".eot",
    ".mp3", ".mp4", ".mov", ".avi",
    ".so", ".dll", ".dylib", ".exe", ".class", ".jar",
    ".pyc", ".pyo",
    ".lock",
}


class PathSecurityError(ValueError):
    """Raised when a requested path would escape the repository root."""


def is_excluded_dir(name: str) -> bool:
    return name in EXCLUDED_DIRS or name.startswith(".") and name not in {".", ".."}


def iter_source_files(root: Path, limit: int | None = None):
    """Walk `root`, skipping excluded directories and binary files."""
    count = 0
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in EXCLUDED_DIRS and not d.startswith(".git")]
        for fname in filenames:
            if Path(fname).suffix.lower() in BINARY_EXTENSIONS:
                continue
            full = Path(dirpath) / fname
            yield full
            count += 1
            if limit is not None and count >= limit:
                return


def safe_join(root: Path, relative: str) -> Path:
    """Resolve `relative` against `root`, raising if it escapes the root."""
    root_resolved = root.resolve()
    candidate = (root_resolved / relative.lstrip("/\\")).resolve()
    try:
        candidate.relative_to(root_resolved)
    except ValueError:
        raise PathSecurityError(f"Path '{relative}' escapes the repository root")
    return candidate


def to_relative(root: Path, path: Path) -> str:
    try:
        return str(path.resolve().relative_to(root.resolve()))
    except ValueError:
        return str(path)
