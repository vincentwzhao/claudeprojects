from app.tools.git_tools import get_git_history


def test_get_git_history_returns_commits(git_sample_repo):
    result = get_git_history(git_sample_repo)
    assert result["count"] == 2
    messages = [c["message"] for c in result["commits"]]
    assert "Tweak authService" in messages
    assert "Initial commit: scaffold auth API" in messages


def test_get_git_history_scoped_to_file(git_sample_repo):
    result = get_git_history(git_sample_repo, path="src/services/authService.js")
    assert result["count"] == 2

    result_unrelated = get_git_history(git_sample_repo, path="src/models/User.js")
    assert result_unrelated["count"] == 1


def test_get_git_history_respects_max_commits(git_sample_repo):
    result = get_git_history(git_sample_repo, max_commits=1)
    assert result["count"] == 1


def test_get_git_history_not_a_git_repo(sample_repo_path):
    result = get_git_history(sample_repo_path)
    assert "error" in result
