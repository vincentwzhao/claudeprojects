import { useEffect, useRef, useState, type FormEvent } from "react"
import type { Message } from "../api"
import type { StreamEvent } from "../types"

function ToolChip({ event, onDecision }: { event: Extract<StreamEvent, { type: "tool" }>; onDecision: (approve: boolean) => void }) {
  const statusStyles: Record<string, string> = {
    executed: "border-emerald-700 bg-emerald-950 text-emerald-300",
    pending: "border-amber-600 bg-amber-950 text-amber-300",
    approved: "border-emerald-700 bg-emerald-950 text-emerald-300",
    denied: "border-rose-700 bg-rose-950 text-rose-300",
  }
  return (
    <div className={`my-2 rounded-lg border px-3 py-2 text-sm ${statusStyles[event.status]}`}>
      <div className="flex items-center justify-between gap-3">
        <span>
          <span className="font-mono text-xs opacity-80">{event.name}</span>
          <span className="ml-2 opacity-70">{JSON.stringify(event.args)}</span>
        </span>
        {event.status === "pending" && (
          <div className="flex shrink-0 gap-2">
            <button
              onClick={() => onDecision(true)}
              className="rounded bg-emerald-700 px-2 py-1 text-xs font-medium text-white hover:bg-emerald-600"
            >
              Approve
            </button>
            <button
              onClick={() => onDecision(false)}
              className="rounded bg-rose-700 px-2 py-1 text-xs font-medium text-white hover:bg-rose-600"
            >
              Deny
            </button>
          </div>
        )}
      </div>
      {event.status === "pending" && (
        <p className="mt-1 text-xs opacity-70">This is a destructive action — waiting for your approval.</p>
      )}
      {event.result !== undefined && (
        <pre className="mt-1 max-h-24 overflow-auto text-xs opacity-70">{JSON.stringify(event.result, null, 2)}</pre>
      )}
    </div>
  )
}

function StreamEvents({ events, onDecision }: { events: StreamEvent[]; onDecision: (toolCallId: number, approve: boolean) => void }) {
  return (
    <>
      {events.map((ev, i) =>
        ev.type === "text" ? (
          <span key={i} className="whitespace-pre-wrap">
            {ev.text}
          </span>
        ) : (
          <ToolChip key={ev.id} event={ev} onDecision={(approve) => ev.toolCallId && onDecision(ev.toolCallId, approve)} />
        ),
      )}
    </>
  )
}

export default function ChatPanel({
  messages,
  streamEvents,
  isStreaming,
  error,
  onSend,
  onApprovalDecision,
}: {
  messages: Message[]
  streamEvents: StreamEvent[]
  isStreaming: boolean
  error: string | null
  onSend: (text: string) => void
  onApprovalDecision: (toolCallId: number, approve: boolean) => void
}) {
  const [input, setInput] = useState("")
  const bottomRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" })
  }, [messages, streamEvents])

  const submit = (e: FormEvent) => {
    e.preventDefault()
    const text = input.trim()
    if (!text || isStreaming) return
    onSend(text)
    setInput("")
  }

  const hasStreamContent = streamEvents.length > 0

  return (
    <div className="flex h-full flex-col">
      <div className="flex-1 overflow-y-auto px-6 py-4">
        {messages.length === 0 && !hasStreamContent && (
          <p className="mt-10 text-center text-sm text-slate-500">
            Ask it to search the web, read an uploaded file, or manage your tasks.
          </p>
        )}
        <div className="mx-auto flex max-w-3xl flex-col gap-4">
          {messages.map((m) => (
            <div key={m.id} className={`flex ${m.role === "user" ? "justify-end" : "justify-start"}`}>
              <div
                className={`max-w-[80%] rounded-2xl px-4 py-2 text-sm ${
                  m.role === "user" ? "bg-indigo-600 text-white" : "bg-slate-800 text-slate-100"
                }`}
              >
                <span className="whitespace-pre-wrap">{m.content}</span>
              </div>
            </div>
          ))}

          {hasStreamContent && (
            <div className="flex justify-start">
              <div className="max-w-[80%] rounded-2xl bg-slate-800 px-4 py-2 text-sm text-slate-100">
                <StreamEvents events={streamEvents} onDecision={onApprovalDecision} />
                {isStreaming && <span className="ml-0.5 animate-pulse">▋</span>}
              </div>
            </div>
          )}

          {error && (
            <div className="rounded-lg border border-rose-700 bg-rose-950 px-4 py-2 text-sm text-rose-300">
              {error}
            </div>
          )}
          <div ref={bottomRef} />
        </div>
      </div>

      <form onSubmit={submit} className="border-t border-slate-800 p-4">
        <div className="mx-auto flex max-w-3xl gap-2">
          <input
            value={input}
            onChange={(e) => setInput(e.target.value)}
            placeholder={isStreaming ? "Waiting for a response…" : "Message the agent…"}
            disabled={isStreaming}
            className="flex-1 rounded-xl border border-slate-700 bg-slate-900 px-4 py-2 text-sm text-slate-100 placeholder-slate-500 outline-none focus:border-indigo-500 disabled:opacity-50"
          />
          <button
            type="submit"
            disabled={isStreaming || !input.trim()}
            className="rounded-xl bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-500 disabled:opacity-40"
          >
            Send
          </button>
        </div>
      </form>
    </div>
  )
}
