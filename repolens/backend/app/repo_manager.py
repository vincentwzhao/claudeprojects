"""Loads repositories (git URL or local path) into a workspace and keeps an
in-memory registry of them for the lifetime of the process.

Kept intentionally simple (no database) per the MVP scope: a dict is plenty
for a single-process demo server. If the process restarts, repos must be
reloaded — documented as a known limitation.
"""

from __future__ import annotations

import re
import shutil
import subprocess
import uuid
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from app.analysis.analyzer import RepositoryAnalysis, analyze_repository
from app.config import settings

GIT_URL_RE = re.compile(r"^(https?://|git@)", re.IGNORECASE)


class RepoLoadError(Exception):
    pass


@dataclass
class Repo:
    id: str
    name: str
    source: str
    path: Path
    is_local: bool
    analysis: Optional[RepositoryAnalysis] = field(default=None)


class RepoManager:
    def __init__(self) -> None:
        self._repos: dict[str, Repo] = {}
        self._workspace = Path(settings.workspace_dir)
        self._workspace.mkdir(parents=True, exist_ok=True)

    def load(self, source: str) -> Repo:
        source = source.strip()
        if not source:
            raise RepoLoadError("Repository source must not be empty")

        if GIT_URL_RE.match(source):
            repo = self._clone(source)
        else:
            repo = self._load_local(source)

        repo.analysis = analyze_repository(repo.path)
        self._repos[repo.id] = repo
        return repo

    def _clone(self, url: str) -> Repo:
        repo_id = uuid.uuid4().hex[:12]
        name = url.rstrip("/").rsplit("/", 1)[-1].removesuffix(".git") or repo_id
        dest = self._workspace / repo_id
        try:
            subprocess.run(
                ["git", "clone", "--depth", "100", "--single-branch", url, str(dest)],
                check=True,
                capture_output=True,
                text=True,
                timeout=180,
            )
        except FileNotFoundError as exc:
            raise RepoLoadError("git is not installed on the server") from exc
        except subprocess.CalledProcessError as exc:
            raise RepoLoadError(f"git clone failed: {exc.stderr.strip()[:500]}") from exc
        except subprocess.TimeoutExpired as exc:
            raise RepoLoadError("git clone timed out (repository too large?)") from exc

        return Repo(id=repo_id, name=name, source=url, path=dest, is_local=False)

    def _load_local(self, path_str: str) -> Repo:
        path = Path(path_str).expanduser().resolve()
        if not path.exists() or not path.is_dir():
            raise RepoLoadError(f"Local path does not exist or is not a directory: {path}")
        repo_id = uuid.uuid4().hex[:12]
        return Repo(id=repo_id, name=path.name, source=str(path), path=path, is_local=True)

    def get(self, repo_id: str) -> Repo:
        repo = self._repos.get(repo_id)
        if repo is None:
            raise KeyError(repo_id)
        return repo

    def list(self) -> list[Repo]:
        return list(self._repos.values())

    def remove(self, repo_id: str) -> None:
        repo = self._repos.pop(repo_id, None)
        if repo and not repo.is_local and repo.path.exists():
            shutil.rmtree(repo.path, ignore_errors=True)


repo_manager = RepoManager()
