import datetime

from pydantic import BaseModel, ConfigDict


class ConversationOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id: int
    title: str
    created_at: datetime.datetime
    updated_at: datetime.datetime


class MessageOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id: int
    role: str
    content: str
    created_at: datetime.datetime


class ChatRequest(BaseModel):
    conversation_id: int | None = None
    message: str


class TaskCreateIn(BaseModel):
    title: str
    description: str = ""
    status: str = "todo"
    due_date: datetime.datetime | None = None


class TaskUpdateIn(BaseModel):
    title: str | None = None
    description: str | None = None
    status: str | None = None
    due_date: datetime.datetime | None = None


class TaskOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id: int
    title: str
    description: str
    status: str
    due_date: datetime.datetime | None
    created_at: datetime.datetime
    updated_at: datetime.datetime


class FileOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id: int
    filename: str
    mime_type: str
    uploaded_at: datetime.datetime
    extracted_chars: int = 0


class ToolCallOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id: int
    conversation_id: int
    tool_name: str
    arguments_json: str
    requires_approval: bool
    status: str
    result_json: str | None
    created_at: datetime.datetime
    resolved_at: datetime.datetime | None


class ApprovalDecisionIn(BaseModel):
    approve: bool
