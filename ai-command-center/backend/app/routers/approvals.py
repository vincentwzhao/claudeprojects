from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import JSONResponse, StreamingResponse
from sqlalchemy.orm import Session

from .. import agent, models, schemas
from ..database import get_db
from ..deps import get_or_create_default_user

router = APIRouter(prefix="/approvals", tags=["approvals"])


@router.get("", response_model=list[schemas.ToolCallOut])
def list_pending_approvals(db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    return (
        db.query(models.ToolCall)
        .join(models.Conversation, models.ToolCall.conversation_id == models.Conversation.id)
        .filter(models.Conversation.user_id == user.id, models.ToolCall.status == "pending")
        .order_by(models.ToolCall.created_at)
        .all()
    )


@router.post("/{tool_call_id}/decision")
def decide(tool_call_id: int, body: schemas.ApprovalDecisionIn, db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    tc = db.query(models.ToolCall).filter(models.ToolCall.id == tool_call_id).first()
    if tc is None:
        raise HTTPException(404, "Tool call not found")
    if tc.status != "pending":
        raise HTTPException(400, f"Tool call already {tc.status}")

    import datetime

    tc.status = "approved" if body.approve else "denied"
    tc.resolved_at = datetime.datetime.utcnow()
    db.commit()

    conversation_id = tc.conversation_id
    message_id = tc.message_id
    user_id = user.id
    db.close()

    generator = agent.resume_after_approval(conversation_id, user_id, message_id)
    if generator is None:
        return JSONResponse({"status": "waiting_on_other_approvals"})
    return StreamingResponse(generator, media_type="text/event-stream")
