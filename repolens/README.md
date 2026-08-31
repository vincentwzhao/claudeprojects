# RepoLens

**RepoLens** is an AI-powered codebase onboarding and exploration agent. Point
it at a GitHub repository (or a local checkout), and it lets you ask
questions about the architecture, code flow, features, and implementation
details — grounded in the actual source code, not guesses.

## Problem

Developers joining an unfamiliar codebase spend a disproportionate amount of
time just figuring out:

- where things live
- how components talk to each other
- how a feature actually flows through the system end-to-end
- where to make a given change safely

Reading through hundreds of files to answer "how does login work?" is slow
and error-prone, and asking a plain LLM (with no access to the code) invites
confident-sounding hallucination.

## Solution

RepoLens gives an LLM agent **tools to inspect the real repository** —
`search_files`, `search_code`, `read_file`, `list_directory`,
`get_git_history` — and a strict grounding rule: every claim must come from
something a tool actually returned, cited as `path/to/file.ext:LINE`. If the
evidence isn't there, the agent says so instead of inventing an answer.

On top of the general Q&A loop, RepoLens implements three focused workflows:
a deterministic static-analysis **`/project`** overview, a guided
**`/explain <concept>`** walkthrough, and a layered **`/trace <feature>`**
that follows a feature from HTTP request down to the database.

## Architecture

```
                    ┌─────────────┐
   Browser  ──────► │  Frontend   │  React + Vite (chat UI, sidebar, panels)
                    └──────┬──────┘
                           │ REST (JSON)
                    ┌──────▼──────┐
                    │  FastAPI    │  routers: repos / chat / explain / trace / project
                    │  Backend    │
                    └──────┬──────┘
                           │
             ┌─────────────┼─────────────────┐
             │             │                 │
     ┌───────▼──────┐┌─────▼──────┐   ┌──────▼───────┐
     │ RepoManager  ││  Analyzer  │   │    Agent     │
     │ clone/load   ││ heuristic  │   │  tool-use    │
     │ repo to disk ││ static     │   │  loop (Claude│
     │              ││ analysis   │   │  Messages API│
     └──────────────┘└────────────┘   └──────┬───────┘
                                              │ chooses & calls
                                       ┌──────▼───────┐
                                       │    Tools     │
                                       │ search_files │
                                       │ search_code  │
                                       │ read_file    │
                                       │ list_directory│
                                       │ get_git_history│
                                       └──────┬───────┘
                                              │ operate on
                                       ┌──────▼───────┐
                                       │  Repository  │
                                       │  on disk     │
                                       └──────────────┘
```

**Agent loop** (`backend/app/agent/core.py`): a hand-written loop against the
Anthropic Messages API's tool-use protocol — no agent framework. Each turn:
the model either asks to call a tool (which is executed against the real
repo on disk and the result fed back) or produces a final answer. The loop
tracks every tool call (for the UI's "agent used N tool calls" trace) and
extracts `file:line` references both from tool calls made and from citations
in the model's own answer.

```
User question
     │
     ▼
Agent (Claude, with tool definitions)
     │
     ▼
Chooses a tool  ──────►  search_files / search_code / read_file / list_directory / get_git_history
     │                          │
     │◄─────────────────────────  tool result (JSON)
     ▼
Reasons over result, decides: call another tool, or answer
     │
     ▼
Final answer, grounded in what the tools returned, with file:line citations
```

## Features

- **Repository analysis** — heuristic, deterministic static analysis
  (`backend/app/analysis/analyzer.py`) that detects languages, frameworks,
  entry points, important directories, API routes, database layer,
  authentication mechanism, external services, configuration, tests, build
  system, and dependencies — without calling an LLM.
- **AI codebase Q&A** — a conversational interface where the agent decides
  which tools to call to answer your question.
- **`/project`** — generates a Repository Overview (Architecture, Entry
  Points, Major Components, Data Flow, Dependencies, Authentication,
  Potential Issues), grounded in the static analysis above.
- **`/explain <concept>`** — searches the repo, reads the relevant files, and
  explains a concept (e.g. "authentication") as a short numbered flow with
  file references.
- **`/trace <feature>`** — follows a feature through the codebase's actual
  layers (route → controller → service → repository → database, or whatever
  the project actually has) and renders an ASCII flow diagram.
- **Source references** — every answer lists the files (and line numbers,
  where known) it's grounded in, shown as reference chips in the UI.
- **Tool-use transparency** — the UI shows exactly which tools the agent
  called and with what arguments, so you can see it's actually inspecting
  the repo rather than making things up.

## Project Structure

```
repolens/
├── backend/            FastAPI app
│   ├── app/
│   │   ├── agent/      tool-use loop + prompts for chat/explain/trace/project
│   │   ├── analysis/   heuristic repository analyzer
│   │   ├── tools/      search_files, search_code, read_file, list_directory, get_git_history
│   │   ├── routers/    /api/repos, /api/repos/{id}/{chat,explain,trace,project}
│   │   ├── config.py, repo_manager.py, paths.py, schemas.py, main.py
│   └── requirements.txt
├── frontend/           React + Vite + TypeScript UI
│   └── src/
│       ├── api/client.ts
│       └── components/ Sidebar, Chat, MessageBubble, AnalysisPanel, FileBrowser, RepoLoader
├── tests/              pytest suite (runs against a fixture repo, offline)
│   └── fixtures/sample_repo/   a tiny Express + JWT auth app
├── pytest.ini
└── README.md
```

## Setup

Requires Python 3.11+, Node 20+, and `git`. An [Anthropic API
key](https://console.anthropic.com/) is required to run the agent (static
analysis and repo browsing work without one).

### 1. Backend

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

cp .env.example .env
# edit .env and set ANTHROPIC_API_KEY=sk-ant-...

uvicorn app.main:app --reload --port 8000
```

### 2. Frontend

```bash
cd frontend
npm install
npm run dev
```

Open http://localhost:5173. The Vite dev server proxies `/api` requests to
the backend on port 8000 (see `frontend/vite.config.ts`).

### 3. Run the tests

From the repo root (`repolens/`):

```bash
cd backend && python3 -m venv .venv && source .venv/bin/activate && pip install -r requirements.txt
cd ..
python -m pytest -q
```

The suite runs entirely offline against a small fixture repo
(`tests/fixtures/sample_repo`) — no API key needed. The Anthropic client is
mocked for agent/API tests so tool-selection logic is verified deterministically.

## Example

```
Repository: https://github.com/gothinkster/node-express-realworld-example-app

✓ Repository indexed
✓ Architecture analyzed
✓ Entry points identified
✓ Dependencies analyzed

Ask anything about this codebase.

> How does login work?

Login is handled in src/app/routes/auth/auth.controller.ts:30, which
registers POST /users/login and delegates to the login() function in
auth.service.ts. That function looks up the user, verifies the password,
and returns a signed JWT.

📄 src/app/routes/auth/auth.controller.ts:30
📄 src/app/routes/auth/auth.service.ts
```

(This example reflects what the tool chain actually returns when pointed at
that real repository — see "Known Limitations" for how it was verified in
this environment.)

## Known Limitations

- **No live LLM verification in this build environment.** This sandbox has
  no `ANTHROPIC_API_KEY` configured, so the end-to-end agent conversation
  loop could not be exercised against the live Anthropic API here. What
  *was* verified in this environment, against real cloned repositories
  (`expressjs/express` and `gothinkster/node-express-realworld-example-app`):
  - the full repo-loading, cloning, and static-analysis pipeline
  - every tool (`search_code`, `read_file`, `search_files`,
    `get_git_history`) called directly against real files, returning
    correct, accurate `file:line` results (e.g. tracing the `/users/login`
    route to its controller and service functions)
  - the agent's tool-selection *loop itself*, with the Anthropic client
    mocked to simulate realistic tool-use conversations (see
    `tests/test_agent_core.py`, `tests/test_api.py`)

  Supply your own `ANTHROPIC_API_KEY` to exercise the live agent — the
  wiring between "model asks for a tool" and "tool runs against the repo"
  is the same code path either way.
- **Heuristic analysis, not a full parser.** The analyzer uses regexes and
  dependency-manifest lookups, not a real AST/language server, so it works
  well on common project layouts (Express/FastAPI/Flask/Django/Rails/Spring
  style apps) but can miss unconventional structures or highly dynamic
  routing.
- **In-memory repo registry.** Loaded repositories live in the backend
  process's memory; restarting the backend forgets them (the cloned files
  on disk under `backend/workspace/` remain, but must be reloaded via
  `POST /api/repos`).
- **No streaming.** Responses are returned once the agent finishes, not
  token-by-token.
- **Single-tenant, no auth on the API itself.** Fine for local/demo use; not
  hardened for multi-user deployment.

## Future Improvements

- Semantic code search (embeddings) instead of keyword/regex search
- Dependency graph visualization
- Pull request analysis ("what does this diff change architecturally?")
- Deeper GitHub integration (issues, PR comments, CI status)
- Agent-proposed code change suggestions/diffs
- Multi-repository analysis (cross-repo questions in a microservices setup)
- IDE integration (VS Code extension surfacing the same Q&A inline)
- Streaming responses and persistent (non-in-memory) repo storage
