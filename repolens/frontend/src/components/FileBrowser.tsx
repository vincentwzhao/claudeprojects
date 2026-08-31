import { useEffect, useState } from "react";
import { api } from "../api/client";
import type { DirEntry } from "../api/client";

export function FileBrowser({ repoId }: { repoId: string }) {
  const [path, setPath] = useState(".");
  const [entries, setEntries] = useState<DirEntry[]>([]);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;
    api
      .listFiles(repoId, path)
      .then((res) => {
        if (!cancelled) setEntries(res.entries);
      })
      .catch((err) => !cancelled && setError(String(err)));
    return () => {
      cancelled = true;
    };
  }, [repoId, path]);

  const parts = path === "." ? [] : path.split("/");

  return (
    <div className="panel">
      <h2>Files</h2>
      <div className="breadcrumbs">
        <button onClick={() => setPath(".")}>root</button>
        {parts.map((part, i) => (
          <span key={i}>
            {" / "}
            <button onClick={() => setPath(parts.slice(0, i + 1).join("/"))}>{part}</button>
          </span>
        ))}
      </div>
      {error && <div className="error-box">{error}</div>}
      <ul className="file-list">
        {entries.map((entry) => (
          <li key={entry.name}>
            {entry.type === "dir" ? (
              <button className="file-entry dir" onClick={() => setPath(path === "." ? entry.name : `${path}/${entry.name}`)}>
                📁 {entry.name}
              </button>
            ) : (
              <span className="file-entry file">📄 {entry.name}</span>
            )}
          </li>
        ))}
      </ul>
    </div>
  );
}
