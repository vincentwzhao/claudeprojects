from sqlalchemy.orm import Session

from . import models

DEFAULT_USER_EMAIL = "local@ai-command-center"


def get_or_create_default_user(db: Session) -> models.User:
    """Single-user app for now (see ARCHITECTURE_PLAN.md) — but a real
    users table means adding accounts later doesn't need a schema change."""
    user = db.query(models.User).filter(models.User.email == DEFAULT_USER_EMAIL).first()
    if user is None:
        user = models.User(email=DEFAULT_USER_EMAIL)
        db.add(user)
        db.commit()
        db.refresh(user)
    return user
