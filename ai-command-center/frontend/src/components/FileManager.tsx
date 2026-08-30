import { useRef, useState, type ChangeEvent } from "react"
import type { FileRecord } from "../api"

export default function FileManager({
  files,
  onUpload,
  onDelete,
}: {
  files: FileRecord[]
  onUpload: (file: File) => Promise<void>
  onDelete: (id: number) => void
}) {
  const inputRef = useRef<HTMLInputElement>(null)
  const [uploading, setUploading] = useState(false)
  const [error, setError] = useState<string | null>(null)

  const handleChange = async (e: ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (!file) return
    setUploading(true)
    setError(null)
    try {
      await onUpload(file)
    } catch (err) {
      setError(err instanceof Error ? err.message : "Upload failed")
    } finally {
      setUploading(false)
      if (inputRef.current) inputRef.current.value = ""
    }
  }

  return (
    <div className="flex flex-col gap-3">
      <div>
        <input ref={inputRef} type="file" onChange={handleChange} disabled={uploading} className="hidden" id="file-upload" />
        <label
          htmlFor="file-upload"
          className="block cursor-pointer rounded-lg border border-dashed border-slate-700 px-3 py-2 text-center text-sm text-slate-400 hover:border-indigo-500 hover:text-slate-200"
        >
          {uploading ? "Uploading…" : "Upload a file (.txt, .md, .pdf)"}
        </label>
        {error && <p className="mt-1 text-xs text-rose-400">{error}</p>}
      </div>

      {files.length === 0 && <p className="text-xs text-slate-500">No files yet.</p>}

      <ul className="flex flex-col gap-2">
        {files.map((f) => (
          <li
            key={f.id}
            className="flex items-center justify-between gap-2 rounded-lg border border-slate-800 bg-slate-900 px-3 py-2"
          >
            <div className="min-w-0">
              <p className="truncate text-sm text-slate-100">
                {f.filename} <span className="ml-1 text-xs text-slate-500">(id {f.id})</span>
              </p>
              <p className="text-xs text-slate-500">{f.extracted_chars.toLocaleString()} chars extracted</p>
            </div>
            <button
              onClick={() => onDelete(f.id)}
              className="shrink-0 rounded px-1.5 py-1 text-xs text-slate-500 hover:bg-rose-950 hover:text-rose-300"
              title="Delete file"
            >
              ✕
            </button>
          </li>
        ))}
      </ul>
    </div>
  )
}
