import type { RepoSummary } from "../api/client";

export type View = "chat" | "architecture" | "dependencies" | "files";

interface Props {
  repo: RepoSummary;
  view: View;
  onChangeView: (view: View) => void;
  onRunProject: () => void;
  onStartExplain: () => void;
  onStartTrace: () => void;
  onNewRepo: () => void;
}

export function Sidebar({ repo, view, onChangeView, onRunProject, onStartExplain, onStartTrace, onNewRepo }: Props) {
  return (
    <aside className="sidebar">
      <div className="sidebar-brand">RepoLens</div>

      <div className="sidebar-section">
        <div className="sidebar-heading">Repository</div>
        <button className="sidebar-repo" onClick={onNewRepo} title="Load a different repository">
          <div className="sidebar-repo-name">{repo.name}</div>
          <div className="sidebar-repo-meta">{repo.file_count} files · {repo.frameworks[0] ?? Object.keys(repo.languages)[0] ?? "unknown"}</div>
        </button>
      </div>

      <div className="sidebar-section">
        <div className="sidebar-heading">Analysis</div>
        <nav>
          <button className="nav-item" onClick={onRunProject}>Overview</button>
          <button className={`nav-item ${view === "architecture" ? "active" : ""}`} onClick={() => onChangeView("architecture")}>Architecture</button>
          <button className={`nav-item ${view === "files" ? "active" : ""}`} onClick={() => onChangeView("files")}>Files</button>
          <button className={`nav-item ${view === "dependencies" ? "active" : ""}`} onClick={() => onChangeView("dependencies")}>Dependencies</button>
        </nav>
      </div>

      <div className="sidebar-section">
        <div className="sidebar-heading">Tools</div>
        <nav>
          <button className="nav-item" onClick={onStartExplain}>Explain</button>
          <button className="nav-item" onClick={onStartTrace}>Trace</button>
        </nav>
      </div>
    </aside>
  );
}
