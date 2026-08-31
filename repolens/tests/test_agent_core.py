"""Tests for the agent's tool-selection loop, with the Anthropic API mocked
out. These verify the AGENT WIRING (it calls the right tool, feeds results
back, stops correctly, extracts references) without needing network access
or an API key.
"""

from types import SimpleNamespace

import pytest

from app.agent.core import run_agent


class FakeMessages:
    def __init__(self, responses):
        self._responses = list(responses)
        self.calls = []

    def create(self, **kwargs):
        self.calls.append(kwargs)
        return self._responses.pop(0)


class FakeClient:
    def __init__(self, responses):
        self.messages = FakeMessages(responses)


def tool_use_response(name, tool_input, tool_id="tool_1"):
    block = SimpleNamespace(type="tool_use", name=name, input=tool_input, id=tool_id)
    return SimpleNamespace(stop_reason="tool_use", content=[block])


def text_response(text):
    block = SimpleNamespace(type="text", text=text)
    return SimpleNamespace(stop_reason="end_turn", content=[block])


@pytest.fixture
def patch_client(monkeypatch):
    def _patch(responses):
        fake = FakeClient(responses)
        monkeypatch.setattr("app.agent.core._client", fake)
        monkeypatch.setattr("app.agent.core._get_client", lambda: fake)
        return fake

    return _patch


def test_agent_picks_search_code_then_reads_file(sample_repo_path, patch_client):
    fake = patch_client([
        tool_use_response("search_code", {"query": "async function login"}),
        tool_use_response("read_file", {"path": "src/services/authService.js"}, tool_id="tool_2"),
        text_response(
            "Login is handled in src/services/authService.js:12.\n\nRelevant files:\n- src/services/authService.js"
        ),
    ])

    result = run_agent(sample_repo_path, "system", "How does login work?")

    tool_names = [t.tool for t in result.trace]
    assert tool_names == ["search_code", "read_file"]
    assert "src/services/authService.js" in result.answer
    assert any(r.file == "src/services/authService.js" for r in result.references)
    assert fake.messages.calls[0]["messages"][0]["content"] == "How does login work?"


def test_agent_stops_on_end_turn_without_tools(sample_repo_path, patch_client):
    patch_client([text_response("I couldn't find enough evidence for that.")])

    result = run_agent(sample_repo_path, "system", "Does this repo do payments?")

    assert result.trace == []
    assert "couldn't find" in result.answer.lower()


def test_agent_surfaces_tool_errors_but_keeps_going(sample_repo_path, patch_client):
    patch_client([
        tool_use_response("read_file", {"path": "does/not/exist.js"}),
        text_response("That file doesn't exist in this repository. Relevant files: none found."),
    ])

    result = run_agent(sample_repo_path, "system", "What does does/not/exist.js do?")

    assert result.trace[0].output_summary.startswith("error:")
    assert "doesn't exist" in result.answer


def test_agent_respects_max_iterations(sample_repo_path, patch_client):
    # Two tool_use rounds fill the max_iterations=2 budget; the loop should
    # then force one final, tool-free answer instead of looping forever.
    responses = [
        tool_use_response("search_files", {"pattern": "auth"}, tool_id=f"t{i}") for i in range(2)
    ]
    responses.append(text_response("Forced final answer."))
    patch_client(responses)

    result = run_agent(sample_repo_path, "system", "question", max_iterations=2)

    assert result.stopped_reason == "max_iterations"
    assert result.answer == "Forced final answer."
    assert len(result.trace) == 2
