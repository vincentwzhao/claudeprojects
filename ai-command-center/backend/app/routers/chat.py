import json

from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import StreamingResponse
from sqlalchemy.orm import Session

from .. import agent, models, schemas
from ..database import get_db
from ..deps import get_or_create_default_user

router = APIRouter(tags=["chat"])


def _prefixed_stream(conversation_id: int, generator):
    yield f"event: conversation\ndata: {json.dumps({'conversation_id': conversation_id})}\n\n"
    yield from generator


@router.post("/chat")
def chat(req: schemas.ChatRequest, db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)

    if req.conversation_id is not None:
        conversation = (
            db.query(models.Conversation)
            .filter(models.Conversation.id == req.conversation_id, models.Conversation.user_id == user.id)
            .first()
        )
        if conversation is None:
            raise HTTPException(404, "Conversation not found")
    else:
        title = req.message.strip()[:50] or "New conversation"
        conversation = models.Conversation(user_id=user.id, title=title)
        db.add(conversation)
        db.commit()
        db.refresh(conversation)

    conversation_id = conversation.id
    user_id = user.id
    generator = agent.stream_chat_turn(conversation_id, user_id, req.message)
    return StreamingResponse(
        _prefixed_stream(conversation_id, generator), media_type="text/event-stream"
    )
