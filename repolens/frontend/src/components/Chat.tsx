import { useEffect, useRef, useState } from "react";
import type { FormEvent } from "react";
import type { ChatMessage } from "../types";
import { MessageBubble } from "./MessageBubble";

interface Props {
  messages: ChatMessage[];
  onSend: (text: string) => void;
  busy: boolean;
  inputSeed: string;
  onSeedConsumed: () => void;
}

export function Chat({ messages, onSend, busy, inputSeed, onSeedConsumed }: Props) {
  const [text, setText] = useState("");
  const bottomRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  useEffect(() => {
    if (inputSeed) {
      setText(inputSeed);
      inputRef.current?.focus();
      onSeedConsumed();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [inputSeed]);

  function handleSubmit(e: FormEvent) {
    e.preventDefault();
    if (!text.trim() || busy) return;
    onSend(text.trim());
    setText("");
  }

  return (
    <div className="chat">
      <div className="chat-scroll">
        {messages.length === 0 && (
          <div className="chat-empty">
            <p>Ask anything about this codebase.</p>
            <p className="hint">
              Try: "How does authentication work?" · "Where is user registration implemented?" ·{" "}
              <code>/explain authentication</code> · <code>/trace user registration</code>
            </p>
          </div>
        )}
        {messages.map((m, i) => (
          <MessageBubble key={i} message={m} />
        ))}
        <div ref={bottomRef} />
      </div>

      <form className="chat-input" onSubmit={handleSubmit}>
        <input
          ref={inputRef}
          type="text"
          placeholder="Ask a question, or use /explain <concept>, /trace <feature>, /project"
          value={text}
          onChange={(e) => setText(e.target.value)}
          disabled={busy}
        />
        <button type="submit" disabled={busy || !text.trim()}>
          {busy ? "…" : "Send"}
        </button>
      </form>
    </div>
  );
}
