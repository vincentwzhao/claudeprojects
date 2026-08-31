import { useState } from "react";
import type { ChatMessage } from "../types";

function renderAnswer(text: string) {
  // Minimal markdown: headings, bullet lines, and bare paragraphs.
  const lines = text.split("\n");
  return lines.map((line, i) => {
    if (line.startsWith("## ")) return <h3 key={i}>{line.slice(3)}</h3>;
    if (line.startsWith("- ") || line.startsWith("* ")) return <li key={i}>{line.slice(2)}</li>;
    if (/^\d+\.\s/.test(line)) return <li key={i}>{line.replace(/^\d+\.\s/, "")}</li>;
    if (!line.trim()) return <div key={i} className="line-gap" />;
    return <p key={i}>{line}</p>;
  });
}

export function MessageBubble({ message }: { message: ChatMessage }) {
  const [showTrace, setShowTrace] = useState(false);

  if (message.role === "user") {
    return (
      <div className="msg msg-user">
        <div className="msg-bubble">{message.content}</div>
      </div>
    );
  }

  if (message.pending) {
    return (
      <div className="msg msg-assistant">
        <div className="msg-bubble pending">
          <span className="spinner" /> RepoLens is inspecting the repository…
        </div>
      </div>
    );
  }

  if (message.error) {
    return (
      <div className="msg msg-assistant">
        <div className="msg-bubble error">{message.content}</div>
      </div>
    );
  }

  return (
    <div className="msg msg-assistant">
      <div className="msg-bubble">
        <div className="answer">{renderAnswer(message.content)}</div>

        {message.references && message.references.length > 0 && (
          <div className="reference-list">
            {message.references.map((ref, i) => (
              <span className="reference-chip" key={i}>
                📄 {ref.file}
                {ref.line ? `:${ref.line}` : ""}
              </span>
            ))}
          </div>
        )}

        {message.trace && message.trace.length > 0 && (
          <div className="trace-box">
            <button className="trace-toggle" onClick={() => setShowTrace((s) => !s)}>
              {showTrace ? "▾" : "▸"} Agent used {message.trace.length} tool call{message.trace.length === 1 ? "" : "s"}
            </button>
            {showTrace && (
              <ul className="trace-list">
                {message.trace.map((t, i) => (
                  <li key={i}>
                    <code>{t.tool}({JSON.stringify(t.input)})</code>
                    <span className="trace-result"> → {t.output_summary}</span>
                  </li>
                ))}
              </ul>
            )}
          </div>
        )}
      </div>
    </div>
  );
}
