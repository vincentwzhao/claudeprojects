/** UI-only types for rendering a chat turn as it streams in. */

export interface ToolEvent {
  id: string
  name: string
  args: any
  status: "executed" | "pending" | "approved" | "denied"
  result?: any
  toolCallId?: number
}

export type StreamEvent = { type: "text"; text: string } | ({ type: "tool" } & ToolEvent)

export interface StreamTurn {
  events: StreamEvent[]
}
