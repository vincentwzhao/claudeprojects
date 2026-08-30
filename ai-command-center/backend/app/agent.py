"""The agent loop — the core learning piece of this project.

A plain Python generator that:
  1. builds the message list (system prompt + history) from the DB,
  2. calls Claude with tool definitions,
  3. inspects the response for tool_use blocks,
  4. executes safe tools immediately, or pauses on destructive ones
     until a human approves/denies them via the frontend,
  5. feeds tool_result blocks back and repeats,
  6. stops once Claude returns a plain text (non tool_use) response.

Nothing here is hidden behind an agent framework — every step is visible
so the loop is easy to read end to end.
"""

import json
from collections.abc import Generator

import anthropic
from sqlalchemy.orm import Session

from . import models
from .config import settings
from .database import SessionLocal
from .tools import TOOLS, anthropic_tool_definitions

SYSTEM_PROMPT = (
    "You are the assistant inside a personal AI command center. You can search the "
    "web, read files the user uploaded, and manage the user's task list. Destructive "
    "actions (deleting a task or file) require the user's explicit approval before "
    "they run — if you call one of those tools, the system will pause and ask the "
    "user, so just call it when it's the right action; don't ask the user to confirm "
    "in words first. Be concise."
)

MAX_TOKENS = 2048

_client: anthropic.Anthropic | None = None


def _get_client() -> anthropic.Anthropic:
    global _client
    if _client is None:
        if not settings.anthropic_api_key:
            raise RuntimeError(
                "ANTHROPIC_API_KEY is not set. Add it to backend/.env (see .env.example)."
            )
        _client = anthropic.Anthropic(api_key=settings.anthropic_api_key)
    return _client


def _sse(event: str, data: dict) -> str:
    return f"event: {event}\ndata: {json.dumps(data)}\n\n"


def _build_api_messages(db: Session, conversation_id: int) -> list[dict]:
    """Reconstruct the Claude API message list from persisted DB rows.

    Conversation state lives in the DB, not in server memory, so this
    works identically whether we're starting a fresh turn or resuming
    a loop that paused for approval (even across a server restart).
    """
    rows = (
        db.query(models.Message)
        .filter(models.Message.conversation_id == conversation_id)
        .order_by(models.Message.id)
        .all()
    )
    api_messages: list[dict] = []
    for row in rows:
        if row.role == "user":
            api_messages.append({"role": "user", "content": row.content})
        elif row.role == "assistant":
            content = json.loads(row.raw_json) if row.raw_json else row.content
            api_messages.append({"role": "assistant", "content": content})
        elif row.role == "tool":
            content = json.loads(row.raw_json) if row.raw_json else []
            api_messages.append({"role": "user", "content": content})
    return api_messages


def _agent_loop(
    db: Session, conversation: models.Conversation, user_id: int, api_messages: list[dict]
) -> Generator[str, None, None]:
    try:
        client = _get_client()
    except RuntimeError as exc:
        yield _sse("error", {"message": str(exc)})
        return

    while True:
        text_so_far = ""
        try:
            with client.messages.stream(
                model=settings.claude_model,
                max_tokens=MAX_TOKENS,
                system=SYSTEM_PROMPT,
                messages=api_messages,
                tools=anthropic_tool_definitions(),
            ) as stream:
                for event in stream.text_stream:
                    text_so_far += event
                    yield _sse("token", {"text": event})
                final_message = stream.get_final_message()
        except anthropic.APIError as exc:
            yield _sse("error", {"message": f"Claude API error: {exc}"})
            return

        content_blocks = [b.model_dump() for b in final_message.content]
        assistant_row = models.Message(
            conversation_id=conversation.id,
            role="assistant",
            content=text_so_far,
            raw_json=json.dumps(content_blocks),
        )
        db.add(assistant_row)
        db.commit()
        db.refresh(assistant_row)
        api_messages.append({"role": "assistant", "content": content_blocks})

        if final_message.stop_reason != "tool_use":
            yield _sse("done", {"text": text_so_far})
            return

        tool_use_blocks = [b for b in final_message.content if b.type == "tool_use"]
        tool_result_blocks = []
        waiting_on_approval = False

        for block in tool_use_blocks:
            spec = TOOLS.get(block.name)
            if spec is None:
                tool_result_blocks.append(
                    {
                        "type": "tool_result",
                        "tool_use_id": block.id,
                        "content": json.dumps({"error": f"Unknown tool {block.name}"}),
                        "is_error": True,
                    }
                )
                continue

            if spec.requires_approval:
                tc = models.ToolCall(
                    conversation_id=conversation.id,
                    message_id=assistant_row.id,
                    tool_use_id=block.id,
                    tool_name=block.name,
                    arguments_json=json.dumps(block.input),
                    requires_approval=True,
                    status="pending",
                )
                db.add(tc)
                db.commit()
                db.refresh(tc)
                waiting_on_approval = True
                yield _sse(
                    "approval_needed",
                    {
                        "tool_call_id": tc.id,
                        "tool_name": tc.tool_name,
                        "arguments": block.input,
                    },
                )
            else:
                result = spec.handler(db, user_id, block.input)
                tc = models.ToolCall(
                    conversation_id=conversation.id,
                    message_id=assistant_row.id,
                    tool_use_id=block.id,
                    tool_name=block.name,
                    arguments_json=json.dumps(block.input),
                    requires_approval=False,
                    status="executed",
                    result_json=json.dumps(result),
                )
                db.add(tc)
                db.commit()
                tool_result_blocks.append(
                    {
                        "type": "tool_result",
                        "tool_use_id": block.id,
                        "content": json.dumps(result),
                    }
                )
                yield _sse(
                    "tool_result",
                    {"tool_name": block.name, "arguments": block.input, "result": result},
                )

        if waiting_on_approval:
            # Stop the loop entirely: some tool_use blocks in this turn still
            # need a human decision. resume_after_approval() picks up once
            # every pending tool_call for this message is resolved.
            return

        tool_row = models.Message(
            conversation_id=conversation.id,
            role="tool",
            content=f"{len(tool_result_blocks)} tool result(s)",
            raw_json=json.dumps(tool_result_blocks),
        )
        db.add(tool_row)
        db.commit()
        api_messages.append({"role": "user", "content": tool_result_blocks})
        # loop continues: feed tool results back to Claude


def stream_chat_turn(
    conversation_id: int, user_id: int, user_text: str
) -> Generator[str, None, None]:
    """Owns its own DB session for the lifetime of the stream.

    A request-scoped session (FastAPI's `Depends(get_db)`) gets torn down
    around the *handler* call, but this generator keeps running — across
    several Claude round trips — after the handler has already returned
    the StreamingResponse. So it opens and closes its own session instead
    of borrowing one that could be closed out from under it mid-stream.
    """
    db = SessionLocal()
    try:
        conversation = db.get(models.Conversation, conversation_id)
        user_row = models.Message(conversation_id=conversation.id, role="user", content=user_text)
        db.add(user_row)
        db.commit()

        api_messages = _build_api_messages(db, conversation.id)
        yield from _agent_loop(db, conversation, user_id, api_messages)
    finally:
        db.close()


def resume_after_approval(
    conversation_id: int, user_id: int, message_id: int
) -> Generator[str, None, None] | None:
    """Continue the loop once every tool_call for `message_id` is resolved.

    Returns None (nothing to stream yet) if sibling tool_calls from the
    same assistant turn are still pending.
    """
    db = SessionLocal()
    tool_calls = (
        db.query(models.ToolCall)
        .filter(models.ToolCall.message_id == message_id)
        .order_by(models.ToolCall.id)
        .all()
    )
    if any(tc.status == "pending" for tc in tool_calls):
        db.close()
        return None

    tool_result_blocks = []
    for tc in tool_calls:
        if tc.status == "approved":
            spec = TOOLS[tc.tool_name]
            result = spec.handler(db, user_id, json.loads(tc.arguments_json))
            tc.status = "executed"
            tc.result_json = json.dumps(result)
        elif tc.status == "denied":
            result = {"error": "The user denied this action."}
            if tc.result_json is None:
                tc.result_json = json.dumps(result)
        else:
            result = json.loads(tc.result_json) if tc.result_json else {}
        tool_result_blocks.append(
            {
                "type": "tool_result",
                "tool_use_id": tc.tool_use_id,
                "content": tc.result_json or json.dumps(result),
            }
        )
    db.commit()

    conversation = db.get(models.Conversation, conversation_id)
    tool_row = models.Message(
        conversation_id=conversation.id,
        role="tool",
        content=f"{len(tool_result_blocks)} tool result(s)",
        raw_json=json.dumps(tool_result_blocks),
    )
    db.add(tool_row)
    db.commit()

    api_messages = _build_api_messages(db, conversation.id)

    def _run():
        try:
            yield from _agent_loop(db, conversation, user_id, api_messages)
        finally:
            db.close()

    return _run()
