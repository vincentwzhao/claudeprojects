import os
import uuid

from fastapi import APIRouter, Depends, HTTPException, UploadFile
from sqlalchemy.orm import Session

from .. import models, schemas
from ..config import settings
from ..database import get_db
from ..deps import get_or_create_default_user

router = APIRouter(prefix="/files", tags=["files"])

TEXT_MIME_PREFIXES = ("text/",)
MAX_UPLOAD_BYTES = 20 * 1024 * 1024


def _extract_text(path: str, mime_type: str) -> str:
    if mime_type == "application/pdf":
        import pdfplumber

        with pdfplumber.open(path) as pdf:
            return "\n\n".join(page.extract_text() or "" for page in pdf.pages)
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except OSError:
        return ""


@router.get("", response_model=list[schemas.FileOut])
def list_files(db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    records = (
        db.query(models.FileRecord)
        .filter(models.FileRecord.user_id == user.id)
        .order_by(models.FileRecord.uploaded_at.desc())
        .all()
    )
    return [
        schemas.FileOut.model_validate(
            {**r.__dict__, "extracted_chars": len(r.extracted_text or "")}
        )
        for r in records
    ]


@router.post("", response_model=schemas.FileOut)
async def upload_file(file: UploadFile, db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    os.makedirs(settings.upload_dir, exist_ok=True)

    original_name = os.path.basename(file.filename or "upload")
    stored_name = f"{uuid.uuid4().hex}_{original_name}"
    dest_path = os.path.join(settings.upload_dir, stored_name)

    size = 0
    with open(dest_path, "wb") as out:
        while chunk := await file.read(1024 * 1024):
            size += len(chunk)
            if size > MAX_UPLOAD_BYTES:
                out.close()
                os.remove(dest_path)
                raise HTTPException(413, "File too large (max 20MB)")
            out.write(chunk)

    mime_type = file.content_type or "application/octet-stream"
    extracted_text = _extract_text(dest_path, mime_type)

    record = models.FileRecord(
        user_id=user.id,
        filename=original_name,
        filepath=dest_path,
        mime_type=mime_type,
        extracted_text=extracted_text,
    )
    db.add(record)
    db.commit()
    db.refresh(record)
    return schemas.FileOut.model_validate(
        {**record.__dict__, "extracted_chars": len(extracted_text)}
    )


@router.delete("/{file_id}")
def delete_file(file_id: int, db: Session = Depends(get_db)):
    user = get_or_create_default_user(db)
    record = (
        db.query(models.FileRecord)
        .filter(models.FileRecord.id == file_id, models.FileRecord.user_id == user.id)
        .first()
    )
    if record is None:
        raise HTTPException(404, "File not found")
    if os.path.exists(record.filepath):
        os.remove(record.filepath)
    db.delete(record)
    db.commit()
    return {"deleted_id": file_id}
