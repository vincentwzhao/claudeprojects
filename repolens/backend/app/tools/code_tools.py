"""search_code: grep-like content search across the repository.

Uses ripgrep when available (fast, respects binary detection) and falls
back to a pure-Python scan otherwise so the tool works in any environment.
"""

from __future__ import annotations

import fnmatch
import re
import shutil
import subprocess
from pathlib import Path

from app.config import settings
from app.paths import iter_source_files, to_relative

_RIPGREP = shutil.which("rg")


def search_code(root: Path, query: str, regex: bool = False, file_glob: str | None = None,
                 max_results: int = 30) -> dict:
    query = query.strip()
    if not query:
        return {"error": "query must not be empty"}
    max_results = min(max_results, settings.max_search_results)

    if _RIPGREP:
        return _search_with_ripgrep(root, query, regex, file_glob, max_results)
    return _search_with_python(root, query, regex, file_glob, max_results)


def _search_with_ripgrep(root: Path, query: str, regex: bool, file_glob: str | None,
                          max_results: int) -> dict:
    # Note: we deliberately do NOT use ripgrep's own --glob here. Its
    # gitignore-style globs match slash-less patterns against the basename
    # only, which would silently disagree with the plain fnmatch-on-full-
    # relative-path semantics used by the pure-Python fallback below. To
    # keep both backends behave identically we filter matches ourselves.
    cmd = ["rg", "--line-number", "--no-heading", "--color", "never", "-m", "5"]
    if not regex:
        cmd.append("--fixed-strings")
    cmd.append("-i")
    cmd += [query, "."]

    try:
        proc = subprocess.run(cmd, cwd=root, capture_output=True, text=True, timeout=20)
    except subprocess.TimeoutExpired:
        return {"error": "search timed out"}

    if proc.returncode not in (0, 1):
        return {"error": f"ripgrep failed: {proc.stderr.strip()[:300]}"}

    matches = []
    for line in proc.stdout.splitlines():
        parts = line.split(":", 2)
        if len(parts) != 3:
            continue
        file_path, line_no, snippet = parts
        file_path = file_path.lstrip("./")
        if file_glob and not fnmatch.fnmatch(file_path, file_glob):
            continue
        matches.append({
            "file": file_path,
            "line": int(line_no),
            "snippet": snippet.strip()[:300],
        })
        if len(matches) >= max_results:
            break

    return {"query": query, "count": len(matches), "matches": matches}


def _search_with_python(root: Path, query: str, regex: bool, file_glob: str | None,
                         max_results: int) -> dict:
    pattern = re.compile(query, re.IGNORECASE) if regex else None
    needle = query.lower()
    matches = []

    for full_path in iter_source_files(root, limit=settings.max_repo_index_files):
        rel = to_relative(root, full_path)
        if file_glob and not fnmatch.fnmatch(rel, file_glob):
            continue
        try:
            text = full_path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for i, line in enumerate(text.splitlines(), start=1):
            hit = bool(pattern.search(line)) if pattern else (needle in line.lower())
            if hit:
                matches.append({"file": rel, "line": i, "snippet": line.strip()[:300]})
                if len(matches) >= max_results:
                    return {"query": query, "count": len(matches), "matches": matches}

    return {"query": query, "count": len(matches), "matches": matches}
