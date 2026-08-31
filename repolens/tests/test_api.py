"""End-to-end API tests: load the fixture repo through the real HTTP layer,
then exercise /chat, /explain, /trace, /project with the Anthropic client
mocked out (so these run offline, without ANTHROPIC_API_KEY).
"""

from types import SimpleNamespace

import pytest
from fastapi.testclient import TestClient

from app.main import app
from app.repo_manager import repo_manager


def text_response(text):
    block = SimpleNamespace(type="text", text=text)
    return SimpleNamespace(stop_reason="end_turn", content=[block])


class FakeMessages:
    def __init__(self, text):
        self._text = text

    def create(self, **kwargs):
        return text_response(self._text)


class FakeClient:
    def __init__(self, text):
        self.messages = FakeMessages(text)


@pytest.fixture(autouse=True)
def clear_registry():
    yield
    for repo in list(repo_manager.list()):
        repo_manager.remove(repo.id)


@pytest.fixture
def client():
    return TestClient(app)


@pytest.fixture
def loaded_repo_id(client, sample_repo_path):
    resp = client.post("/api/repos", json={"source": str(sample_repo_path)})
    assert resp.status_code == 200
    return resp.json()["id"]


def test_load_repo_returns_summary(client, sample_repo_path):
    resp = client.post("/api/repos", json={"source": str(sample_repo_path)})
    assert resp.status_code == 200
    body = resp.json()
    assert body["is_local"] is True
    assert "JavaScript" in body["languages"]
    assert body["steps"] == [
        "Repository indexed",
        "Architecture analyzed",
        "Entry points identified",
        "Dependencies analyzed",
    ]


def test_load_repo_missing_path_returns_400(client):
    resp = client.post("/api/repos", json={"source": "/no/such/path/at/all"})
    assert resp.status_code == 400


def test_get_repo_analysis(client, loaded_repo_id):
    resp = client.get(f"/api/repos/{loaded_repo_id}/analysis")
    assert resp.status_code == 200
    assert "Express (Node.js)" in resp.json()["frameworks"]


def test_unknown_repo_is_404(client):
    resp = client.get("/api/repos/does-not-exist")
    assert resp.status_code == 404


def test_explain_endpoint(client, loaded_repo_id, monkeypatch):
    monkeypatch.setattr(
        "app.agent.core._get_client",
        lambda: FakeClient(
            "Authentication starts in src/routes/auth.js:6.\n\nRelevant files:\n- src/routes/auth.js"
        ),
    )
    resp = client.post(f"/api/repos/{loaded_repo_id}/explain", json={"concept": "authentication"})
    assert resp.status_code == 200
    body = resp.json()
    assert "src/routes/auth.js" in body["answer"]
    assert {"file": "src/routes/auth.js", "line": 6} in body["references"]


def test_trace_endpoint(client, loaded_repo_id, monkeypatch):
    monkeypatch.setattr(
        "app.agent.core._get_client",
        lambda: FakeClient("Route -> Controller -> Service -> Repository -> DB\n\nRelevant files:\n- src/routes/auth.js"),
    )
    resp = client.post(f"/api/repos/{loaded_repo_id}/trace", json={"feature": "user registration"})
    assert resp.status_code == 200
    assert "Route -> Controller" in resp.json()["answer"]


def test_project_endpoint(client, loaded_repo_id, monkeypatch):
    monkeypatch.setattr(
        "app.agent.core._get_client",
        lambda: FakeClient("## Architecture\nExpress app.\n## Entry Points\nsrc/index.js"),
    )
    resp = client.post(f"/api/repos/{loaded_repo_id}/project")
    assert resp.status_code == 200
    assert "## Architecture" in resp.json()["answer"]


def test_chat_endpoint(client, loaded_repo_id, monkeypatch):
    monkeypatch.setattr(
        "app.agent.core._get_client",
        lambda: FakeClient("This repo does not implement payments. Relevant files: none found."),
    )
    resp = client.post(f"/api/repos/{loaded_repo_id}/chat", json={"message": "How do payments work?"})
    assert resp.status_code == 200
    assert "does not implement payments" in resp.json()["answer"]


def test_chat_rejects_empty_message(client, loaded_repo_id):
    resp = client.post(f"/api/repos/{loaded_repo_id}/chat", json={"message": ""})
    assert resp.status_code == 422


def test_chat_without_api_key_returns_500(client, loaded_repo_id, monkeypatch):
    monkeypatch.setattr("app.config.settings.anthropic_api_key", "")
    monkeypatch.setattr("app.agent.core._client", None)
    resp = client.post(f"/api/repos/{loaded_repo_id}/chat", json={"message": "hello"})
    assert resp.status_code == 500
    assert "ANTHROPIC_API_KEY" in resp.json()["detail"]
