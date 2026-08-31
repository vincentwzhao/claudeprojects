from fastapi import HTTPException

from app.repo_manager import Repo, repo_manager


def get_repo_or_404(repo_id: str) -> Repo:
    try:
        return repo_manager.get(repo_id)
    except KeyError:
        raise HTTPException(status_code=404, detail=f"Repository '{repo_id}' not found. Load it first via POST /api/repos.")
