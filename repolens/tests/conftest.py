import shutil
import subprocess
from pathlib import Path

import pytest

FIXTURES_DIR = Path(__file__).parent / "fixtures"
SAMPLE_REPO = FIXTURES_DIR / "sample_repo"


@pytest.fixture
def sample_repo_path() -> Path:
    """Read-only fixture repo on disk (no .git directory)."""
    return SAMPLE_REPO


@pytest.fixture
def git_sample_repo(tmp_path) -> Path:
    """A throwaway copy of the fixture repo, git-initialized with a couple
    of commits, for tests that exercise get_git_history.
    """
    dest = tmp_path / "sample_repo"
    shutil.copytree(SAMPLE_REPO, dest)

    def run(*args):
        subprocess.run(["git", *args], cwd=dest, check=True, capture_output=True)

    run("init", "-q")
    run("config", "user.email", "test@example.com")
    run("config", "user.name", "Test User")
    run("add", ".")
    run("commit", "-q", "-m", "Initial commit: scaffold auth API")

    auth_service = dest / "src" / "services" / "authService.js"
    auth_service.write_text(auth_service.read_text() + "\n// tweak\n")
    run("add", ".")
    run("commit", "-q", "-m", "Tweak authService")

    return dest
