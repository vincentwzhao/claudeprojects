from app.paths import PathSecurityError, safe_join
from app.tools.file_tools import list_directory, read_file, search_files


def test_search_files_substring_match(sample_repo_path):
    result = search_files(sample_repo_path, "auth")
    assert result["count"] > 0
    assert any("routes/auth.js" in f for f in result["files"])
    assert any("authService.js" in f for f in result["files"])


def test_search_files_glob_match(sample_repo_path):
    result = search_files(sample_repo_path, "*.test.js")
    assert result["files"] == ["tests/auth.test.js"]


def test_search_files_excludes_node_modules(sample_repo_path):
    # node_modules doesn't exist in the fixture, but this proves the
    # exclusion list is applied without erroring on repos that do have it.
    result = search_files(sample_repo_path, "*")
    assert all("node_modules" not in f for f in result["files"])


def test_search_files_empty_pattern_errors(sample_repo_path):
    result = search_files(sample_repo_path, "   ")
    assert "error" in result


def test_read_file_returns_numbered_content(sample_repo_path):
    result = read_file(sample_repo_path, "src/services/authService.js")
    assert result["path"] == "src/services/authService.js"
    assert "async function login" in result["content"]
    assert result["content"].splitlines()[0].strip().startswith("1 |")


def test_read_file_line_range(sample_repo_path):
    full = read_file(sample_repo_path, "src/services/authService.js")
    ranged = read_file(sample_repo_path, "src/services/authService.js", start_line=1, end_line=3)
    assert len(ranged["content"].splitlines()) == 3
    assert ranged["content"] != full["content"]


def test_read_file_missing_file(sample_repo_path):
    result = read_file(sample_repo_path, "src/does/not/exist.js")
    assert "error" in result


def test_read_file_blocks_path_traversal(sample_repo_path):
    result = read_file(sample_repo_path, "../../../../etc/passwd")
    assert "error" in result


def test_safe_join_blocks_escape(sample_repo_path):
    import pytest
    with pytest.raises(PathSecurityError):
        safe_join(sample_repo_path, "../outside.txt")


def test_list_directory_top_level(sample_repo_path):
    result = list_directory(sample_repo_path, ".")
    names = {e["name"] for e in result["entries"]}
    assert "src" in names
    assert "package.json" in names


def test_list_directory_nested(sample_repo_path):
    result = list_directory(sample_repo_path, "src/services")
    names = {e["name"] for e in result["entries"]}
    assert "authService.js" in names
    assert "userService.js" in names
