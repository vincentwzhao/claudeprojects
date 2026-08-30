import { useEffect, useState } from "react"
import { api, decideApproval, sendChatMessage, type Conversation, type Message, type Task, type FileRecord, type ToolCallOut } from "./api"
import type { StreamEvent } from "./types"
import Sidebar from "./components/Sidebar"
import ChatPanel from "./components/ChatPanel"
import TaskDashboard from "./components/TaskDashboard"
import FileManager from "./components/FileManager"
import ApprovalsPanel from "./components/ApprovalsPanel"

const APPROVALS_POLL_MS = 4000

export default function App() {
  const [conversations, setConversations] = useState<Conversation[]>([])
  const [activeId, setActiveId] = useState<number | null>(null)
  const [messages, setMessages] = useState<Message[]>([])
  const [streamEvents, setStreamEvents] = useState<StreamEvent[]>([])
  const [isStreaming, setIsStreaming] = useState(false)
  const [chatError, setChatError] = useState<string | null>(null)

  const [tasks, setTasks] = useState<Task[]>([])
  const [files, setFiles] = useState<FileRecord[]>([])
  const [approvals, setApprovals] = useState<ToolCallOut[]>([])

  const refreshTasks = () => api.listTasks().then(setTasks).catch(console.error)
  const refreshFiles = () => api.listFiles().then(setFiles).catch(console.error)
  const refreshApprovals = () => api.listApprovals().then(setApprovals).catch(console.error)
  const refreshConversations = () => api.listConversations().then(setConversations).catch(console.error)

  useEffect(() => {
    refreshConversations()
    refreshTasks()
    refreshFiles()
    refreshApprovals()
    const interval = setInterval(refreshApprovals, APPROVALS_POLL_MS)
    return () => clearInterval(interval)
  }, [])

  useEffect(() => {
    if (activeId == null) {
      setMessages([])
      return
    }
    api.getMessages(activeId).then(setMessages).catch(console.error)
  }, [activeId])

  function finalizeStream() {
    setStreamEvents((events) => {
      if (events.length > 0) {
        const text = events
          .filter((e): e is Extract<StreamEvent, { type: "text" }> => e.type === "text")
          .map((e) => e.text)
          .join("")
        if (text) {
          setMessages((prev) => [
            ...prev,
            { id: Date.now(), role: "assistant", content: text, created_at: new Date().toISOString() },
          ])
        }
      }
      return []
    })
    setIsStreaming(false)
  }

  function handleStreamEvent(event: string, data: any) {
    if (event === "conversation") {
      const newId = data.conversation_id as number
      setActiveId((prev) => prev ?? newId)
      refreshConversations()
      return
    }

    if (event === "token") {
      setStreamEvents((events) => {
        const last = events[events.length - 1]
        if (last && last.type === "text") {
          return [...events.slice(0, -1), { type: "text", text: last.text + data.text }]
        }
        return [...events, { type: "text", text: data.text }]
      })
      setIsStreaming(true)
    } else if (event === "tool_result") {
      setStreamEvents((events) => [
        ...events,
        { type: "tool", id: crypto.randomUUID(), name: data.tool_name, args: data.arguments, status: "executed", result: data.result },
      ])
      if (["create_task", "update_task", "delete_task"].includes(data.tool_name)) refreshTasks()
      if (data.tool_name === "delete_file") refreshFiles()
    } else if (event === "approval_needed") {
      setStreamEvents((events) => [
        ...events,
        { type: "tool", id: `approval-${data.tool_call_id}`, name: data.tool_name, args: data.arguments, status: "pending", toolCallId: data.tool_call_id },
      ])
      setIsStreaming(false)
      refreshApprovals()
    } else if (event === "done") {
      finalizeStream()
      refreshTasks()
      refreshFiles()
    } else if (event === "error") {
      setChatError(data.message)
      finalizeStream()
    } else if (event === "waiting_on_other_approvals") {
      // another destructive tool from the same turn is still pending
    }
  }

  async function handleSend(text: string) {
    setChatError(null)
    setMessages((prev) => [...prev, { id: Date.now(), role: "user", content: text, created_at: new Date().toISOString() }])
    setIsStreaming(true)
    try {
      await sendChatMessage({ conversation_id: activeId, message: text }, handleStreamEvent)
    } catch (err) {
      setChatError(err instanceof Error ? err.message : "Something went wrong")
      setIsStreaming(false)
    }
  }

  function handleApprovalDecision(toolCallId: number, approve: boolean) {
    // reflect the decision immediately in the inline chat chip, if present
    setStreamEvents((events) =>
      events.map((e) => (e.type === "tool" && e.toolCallId === toolCallId ? { ...e, status: approve ? "approved" : "denied" } : e)),
    )
    refreshApprovals()
    decideApproval(toolCallId, approve, handleStreamEvent).catch((err) =>
      setChatError(err instanceof Error ? err.message : "Approval failed"),
    )
  }

  return (
    <div className="grid h-screen grid-cols-[220px_1fr_320px] bg-slate-950 text-slate-100">
      <Sidebar
        conversations={conversations}
        activeId={activeId}
        onSelect={setActiveId}
        onNew={() => {
          setActiveId(null)
          setMessages([])
          setStreamEvents([])
        }}
      />

      <ChatPanel
        messages={messages}
        streamEvents={streamEvents}
        isStreaming={isStreaming}
        error={chatError}
        onSend={handleSend}
        onApprovalDecision={handleApprovalDecision}
      />

      <div className="flex flex-col gap-6 overflow-y-auto border-l border-slate-800 bg-slate-950 p-4">
        <section>
          <h2 className="mb-2 text-xs font-semibold uppercase tracking-wide text-slate-500">
            Approvals {approvals.length > 0 && <span className="text-amber-400">({approvals.length})</span>}
          </h2>
          <ApprovalsPanel approvals={approvals} onDecision={handleApprovalDecision} />
        </section>

        <section>
          <h2 className="mb-2 text-xs font-semibold uppercase tracking-wide text-slate-500">Tasks</h2>
          <TaskDashboard
            tasks={tasks}
            onCreate={(title) => api.createTask({ title }).then(refreshTasks)}
            onUpdateStatus={(id, status) => api.updateTask(id, { status }).then(refreshTasks)}
            onDelete={(id) => api.deleteTask(id).then(refreshTasks)}
          />
        </section>

        <section>
          <h2 className="mb-2 text-xs font-semibold uppercase tracking-wide text-slate-500">Files</h2>
          <FileManager files={files} onUpload={(file) => api.uploadFile(file).then(() => refreshFiles())} onDelete={(id) => api.deleteFile(id).then(refreshFiles)} />
        </section>
      </div>
    </div>
  )
}
