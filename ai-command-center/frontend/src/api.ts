export const API_BASE = import.meta.env.VITE_API_BASE_URL || "http://localhost:8000"

export interface Conversation {
  id: number
  title: string
  created_at: string
  updated_at: string
}

export interface Message {
  id: number
  role: "user" | "assistant" | "tool"
  content: string
  created_at: string
}

export interface Task {
  id: number
  title: string
  description: string
  status: "todo" | "in_progress" | "done"
  due_date: string | null
  created_at: string
  updated_at: string
}

export interface FileRecord {
  id: number
  filename: string
  mime_type: string
  uploaded_at: string
  extracted_chars: number
}

export interface ToolCallOut {
  id: number
  conversation_id: number
  tool_name: string
  arguments_json: string
  requires_approval: boolean
  status: string
  result_json: string | null
  created_at: string
  resolved_at: string | null
}

async function json<T>(res: Response): Promise<T> {
  if (!res.ok) {
    const text = await res.text().catch(() => res.statusText)
    throw new Error(text || `${res.status} ${res.statusText}`)
  }
  return res.json() as Promise<T>
}

export const api = {
  listConversations: () => fetch(`${API_BASE}/conversations`).then((r) => json<Conversation[]>(r)),
  getMessages: (conversationId: number) =>
    fetch(`${API_BASE}/conversations/${conversationId}/messages`).then((r) => json<Message[]>(r)),

  listTasks: () => fetch(`${API_BASE}/tasks`).then((r) => json<Task[]>(r)),
  createTask: (body: { title: string; description?: string; status?: string }) =>
    fetch(`${API_BASE}/tasks`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    }).then((r) => json<Task>(r)),
  updateTask: (id: number, body: Partial<Pick<Task, "title" | "description" | "status">>) =>
    fetch(`${API_BASE}/tasks/${id}`, {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    }).then((r) => json<Task>(r)),
  deleteTask: (id: number) => fetch(`${API_BASE}/tasks/${id}`, { method: "DELETE" }).then((r) => json(r)),

  listFiles: () => fetch(`${API_BASE}/files`).then((r) => json<FileRecord[]>(r)),
  uploadFile: (file: File) => {
    const form = new FormData()
    form.append("file", file)
    return fetch(`${API_BASE}/files`, { method: "POST", body: form }).then((r) => json<FileRecord>(r))
  },
  deleteFile: (id: number) => fetch(`${API_BASE}/files/${id}`, { method: "DELETE" }).then((r) => json(r)),

  listApprovals: () => fetch(`${API_BASE}/approvals`).then((r) => json<ToolCallOut[]>(r)),
}

export type SSEHandler = (event: string, data: any) => void

/** Reads a text/event-stream response body and dispatches parsed events. */
async function pumpSSE(res: Response, onEvent: SSEHandler) {
  if (!res.ok || !res.body) {
    const text = await res.text().catch(() => res.statusText)
    onEvent("error", { message: text || `${res.status} ${res.statusText}` })
    return
  }
  const reader = res.body.getReader()
  const decoder = new TextDecoder()
  let buffer = ""

  while (true) {
    const { done, value } = await reader.read()
    if (done) break
    buffer += decoder.decode(value, { stream: true })

    let sepIndex
    while ((sepIndex = buffer.indexOf("\n\n")) !== -1) {
      const rawEvent = buffer.slice(0, sepIndex)
      buffer = buffer.slice(sepIndex + 2)

      let eventName = "message"
      let dataLine = ""
      for (const line of rawEvent.split("\n")) {
        if (line.startsWith("event:")) eventName = line.slice(6).trim()
        else if (line.startsWith("data:")) dataLine += line.slice(5).trim()
      }
      if (dataLine) {
        try {
          onEvent(eventName, JSON.parse(dataLine))
        } catch {
          onEvent(eventName, dataLine)
        }
      }
    }
  }
}

export function sendChatMessage(
  body: { conversation_id: number | null; message: string },
  onEvent: SSEHandler,
) {
  return fetch(`${API_BASE}/chat`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  }).then((res) => pumpSSE(res, onEvent))
}

export function decideApproval(toolCallId: number, approve: boolean, onEvent: SSEHandler) {
  return fetch(`${API_BASE}/approvals/${toolCallId}/decision`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ approve }),
  }).then(async (res) => {
    const contentType = res.headers.get("content-type") || ""
    if (contentType.includes("text/event-stream")) {
      await pumpSSE(res, onEvent)
    } else {
      const data = await json<{ status: string }>(res)
      onEvent("waiting_on_other_approvals", data)
    }
  })
}
