# Personal AI Command Center — Architecture & 7-Day Plan

## Context

This is a greenfield portfolio project. The goal is explicitly educational: the
target audience is a beginner/intermediate CS student who wants to understand *why*
each piece exists, not just receive working code. This is the design document
requested before any implementation starts. No code will be written until this
plan is approved.

The project lives in its own top-level folder, `ai-command-center/`, kept
separate from any unrelated folders elsewhere in the repo.

---

## 1. Simple Architecture

```
                          ┌─────────────────────┐
                          │   React Frontend     │
                          │  (chat, tasks, files,│
                          │   approval prompts)  │
                          └──────────┬───────────┘
                                     │ REST + SSE/WebSocket
                                     ▼
                          ┌─────────────────────┐
                          │   FastAPI Backend    │
                          │  /chat /tasks /files │
                          │  /approvals          │
                          └──────────┬───────────┘
                                     │
                    ┌────────────────┼─────────────────┐
                    ▼                ▼                 ▼
           ┌────────────────┐ ┌─────────────┐  ┌───────────────┐
           │  Agent Loop     │ │  Tool Layer │  │  SQLite DB     │
           │ (calls Claude,  │ │ web_search  │  │ conversations, │
           │  interprets     │ │ read_file   │  │ messages,      │
           │  tool_use, loops│◄┤ create_task │─►│ tasks, files,  │
           │  until done)    │ │ list_tasks  │  │ tool_calls     │
           └────────┬────────┘ │ delete_task*│  └───────────────┘
                    │           └─────────────┘
                    ▼
            ┌───────────────┐
            │ Claude API     │
            │ (Messages API, │
            │  tool use)     │
            └───────────────┘

  * tools marked destructive pause the loop and create a pending
    tool_call row; the loop only resumes after the user approves it
    via the frontend.
```

Data flow for one turn:
1. User sends a message → backend saves it as a `message` row → appends to
   conversation history pulled from DB.
2. Backend calls Claude with the full message history + tool definitions.
3. If Claude responds with plain text → save + stream back to frontend. Done.
4. If Claude responds with a `tool_use` block:
   - If the tool is "safe" (web_search, read_file, list_tasks) → execute
     immediately, save a `tool_calls` row with the result, feed the result
     back to Claude, loop to step 2.
   - If the tool is "destructive" (delete_task, delete_file, etc.) → save a
     `tool_calls` row with `status=pending`, stop the loop, and tell the
     frontend "waiting for approval." The frontend shows an approve/deny
     card. Only on approval does the backend execute the tool, save the
     result, and resume the loop.

This is a single-process, single-user app — no message queue, no
microservices. That's the right scope for a 7-day solo project; the
architecture is still "real" (same shape as production agent systems) but
without infrastructure overhead that would eat your week.

---

## 2. Recommended Tech Stack

| Layer | Choice | Why |
|---|---|---|
| Frontend | React + Vite + Tailwind CSS | Fast to set up, no SSR complexity you don't need for a single-user dashboard. Tailwind keeps styling fast so you spend time on logic, not CSS. |
| Backend | Python + FastAPI | Async-native, typed, minimal boilerplate, and the Anthropic Python SDK is first-class here. FastAPI's automatic OpenAPI docs are genuinely useful while building. |
| LLM | Anthropic Claude API (Messages API with tool use) | This *is* the "agent" — you write the loop yourself so you actually learn how tool-use loops work, rather than hiding it behind a framework. |
| Database | SQLite via SQLAlchemy ORM | Zero setup (one file), but SQLAlchemy means the schema/queries port to Postgres later with almost no code change if you ever want to deploy multi-user. |
| Web search tool | Tavily API (or Brave Search API) | Both have a free tier and a simple single-endpoint REST API — ideal for a first "tool." |
| File parsing | `pdfplumber` (PDF), plain read (txt/md) | Small, dependency-light, good enough for a command-center use case. |
| Realtime updates | Server-Sent Events (SSE) for streaming chat | Simpler than WebSockets for one-directional streaming (server → client), which is all a chat response needs. |
| Auth | None / single hardcoded user for now | Real auth (OAuth, sessions) is a whole separate project; skip it so the week goes to agent architecture, not login forms. Schema still models a `users` table so the concept is visible and portable later. |
| Deployment (stretch, Day 7) | Frontend → Vercel, Backend → Render/Fly.io | Optional polish step; not required to demonstrate the architecture. |

You'll touch two languages (Python backend, JS/TS frontend) — normal for
full-stack work and good portfolio signal.

---

## 3. Component Explanations

- **Frontend (React)**: Three views — a Chat panel (message list + input +
  streaming responses), a Task dashboard (list/board of tasks pulled from
  the DB), and an Approval panel/modal (shows pending destructive tool
  calls with Approve/Deny buttons). It's a thin client: all state truth
  lives in the backend DB, the frontend just renders it and issues
  actions.

- **Backend API (FastAPI)**: Stateless HTTP layer exposing:
  - `POST /chat` — takes a user message + conversation id, runs the agent
    loop, streams the response.
  - `GET /conversations/:id/messages` — reload history on page refresh.
  - `GET/POST/PATCH /tasks` — CRUD for the task list, used both by the UI
    directly and indirectly by the agent's task tools.
  - `POST /files` — upload + text extraction.
  - `GET /approvals`, `POST /approvals/:id/decision` — list pending tool
    calls and let the user approve/deny.

- **Agent loop**: The core learning piece. A plain Python function that:
  1. builds the message list (system prompt + history),
  2. calls `client.messages.create(..., tools=[...])`,
  3. inspects the response for `tool_use` blocks,
  4. executes or defers each one,
  5. appends `tool_result` blocks and calls the model again,
  6. repeats until the model returns a plain text (non-tool_use) response.
  Writing this by hand (rather than using a hidden agent framework) is
  what makes the "agentic AI" learning goal real.

- **Tool layer**: Each tool is a small Python function + a JSON schema
  describing its inputs (what Claude sees). Tools are tagged
  `requires_approval: bool` — that flag is the single hook that drives the
  whole human-in-the-loop safety story. Start list:
  `web_search`, `read_file`, `list_tasks` (safe/auto-run);
  `create_task`, `update_task` (safe — creating/editing isn't destructive);
  `delete_task`, `delete_file` (destructive — require approval).

- **Conversation state**: Persisted in the DB, not in server memory —
  meaning a page refresh or server restart doesn't lose history. This is
  the difference between "a script that calls an LLM" and "an app with
  memory."

- **Persistent data store (SQLite)**: Single source of truth for
  conversations, messages, tasks, uploaded files, and tool-call/approval
  records. Using an ORM (SQLAlchemy) means you write Python objects, not
  raw SQL, while still learning real relational schema design.

- **Approval workflow**: The mechanism that turns "AI does things
  automatically" into "AI proposes, human disposes" for anything
  irreversible. It's implemented as a DB status field + a pause in the
  agent loop, not a separate service — deliberately simple, but it
  demonstrates the concept that matters (and is the most "portfolio
  impressive" piece since most beginner projects skip it entirely).

---

## 4. Database Schema

```sql
users (
  id            INTEGER PRIMARY KEY,
  email         TEXT UNIQUE,
  created_at    TIMESTAMP
)

conversations (
  id            INTEGER PRIMARY KEY,
  user_id       INTEGER REFERENCES users(id),
  title         TEXT,
  created_at    TIMESTAMP,
  updated_at    TIMESTAMP
)

messages (
  id              INTEGER PRIMARY KEY,
  conversation_id INTEGER REFERENCES conversations(id),
  role            TEXT CHECK(role IN ('user','assistant','tool')),
  content         TEXT,        -- plain text content
  raw_json        TEXT,        -- full Claude API block (tool_use etc.) for replay/debug
  created_at      TIMESTAMP
)

tasks (
  id            INTEGER PRIMARY KEY,
  user_id       INTEGER REFERENCES users(id),
  title         TEXT,
  description   TEXT,
  status        TEXT CHECK(status IN ('todo','in_progress','done')),
  due_date      TIMESTAMP NULL,
  created_at    TIMESTAMP,
  updated_at    TIMESTAMP
)

files (
  id              INTEGER PRIMARY KEY,
  user_id         INTEGER REFERENCES users(id),
  filename        TEXT,
  filepath        TEXT,          -- path on disk
  mime_type       TEXT,
  extracted_text  TEXT,          -- parsed content the agent can read
  uploaded_at     TIMESTAMP
)

tool_calls (
  id                INTEGER PRIMARY KEY,
  conversation_id   INTEGER REFERENCES conversations(id),
  message_id        INTEGER REFERENCES messages(id),
  tool_name         TEXT,
  arguments_json    TEXT,
  requires_approval BOOLEAN,
  status            TEXT CHECK(status IN ('pending','approved','denied','executed','failed')),
  result_json       TEXT NULL,
  created_at        TIMESTAMP,
  resolved_at       TIMESTAMP NULL
)
```

Relationships: `users 1—N conversations/tasks/files`,
`conversations 1—N messages`, `messages 1—N tool_calls`.

This is small enough to hold in your head, but it already has every
relationship pattern (one-to-many, status enums, soft state machine on
`tool_calls.status`) that a bigger app would need — good for explaining in
an interview.

---

## 5. 7-Day Implementation Plan

- **Day 1 — Skeleton & first LLM call**: FastAPI backend + React frontend
  scaffolded, one `/chat` endpoint that calls Claude directly (no tools, no
  persistence — just proves the wiring: browser → API → Claude → browser).
- **Day 2 — Persistence (MVP checkpoint, see §6)**: Add SQLite +
  SQLAlchemy models for `conversations`/`messages`. Chat history now
  survives a refresh. This is the smallest "real app" milestone.
- **Day 3 — First tool + agent loop**: Implement the actual tool-use loop
  (steps in §3) with one tool: `web_search` (Tavily/Brave). This is where
  "agentic AI" starts — Claude decides *when* to search, not you.
- **Day 4 — File upload + read tool**: Upload endpoint, PDF/text parsing
  into `files.extracted_text`, and a `read_file` tool so the agent can pull
  in uploaded documents during conversation.
- **Day 5 — Task management tools + dashboard UI**: `create_task`,
  `list_tasks`, `update_task` tools wired to the `tasks` table, plus a
  Task dashboard view in the frontend that reflects the same DB state the
  agent is modifying (nice demo moment: "I told the agent to add a task
  in chat, and it shows up on the board").
- **Day 6 — Approval workflow**: Mark `delete_task` (and optionally
  `delete_file`) as `requires_approval`. Build the pending-approval flow
  end to end: agent loop pauses → DB row created → frontend shows an
  approve/deny card → decision resumes the loop. This is the most
  differentiated feature for a portfolio piece.
- **Day 7 — Polish & packaging**: error states, loading indicators, basic
  styling pass, README with architecture diagram and setup instructions,
  short demo recording. Optional: deploy frontend to Vercel + backend to
  Render/Fly.io if time allows.

---

## 6. Smallest MVP for Day 2

Chat UI ⇄ FastAPI ⇄ Claude API, with conversation + messages persisted in
SQLite so a page refresh still shows history. No tools, no tasks, no
files, no approvals yet — those are added incrementally on Days 3–6. This
MVP is deliberately small: it proves the full stack is wired correctly
(frontend, backend, LLM call, database) before any agentic complexity is
layered on top, so if something breaks later you know it's the new piece,
not the foundation.

---

## Next Step

This plan proposes architecture and a schedule only — **no code will be
written until you approve it**. Once approved, Day 1 work would begin in
this `ai-command-center/` folder.
