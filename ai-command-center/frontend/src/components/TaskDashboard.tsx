import { useState, type FormEvent } from "react"
import type { Task } from "../api"

const STATUSES: Task["status"][] = ["todo", "in_progress", "done"]
const STATUS_LABEL: Record<Task["status"], string> = {
  todo: "To do",
  in_progress: "In progress",
  done: "Done",
}

export default function TaskDashboard({
  tasks,
  onCreate,
  onUpdateStatus,
  onDelete,
}: {
  tasks: Task[]
  onCreate: (title: string) => void
  onUpdateStatus: (id: number, status: Task["status"]) => void
  onDelete: (id: number) => void
}) {
  const [title, setTitle] = useState("")

  const submit = (e: FormEvent) => {
    e.preventDefault()
    const t = title.trim()
    if (!t) return
    onCreate(t)
    setTitle("")
  }

  return (
    <div className="flex flex-col gap-3">
      <form onSubmit={submit} className="flex gap-2">
        <input
          value={title}
          onChange={(e) => setTitle(e.target.value)}
          placeholder="Add a task…"
          className="flex-1 rounded-lg border border-slate-700 bg-slate-900 px-3 py-1.5 text-sm text-slate-100 placeholder-slate-500 outline-none focus:border-indigo-500"
        />
        <button
          type="submit"
          className="rounded-lg bg-slate-700 px-3 py-1.5 text-sm text-white hover:bg-slate-600"
        >
          Add
        </button>
      </form>

      {tasks.length === 0 && <p className="text-xs text-slate-500">No tasks yet.</p>}

      <ul className="flex flex-col gap-2">
        {tasks.map((t) => (
          <li
            key={t.id}
            className="flex items-center justify-between gap-2 rounded-lg border border-slate-800 bg-slate-900 px-3 py-2"
          >
            <div className="min-w-0">
              <p className={`truncate text-sm ${t.status === "done" ? "text-slate-500 line-through" : "text-slate-100"}`}>
                {t.title}
              </p>
              {t.description && <p className="truncate text-xs text-slate-500">{t.description}</p>}
            </div>
            <div className="flex shrink-0 items-center gap-2">
              <select
                value={t.status}
                onChange={(e) => onUpdateStatus(t.id, e.target.value as Task["status"])}
                className="rounded border border-slate-700 bg-slate-800 px-1.5 py-1 text-xs text-slate-200"
              >
                {STATUSES.map((s) => (
                  <option key={s} value={s}>
                    {STATUS_LABEL[s]}
                  </option>
                ))}
              </select>
              <button
                onClick={() => onDelete(t.id)}
                className="rounded px-1.5 py-1 text-xs text-slate-500 hover:bg-rose-950 hover:text-rose-300"
                title="Delete task"
              >
                ✕
              </button>
            </div>
          </li>
        ))}
      </ul>
    </div>
  )
}
