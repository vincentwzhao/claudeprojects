# AI Command Center

A personal AI command center: a chat agent (Claude, via the Messages API
with tool use) that can search the web, read files you've uploaded, and
manage a task list — with destructive actions (deleting a task or file)
paused for your explicit approval before they run.

See [`ARCHITECTURE_PLAN.md`](./ARCHITECTURE_PLAN.md) for the full design
rationale, database schema, and the 7-day build plan this followed.

## How it works

- **Backend** (`backend/`): FastAPI + SQLite (via SQLAlchemy). The core
  piece is `app/agent.py` — a hand-written loop that calls Claude, checks
  for `tool_use` blocks, executes safe tools immediately, and for
  destructive tools (`delete_task`, `delete_file`) pauses and writes a
  `pending` row to the `tool_calls` table until you approve or deny it via
  the frontend. Nothing here is hidden behind an agent framework.
- **Frontend** (`frontend/`): React + Vite + Tailwind. A chat panel
  (streamed over SSE), a task dashboard, a file manager, and an approvals
  panel — all reading/writing the same backend state, so telling the
  agent "add a task" in chat updates the same board you see in the
  sidebar.

## Setup

### Backend

```bash
cd backend
python3 -m venv venv
source venv/bin/activate   # Windows: venv\Scripts\activate
pip install -r requirements.txt

cp .env.example .env
# then edit .env and set ANTHROPIC_API_KEY (required) — get one at
# https://console.anthropic.com/
# TAVILY_API_KEY is optional; without it, web_search returns a helpful
# error instead of crashing.

uvicorn app.main:app --reload --port 8000
```

Visit `http://localhost:8000/health` — it should report
`{"status": "ok", "anthropic_key_configured": true}`.

### Frontend

```bash
cd frontend
npm install
npm run dev
```

Visit `http://localhost:5173`. By default it talks to the backend at
`http://localhost:8000` (override with a `VITE_API_BASE_URL` env var if
you run the backend elsewhere).

## Trying it out

- Ask it to do something with your tasks: "add a task to write the report
  by Friday" — watch it show up in the Tasks panel immediately.
- Upload a `.txt`/`.md`/`.pdf` file, then ask "what does the file I
  uploaded say?" — it'll call `read_file`.
- Ask it to delete a task. It'll call `delete_task`, and the loop will
  pause: an approval card appears inline in the chat *and* in the
  Approvals panel. Nothing is deleted until you click Approve.
- Set `TAVILY_API_KEY` and ask something time-sensitive to see
  `web_search` in action.

## Notes on scope

This is a single-process, single-user app by design (see
`ARCHITECTURE_PLAN.md` §1) — no message queue, no auth, no multi-tenant
schema beyond a `users` table that exists so the concept is visible and
the schema is portable if you ever add real accounts. That's the right
scope for demonstrating the architecture without infrastructure overhead
eating the time better spent on the agent loop itself.
