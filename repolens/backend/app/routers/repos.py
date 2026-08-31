import logging

from fastapi import APIRouter, HTTPException

from app.deps import get_repo_or_404
from app.repo_manager import RepoLoadError, repo_manager
from app.schemas import DirectoryListing, LoadRepoRequest, RepoSummary
from app.tools.file_tools import list_directory

logger = logging.getLogger("repolens.repos")

router = APIRouter(prefix="/api/repos", tags=["repos"])

INDEX_STEPS = [
    "Repository indexed",
    "Architecture analyzed",
    "Entry points identified",
    "Dependencies analyzed",
]


def _to_summary(repo) -> RepoSummary:
    analysis = repo.analysis
    return RepoSummary(
        id=repo.id,
        name=repo.name,
        source=repo.source,
        is_local=repo.is_local,
        file_count=analysis.file_count if analysis else 0,
        languages=analysis.languages if analysis else {},
        frameworks=analysis.frameworks if analysis else [],
        entry_points=analysis.entry_points if analysis else [],
        steps=INDEX_STEPS,
    )


@router.post("", response_model=RepoSummary)
def load_repo(payload: LoadRepoRequest) -> RepoSummary:
    try:
        repo = repo_manager.load(payload.source)
    except RepoLoadError as exc:
        logger.warning("Failed to load repo %s: %s", payload.source, exc)
        raise HTTPException(status_code=400, detail=str(exc))
    return _to_summary(repo)


@router.get("", response_model=list[RepoSummary])
def list_repos() -> list[RepoSummary]:
    return [_to_summary(r) for r in repo_manager.list()]


@router.get("/{repo_id}", response_model=RepoSummary)
def get_repo(repo_id: str) -> RepoSummary:
    repo = get_repo_or_404(repo_id)
    return _to_summary(repo)


@router.get("/{repo_id}/analysis")
def get_analysis(repo_id: str) -> dict:
    repo = get_repo_or_404(repo_id)
    return repo.analysis.to_dict() if repo.analysis else {}


@router.get("/{repo_id}/files", response_model=DirectoryListing)
def get_files(repo_id: str, path: str = ".") -> DirectoryListing:
    repo = get_repo_or_404(repo_id)
    result = list_directory(repo.path, path)
    if "error" in result:
        raise HTTPException(status_code=400, detail=result["error"])
    return DirectoryListing(**result)


@router.delete("/{repo_id}", status_code=204)
def delete_repo(repo_id: str) -> None:
    get_repo_or_404(repo_id)
    repo_manager.remove(repo_id)
