"""The agent loop: User -> Agent -> choose tool -> inspect repository ->
reason over results -> answer. This is a hand-written loop against the
Anthropic Messages API's tool-use protocol (no framework), so the control
flow is easy to follow and to test.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import anthropic

from app.config import settings
from app.tools import TOOL_SCHEMAS, execute_tool

_client: anthropic.Anthropic | None = None

CITATION_RE = re.compile(r"([A-Za-z0-9_./\-]+\.[A-Za-z0-9]{1,8}):(\d+)")


def _get_client() -> anthropic.Anthropic:
    global _client
    if _client is None:
        if not settings.anthropic_api_key:
            raise RuntimeError(
                "ANTHROPIC_API_KEY is not set. Add it to backend/.env (see .env.example)."
            )
        _client = anthropic.Anthropic(api_key=settings.anthropic_api_key)
    return _client


@dataclass
class ToolCallRecord:
    tool: str
    input: dict[str, Any]
    output_summary: str


@dataclass
class Reference:
    file: str
    line: int | None = None

    def key(self) -> tuple[str, int | None]:
        return (self.file, self.line)


@dataclass
class AgentResult:
    answer: str
    trace: list[ToolCallRecord] = field(default_factory=list)
    references: list[Reference] = field(default_factory=list)
    stopped_reason: str = "end_turn"


def _summarize_tool_output(name: str, result: dict[str, Any]) -> str:
    if "error" in result:
        return f"error: {result['error']}"
    if name == "search_files":
        return f"{result.get('count', 0)} file(s) matched"
    if name == "search_code":
        return f"{result.get('count', 0)} match(es)"
    if name == "read_file":
        return f"read {result.get('total_lines', '?')} lines"
    if name == "list_directory":
        return f"{len(result.get('entries', []))} entries"
    if name == "get_git_history":
        return f"{result.get('count', 0)} commit(s)"
    return "ok"


def _collect_reference_from_tool(name: str, tool_input: dict[str, Any], result: dict[str, Any]) -> Reference | None:
    if "error" in result:
        return None
    if name == "read_file" and result.get("path"):
        line = tool_input.get("start_line")
        return Reference(file=result["path"], line=int(line) if line else None)
    return None


def _extract_citations(text: str) -> list[Reference]:
    return [Reference(file=m.group(1), line=int(m.group(2))) for m in CITATION_RE.finditer(text)]


def _dedupe_references(refs: list[Reference]) -> list[Reference]:
    by_file: dict[str, Reference] = {}
    for ref in refs:
        existing = by_file.get(ref.file)
        if existing is None or (existing.line is None and ref.line is not None):
            by_file[ref.file] = ref
    return list(by_file.values())


def run_agent(
    repo_root: Path,
    system_prompt: str,
    user_message: str,
    max_iterations: int | None = None,
) -> AgentResult:
    client = _get_client()
    max_iterations = max_iterations or settings.max_agent_iterations

    messages: list[dict[str, Any]] = [{"role": "user", "content": user_message}]
    trace: list[ToolCallRecord] = []
    tool_refs: list[Reference] = []

    final_text = ""
    stopped_reason = "end_turn"

    for iteration in range(max_iterations):
        response = client.messages.create(
            model=settings.claude_model,
            max_tokens=2048,
            system=system_prompt,
            tools=TOOL_SCHEMAS,
            messages=messages,
        )
        messages.append({"role": "assistant", "content": response.content})

        if response.stop_reason != "tool_use":
            final_text = "".join(
                block.text for block in response.content if getattr(block, "type", None) == "text"
            )
            stopped_reason = response.stop_reason
            break

        tool_results = []
        for block in response.content:
            if getattr(block, "type", None) != "tool_use":
                continue
            result = execute_tool(repo_root, block.name, block.input)
            trace.append(ToolCallRecord(tool=block.name, input=block.input,
                                         output_summary=_summarize_tool_output(block.name, result)))
            ref = _collect_reference_from_tool(block.name, block.input, result)
            if ref:
                tool_refs.append(ref)

            tool_results.append({
                "type": "tool_result",
                "tool_use_id": block.id,
                "content": _stringify_tool_result(result),
                "is_error": "error" in result,
            })
        messages.append({"role": "user", "content": tool_results})
    else:
        stopped_reason = "max_iterations"
        # ask once more, forcing a final answer without further tool use
        messages.append({
            "role": "user",
            "content": "You've used many tool calls. Please give your best final answer now, "
                        "based on everything you've found so far, following the same citation rules.",
        })
        response = client.messages.create(
            model=settings.claude_model,
            max_tokens=2048,
            system=system_prompt,
            messages=messages,
        )
        final_text = "".join(
            block.text for block in response.content if getattr(block, "type", None) == "text"
        )

    if not final_text:
        final_text = "I wasn't able to produce a grounded answer for this request."

    references = _dedupe_references(tool_refs + _extract_citations(final_text))

    return AgentResult(answer=final_text, trace=trace, references=references, stopped_reason=stopped_reason)


def _stringify_tool_result(result: dict[str, Any]) -> str:
    import json
    return json.dumps(result)[:8000]
