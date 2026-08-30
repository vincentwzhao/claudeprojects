from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from .. import models, schemas
from ..database import get_db
from ..deps import get_or_create_default_user

router = APIRouter(prefix="/tasks", tags=["tasks"])


@router.get("", response_model=list[schemas.TaskOut])
def list_tasks(db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    return (
        db.query(models.Task)
        .filter(models.Task.user_id == user.id)
        .order_by(models.Task.created_at.desc())
        .all()
    )


@router.post("", response_model=schemas.TaskOut)
def create_task(body: schemas.TaskCreateIn, db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    task = models.Task(user_id=user.id, **body.model_dump())
    db.add(task)
    db.commit()
    db.refresh(task)
    return task


@router.patch("/{task_id}", response_model=schemas.TaskOut)
def update_task(task_id: int, body: schemas.TaskUpdateIn, db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    task = (
        db.query(models.Task)
        .filter(models.Task.id == task_id, models.Task.user_id == user.id)
        .first()
    )
    if task is None:
        raise HTTPException(404, "Task not found")
    for field, value in body.model_dump(exclude_unset=True).items():
        setattr(task, field, value)
    db.commit()
    db.refresh(task)
    return task


@router.delete("/{task_id}")
def delete_task(task_id: int, db: Session = Depends(get_db)):
    # Directly deleted by the human via the dashboard — not an agent tool
    # call, so it doesn't go through the approval workflow.
    user = get_or_create_default_user(db)
    task = (
        db.query(models.Task)
        .filter(models.Task.id == task_id, models.Task.user_id == user.id)
        .first()
    )
    if task is None:
        raise HTTPException(404, "Task not found")
    db.delete(task)
    db.commit()
    return {"deleted_id": task_id}
