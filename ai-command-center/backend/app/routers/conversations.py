from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from .. import models, schemas
from ..database import get_db
from ..deps import get_or_create_default_user

router = APIRouter(prefix="/conversations", tags=["conversations"])


@router.get("", response_model=list[schemas.ConversationOut])
def list_conversations(db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    convos = (
        db.query(models.Conversation)
        .filter(models.Conversation.user_id == user.id)
        .order_by(models.Conversation.updated_at.desc())
        .all()
    )
    return convos


@router.get("/{conversation_id}/messages", response_model=list[schemas.MessageOut])
def get_messages(conversation_id: int, db: Session = Depends(get_db)):
    convo = db.query(models.Conversation).filter(models.Conversation.id == conversation_id).first()
    if convo is None:
        raise HTTPException(404, "Conversation not found")
    return [m for m in convo.messages if m.role in ("user", "assistant")]
