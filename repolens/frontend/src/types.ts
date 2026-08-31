import type { Reference, ToolCall } from "./api/client";

export interface ChatMessage {
  role: "user" | "assistant";
  content: string;
  references?: Reference[];
  trace?: ToolCall[];
  pending?: boolean;
  error?: boolean;
}
