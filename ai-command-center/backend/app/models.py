import datetime

from sqlalchemy import CheckConstraint, ForeignKey, String, Text
from sqlalchemy.orm import Mapped, mapped_column, relationship

from .database import Base


def utcnow() -> datetime.datetime:
    return datetime.datetime.utcnow()


class User(Base):
    __tablename__ = "users"

    id: Mapped[int] = mapped_column(primary_key=True)
    email: Mapped[str] = mapped_column(String, unique=True)
    created_at: Mapped[datetime.datetime] = mapped_column(default=utcnow)

    conversations: Mapped[list["Conversation"]] = relationship(back_populates="user")
    tasks: Mapped[list["Task"]] = relationship(back_populates="user")
    files: Mapped[list["FileRecord"]] = relationship(back_populates="user")


class Conversation(Base):
    __tablename__ = "conversations"

    id: Mapped[int] = mapped_column(primary_key=True)
    user_id: Mapped[int] = mapped_column(ForeignKey("users.id"))
    title: Mapped[str] = mapped_column(String, default="New conversation")
    created_at: Mapped[datetime.datetime] = mapped_column(default=utcnow)
    updated_at: Mapped[datetime.datetime] = mapped_column(default=utcnow, onupdate=utcnow)

    user: Mapped["User"] = relationship(back_populates="conversations")
    messages: Mapped[list["Message"]] = relationship(
        back_populates="conversation", cascade="all, delete-orphan", order_by="Message.id"
    )


class Message(Base):
    __tablename__ = "messages"
    __table_args__ = (CheckConstraint("role in ('user','assistant','tool')", name="ck_message_role"),)

    id: Mapped[int] = mapped_column(primary_key=True)
    conversation_id: Mapped[int] = mapped_column(ForeignKey("conversations.id"))
    role: Mapped[str] = mapped_column(String)
    content: Mapped[str] = mapped_column(Text, default="")
    raw_json: Mapped[str | None] = mapped_column(Text, nullable=True)
    created_at: Mapped[datetime.datetime] = mapped_column(default=utcnow)

    conversation: Mapped["Conversation"] = relationship(back_populates="messages")
    tool_calls: Mapped[list["ToolCall"]] = relationship(back_populates="message")


class Task(Base):
    __tablename__ = "tasks"
    __table_args__ = (
        CheckConstraint("status in ('todo','in_progress','done')", name="ck_task_status"),
    )

    id: Mapped[int] = mapped_column(primary_key=True)
    user_id: Mapped[int] = mapped_column(ForeignKey("users.id"))
    title: Mapped[str] = mapped_column(String)
    description: Mapped[str] = mapped_column(Text, default="")
    status: Mapped[str] = mapped_column(String, default="todo")
    due_date: Mapped[datetime.datetime | None] = mapped_column(nullable=True)
    created_at: Mapped[datetime.datetime] = mapped_column(default=utcnow)
    updated_at: Mapped[datetime.datetime] = mapped_column(default=utcnow, onupdate=utcnow)

    user: Mapped["User"] = relationship(back_populates="tasks")


class FileRecord(Base):
    __tablename__ = "files"

    id: Mapped[int] = mapped_column(primary_key=True)
    user_id: Mapped[int] = mapped_column(ForeignKey("users.id"))
    filename: Mapped[str] = mapped_column(String)
    filepath: Mapped[str] = mapped_column(String)
    mime_type: Mapped[str] = mapped_column(String, default="")
    extracted_text: Mapped[str] = mapped_column(Text, default="")
    uploaded_at: Mapped[datetime.datetime] = mapped_column(default=utcnow)

    user: Mapped["User"] = relationship(back_populates="files")


class ToolCall(Base):
    __tablename__ = "tool_calls"
    __table_args__ = (
        CheckConstraint(
            "status in ('pending','approved','denied','executed','failed')",
            name="ck_tool_call_status",
        ),
    )

    id: Mapped[int] = mapped_column(primary_key=True)
    conversation_id: Mapped[int] = mapped_column(ForeignKey("conversations.id"))
    message_id: Mapped[int | None] = mapped_column(ForeignKey("messages.id"), nullable=True)
    tool_use_id: Mapped[str] = mapped_column(String, default="")
    tool_name: Mapped[str] = mapped_column(String)
    arguments_json: Mapped[str] = mapped_column(Text, default="{}")
    requires_approval: Mapped[bool] = mapped_column(default=False)
    status: Mapped[str] = mapped_column(String, default="pending")
    result_json: Mapped[str | None] = mapped_column(Text, nullable=True)
    created_at: Mapped[datetime.datetime] = mapped_column(default=utcnow)
    resolved_at: Mapped[datetime.datetime | None] = mapped_column(nullable=True)

    conversation: Mapped["Conversation"] = relationship()
    message: Mapped["Message | None"] = relationship(back_populates="tool_calls")
