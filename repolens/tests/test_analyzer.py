from app.analysis.analyzer import analyze_repository


def test_analyze_repository_languages(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    assert "JavaScript" in analysis.languages


def test_analyze_repository_frameworks(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    assert "Express (Node.js)" in analysis.frameworks


def test_analyze_repository_entry_points(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    entry_paths = [e["path"] for e in analysis.entry_points]
    assert "src/index.js" in entry_paths


def test_analyze_repository_database_layer(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    labels = [d["type"] for d in analysis.database_layer]
    assert any("PostgreSQL" in label for label in labels)


def test_analyze_repository_authentication(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    labels = [a["type"] for a in analysis.authentication]
    assert any("JWT" in label for label in labels)
    assert any("bcrypt" in label for label in labels)


def test_analyze_repository_api_routes(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    route_paths = {(r["method"], r["path"]) for r in analysis.api_routes}
    assert ("POST", "/register") in route_paths
    assert ("POST", "/login") in route_paths


def test_analyze_repository_important_directories(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    dir_paths = {d["path"] for d in analysis.important_directories}
    assert "src/routes" in dir_paths or "routes" in dir_paths
    assert "src/services" in dir_paths or "services" in dir_paths

def test_analyze_repository_tests_detected(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    assert analysis.tests["directories"]
    assert analysis.tests["framework"] == "Jest"


def test_analyze_repository_build_system(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    tools = {b["tool"] for b in analysis.build_system}
    assert "npm/yarn/pnpm" in tools


def test_analyze_repository_dependencies(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    npm_deps = {d["name"] for d in analysis.dependencies.get("npm", [])}
    assert "express" in npm_deps
    assert "jsonwebtoken" in npm_deps


def test_analyze_repository_configuration(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    assert ".env.example" in analysis.configuration


def test_analyze_repository_potential_issues_flag_missing_ci(sample_repo_path):
    analysis = analyze_repository(sample_repo_path)
    assert any("CI" in issue for issue in analysis.potential_issues)


def test_analyze_repository_to_dict_serializable(sample_repo_path):
    import json
    analysis = analyze_repository(sample_repo_path)
    json.dumps(analysis.to_dict())  # should not raise
