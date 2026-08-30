"""Tool layer: each tool is a function + a JSON schema describing its
inputs (what Claude sees), plus a `requires_approval` flag.

That flag is the entire human-in-the-loop safety mechanism: the agent
loop (see agent.py) checks it before running a tool. Everything else
about a tool is identical whether it's safe or destructive.
"""

from dataclasses import dataclass
from typing import Any, Callable

import httpx
from sqlalchemy.orm import Session

from . import models
from .config import settings


@dataclass
class ToolSpec:
    name: str
    description: str
    input_schema: dict[str, Any]
    requires_approval: bool
    handler: Callable[[Session, int, dict[str, Any]], dict[str, Any]]


# ---- handlers -----------------------------------------------------------


def _web_search(db: Session, user_id: int, args: dict[str, Any]) -> dict[str, Any]:
    query = args["query"]
    if not settings.tavily_api_key:
        return {
            "error": "No TAVILY_API_KEY configured. Set it in backend/.env to enable web search.",
        }
    resp = httpx.post(
        "https://api.tavily.com/search",
        json={"api_key": settings.tavily_api_key, "query": query, "max_results": 5},
        timeout=20,
    )
    resp.raise_for_status()
    data = resp.json()
    results = [
        {"title": r.get("title"), "url": r.get("url"), "content": r.get("content")}
        for r in data.get("results", [])
    ]
    return {"query": query, "results": results}


def _read_file(db: Session, user_id: int, args: dict[str, Any]) -> dict[str, Any]:
    file_id = args["file_id"]
    record = (
        db.query(models.FileRecord)
        .filter(models.FileRecord.id == file_id, models.FileRecord.user_id == user_id)
        .first()
    )
    if record is None:
        return {"error": f"No file with id {file_id}"}
    text = record.extracted_text or ""
    truncated = text[:20000]
    return {
        "filename": record.filename,
        "mime_type": record.mime_type,
        "text": truncated,
        "truncated": len(text) > len(truncated),
    }


def _list_tasks(db: Session, user_id: int, args: dict[str, Any]) -> dict[str, Any]:
    query = db.query(models.Task).filter(models.Task.user_id == user_id)
    status = args.get("status")
    if status:
        query = query.filter(models.Task.status == status)
    tasks = query.order_by(models.Task.created_at.desc()).all()
    return {
        "tasks": [
            {
                "id": t.id,
                "title": t.title,
                "description": t.description,
                "status": t.status,
                "due_date": t.due_date.isoformat() if t.due_date else None,
            }
            for t in tasks
        ]
    }


def _create_task(db: Session, user_id: int, args: dict[str, Any]) -> dict[str, Any]:
    task = models.Task(
        user_id=user_id,
        title=args["title"],
        description=args.get("description", ""),
        status=args.get("status", "todo"),
    )
    db.add(task)
    db.commit()
    db.refresh(task)
    return {"id": task.id, "title": task.title, "status": task.status}


def _update_task(db: Session, user_id: int, args: dict[str, Any]) -> dict[str, Any]:
    task = (
        db.query(models.Task)
        .filter(models.Task.id == args["task_id"], models.Task.user_id == user_id)
        .first()
    )
    if task is None:
        return {"error": f"No task with id {args['task_id']}"}
    for field in ("title", "description", "status"):
        if args.get(field) is not None:
            setattr(task, field, args[field])
    db.commit()
    db.refresh(task)
    return {"id": task.id, "title": task.title, "status": task.status}


def _delete_task(db: Session, user_id: int, args: dict[str, Any]) -> dict[str, Any]:
    task = (
        db.query(models.Task)
        .filter(models.Task.id == args["task_id"], models.Task.user_id == user_id)
        .first()
    )
    if task is None:
        return {"error": f"No task with id {args['task_id']}"}
    db.delete(task)
    db.commit()
    return {"deleted_id": args["task_id"]}


def _delete_file(db: Session, user_id: int, args: dict[str, Any]) -> dict[str, Any]:
    record = (
        db.query(models.FileRecord)
        .filter(models.FileRecord.id == args["file_id"], models.FileRecord.user_id == user_id)
        .first()
    )
    if record is None:
        return {"error": f"No file with id {args['file_id']}"}
    db.delete(record)
    db.commit()
    return {"deleted_id": args["file_id"]}


# ---- registry -------------------------------------------------------------

TOOLS: dict[str, ToolSpec] = {
    "web_search": ToolSpec(
        name="web_search",
        description="Search the web for current information. Use this when the user asks about "
        "something you might not know or that could have changed recently.",
        input_schema={
            "type": "object",
            "properties": {"query": {"type": "string", "description": "The search query."}},
            "required": ["query"],
        },
        requires_approval=False,
        handler=_web_search,
    ),
    "read_file": ToolSpec(
        name="read_file",
        description="Read the extracted text content of a file the user has uploaded.",
        input_schema={
            "type": "object",
            "properties": {"file_id": {"type": "integer", "description": "The file's id."}},
            "required": ["file_id"],
        },
        requires_approval=False,
        handler=_read_file,
    ),
    "list_tasks": ToolSpec(
        name="list_tasks",
        description="List the user's tasks, optionally filtered by status.",
        input_schema={
            "type": "object",
            "properties": {
                "status": {
                    "type": "string",
                    "enum": ["todo", "in_progress", "done"],
                    "description": "Optional status filter.",
                }
            },
        },
        requires_approval=False,
        handler=_list_tasks,
    ),
    "create_task": ToolSpec(
        name="create_task",
        description="Create a new task for the user.",
        input_schema={
            "type": "object",
            "properties": {
                "title": {"type": "string"},
                "description": {"type": "string"},
                "status": {"type": "string", "enum": ["todo", "in_progress", "done"]},
            },
            "required": ["title"],
        },
        requires_approval=False,
        handler=_create_task,
    ),
    "update_task": ToolSpec(
        name="update_task",
        description="Update an existing task's title, description, or status.",
        input_schema={
            "type": "object",
            "properties": {
                "task_id": {"type": "integer"},
                "title": {"type": "string"},
                "description": {"type": "string"},
                "status": {"type": "string", "enum": ["todo", "in_progress", "done"]},
            },
            "required": ["task_id"],
        },
        requires_approval=False,
        handler=_update_task,
    ),
    "delete_task": ToolSpec(
        name="delete_task",
        description="Permanently delete a task. Destructive — requires user approval.",
        input_schema={
            "type": "object",
            "properties": {"task_id": {"type": "integer"}},
            "required": ["task_id"],
        },
        requires_approval=True,
        handler=_delete_task,
    ),
    "delete_file": ToolSpec(
        name="delete_file",
        description="Permanently delete an uploaded file. Destructive — requires user approval.",
        input_schema={
            "type": "object",
            "properties": {"file_id": {"type": "integer"}},
            "required": ["file_id"],
        },
        requires_approval=True,
        handler=_delete_file,
    ),
}


def anthropic_tool_definitions() -> list[dict[str, Any]]:
    return [
        {"name": t.name, "description": t.description, "input_schema": t.input_schema}
        for t in TOOLS.values()
    ]
