import logging

from fastapi import APIRouter, HTTPException

from app.agent import run_agent
from app.agent.prompts import (
    PROJECT_SYSTEM_PROMPT,
    build_chat_system_prompt,
    build_chat_user_message,
    build_explain_user_message,
    build_project_user_message,
    build_trace_user_message,
)
from app.deps import get_repo_or_404
from app.schemas import AgentResponse, ChatRequest, ExplainRequest, TraceRequest

logger = logging.getLogger("repolens.commands")

router = APIRouter(prefix="/api/repos", tags=["commands"])


def _run(repo_id: str, system_prompt: str, user_message: str, max_iterations: int | None = None) -> AgentResponse:
    repo = get_repo_or_404(repo_id)
    try:
        result = run_agent(repo.path, system_prompt, user_message, max_iterations=max_iterations)
    except RuntimeError as exc:
        # e.g. missing ANTHROPIC_API_KEY
        raise HTTPException(status_code=500, detail=str(exc))
    except Exception as exc:
        logger.exception("Agent run failed for repo=%s", repo_id)
        raise HTTPException(status_code=502, detail=f"Agent failed: {exc}")

    return AgentResponse(
        answer=result.answer,
        references=[{"file": r.file, "line": r.line} for r in result.references],
        trace=[{"tool": t.tool, "input": t.input, "output_summary": t.output_summary} for t in result.trace],
    )


@router.post("/{repo_id}/chat", response_model=AgentResponse)
def chat(repo_id: str, payload: ChatRequest) -> AgentResponse:
    repo = get_repo_or_404(repo_id)
    system_prompt = build_chat_system_prompt(repo.analysis.to_dict() if repo.analysis else {})
    return _run(repo_id, system_prompt, build_chat_user_message(payload.message))


@router.post("/{repo_id}/explain", response_model=AgentResponse)
def explain(repo_id: str, payload: ExplainRequest) -> AgentResponse:
    repo = get_repo_or_404(repo_id)
    system_prompt = build_chat_system_prompt(repo.analysis.to_dict() if repo.analysis else {})
    return _run(repo_id, system_prompt, build_explain_user_message(payload.concept))


@router.post("/{repo_id}/trace", response_model=AgentResponse)
def trace(repo_id: str, payload: TraceRequest) -> AgentResponse:
    repo = get_repo_or_404(repo_id)
    system_prompt = build_chat_system_prompt(repo.analysis.to_dict() if repo.analysis else {})
    return _run(repo_id, system_prompt, build_trace_user_message(payload.feature), max_iterations=10)


@router.post("/{repo_id}/project", response_model=AgentResponse)
def project_overview(repo_id: str) -> AgentResponse:
    repo = get_repo_or_404(repo_id)
    analysis_dict = repo.analysis.to_dict() if repo.analysis else {}
    return _run(repo_id, PROJECT_SYSTEM_PROMPT, build_project_user_message(analysis_dict), max_iterations=5)
