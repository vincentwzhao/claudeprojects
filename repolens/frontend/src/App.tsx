import { useCallback, useState } from "react";
import "./App.css";
import { ApiError, api } from "./api/client";
import type { RepoSummary, RepositoryAnalysis } from "./api/client";
import { ArchitecturePanel, DependenciesPanel } from "./components/AnalysisPanel";
import { Chat } from "./components/Chat";
import { FileBrowser } from "./components/FileBrowser";
import { RepoLoader } from "./components/RepoLoader";
import { Sidebar } from "./components/Sidebar";
import type { View } from "./components/Sidebar";
import type { ChatMessage } from "./types";

function App() {
  const [repo, setRepo] = useState<RepoSummary | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  const [view, setView] = useState<View>("chat");
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [busy, setBusy] = useState(false);
  const [inputSeed, setInputSeed] = useState("");
  const [analysis, setAnalysis] = useState<RepositoryAnalysis | null>(null);

  async function handleLoadRepo(source: string) {
    setLoading(true);
    setLoadError(null);
    try {
      const summary = await api.loadRepo(source);
      setRepo(summary);
      const fullAnalysis = await api.getAnalysis(summary.id);
      setAnalysis(fullAnalysis);
      setMessages([]);
      setView("chat");
    } catch (err) {
      setLoadError(err instanceof ApiError ? err.message : "Failed to load repository. Is the backend running?");
    } finally {
      setLoading(false);
    }
  }

  const runCommand = useCallback(
    async (userLabel: string, call: () => Promise<{ answer: string; references: { file: string; line: number | null }[]; trace: { tool: string; input: Record<string, unknown>; output_summary: string }[] }>) => {
      if (!repo) return;
      setMessages((prev) => [...prev, { role: "user", content: userLabel }, { role: "assistant", content: "", pending: true }]);
      setBusy(true);
      try {
        const result = await call();
        setMessages((prev) => [
          ...prev.slice(0, -1),
          { role: "assistant", content: result.answer, references: result.references, trace: result.trace },
        ]);
      } catch (err) {
        const message = err instanceof ApiError ? err.message : "Something went wrong talking to the backend.";
        setMessages((prev) => [...prev.slice(0, -1), { role: "assistant", content: message, error: true }]);
      } finally {
        setBusy(false);
      }
    },
    [repo]
  );

  function handleSend(text: string) {
    if (!repo) return;
    setView("chat");
    if (text.startsWith("/explain ")) {
      const concept = text.slice("/explain ".length).trim();
      runCommand(text, () => api.explain(repo.id, concept));
    } else if (text.startsWith("/trace ")) {
      const feature = text.slice("/trace ".length).trim();
      runCommand(text, () => api.trace(repo.id, feature));
    } else if (text === "/project" || text.startsWith("/project ")) {
      runCommand(text, () => api.project(repo.id));
    } else {
      runCommand(text, () => api.chat(repo.id, text));
    }
  }

  function handleRunProject() {
    if (!repo) return;
    setView("chat");
    runCommand("/project", () => api.project(repo.id));
  }

  if (!repo) {
    return <RepoLoader onLoad={handleLoadRepo} loading={loading} error={loadError} />;
  }

  return (
    <div className="app">
      <Sidebar
        repo={repo}
        view={view}
        onChangeView={setView}
        onRunProject={handleRunProject}
        onStartExplain={() => {
          setView("chat");
          setInputSeed("/explain ");
        }}
        onStartTrace={() => {
          setView("chat");
          setInputSeed("/trace ");
        }}
        onNewRepo={() => {
          setRepo(null);
          setAnalysis(null);
          setMessages([]);
        }}
      />

      <main className="main">
        {view === "chat" && (
          <Chat
            messages={messages}
            onSend={handleSend}
            busy={busy}
            inputSeed={inputSeed}
            onSeedConsumed={() => setInputSeed("")}
          />
        )}
        {view === "architecture" && analysis && <ArchitecturePanel analysis={analysis} />}
        {view === "dependencies" && analysis && <DependenciesPanel analysis={analysis} />}
        {view === "files" && <FileBrowser repoId={repo.id} />}
      </main>
    </div>
  );
}

export default App;
