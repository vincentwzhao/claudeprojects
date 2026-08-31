from app.tools.code_tools import search_code


def test_search_code_finds_login_implementation(sample_repo_path):
    result = search_code(sample_repo_path, "async function login")
    assert result["count"] >= 1
    files = {m["file"] for m in result["matches"]}
    assert "src/services/authService.js" in files


def test_search_code_is_case_insensitive(sample_repo_path):
    result = search_code(sample_repo_path, "JSONWEBTOKEN")
    assert result["count"] >= 1


def test_search_code_restricts_by_glob(sample_repo_path):
    result = search_code(sample_repo_path, "router", file_glob="*routes*.js")
    assert result["count"] >= 1
    assert all("routes/" in m["file"] for m in result["matches"])


def test_search_code_regex(sample_repo_path):
    result = search_code(sample_repo_path, r"jwt\.(sign|verify)", regex=True)
    assert result["count"] >= 2


def test_search_code_returns_line_numbers(sample_repo_path):
    result = search_code(sample_repo_path, "jwt.sign")
    match = next(m for m in result["matches"] if m["file"] == "src/services/authService.js")
    assert match["line"] > 0


def test_search_code_empty_query_errors(sample_repo_path):
    result = search_code(sample_repo_path, "")
    assert "error" in result


def test_search_code_no_matches(sample_repo_path):
    result = search_code(sample_repo_path, "definitely_not_in_this_repo_xyz")
    assert result["count"] == 0
