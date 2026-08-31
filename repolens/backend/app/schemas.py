from __future__ import annotations

from pydantic import BaseModel, Field


class LoadRepoRequest(BaseModel):
    source: str = Field(..., min_length=1, description="GitHub URL or local filesystem path")


class RepoSummary(BaseModel):
    id: str
    name: str
    source: str
    is_local: bool
    file_count: int
    languages: dict[str, int]
    frameworks: list[str]
    entry_points: list[dict]
    steps: list[str]


class ChatRequest(BaseModel):
    message: str = Field(..., min_length=1, max_length=2000)


class ExplainRequest(BaseModel):
    concept: str = Field(..., min_length=1, max_length=200)


class TraceRequest(BaseModel):
    feature: str = Field(..., min_length=1, max_length=200)


class ReferenceOut(BaseModel):
    file: str
    line: int | None = None


class ToolCallOut(BaseModel):
    tool: str
    input: dict
    output_summary: str


class AgentResponse(BaseModel):
    answer: str
    references: list[ReferenceOut]
    trace: list[ToolCallOut]


class DirectoryListing(BaseModel):
    path: str
    entries: list[dict]
