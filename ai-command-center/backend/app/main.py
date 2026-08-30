from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from . import models
from .config import settings
from .database import engine
from .routers import approvals, chat, conversations, files, tasks

models.Base.metadata.create_all(bind=engine)

app = FastAPI(title="AI Command Center")

app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors_origin_list,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(chat.router)
app.include_router(conversations.router)
app.include_router(tasks.router)
app.include_router(files.router)
app.include_router(approvals.router)


@app.get("/health")
def health():
    return {"status": "ok", "anthropic_key_configured": bool(settings.anthropic_api_key)}
