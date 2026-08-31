import { useState } from "react";
import type { FormEvent } from "react";

interface Props {
  onLoad: (source: string) => Promise<void>;
  loading: boolean;
  error: string | null;
}

const STEPS = [
  "Repository indexed",
  "Architecture analyzed",
  "Entry points identified",
  "Dependencies analyzed",
];

export function RepoLoader({ onLoad, loading, error }: Props) {
  const [source, setSource] = useState("");

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    if (!source.trim() || loading) return;
    await onLoad(source.trim());
  }

  return (
    <div className="repo-loader">
      <div className="repo-loader-card">
        <h1>RepoLens</h1>
        <p className="subtitle">AI-powered codebase onboarding &amp; exploration</p>
        <form onSubmit={handleSubmit}>
          <input
            type="text"
            placeholder="https://github.com/owner/repo or /path/to/local/repo"
            value={source}
            onChange={(e) => setSource(e.target.value)}
            disabled={loading}
            autoFocus
          />
          <button type="submit" disabled={loading || !source.trim()}>
            {loading ? "Analyzing…" : "Analyze Repository"}
          </button>
        </form>

        {loading && (
          <ul className="step-list pending">
            {STEPS.map((step) => (
              <li key={step}>
                <span className="spinner" /> {step}
              </li>
            ))}
          </ul>
        )}

        {error && <div className="error-box">{error}</div>}
      </div>
    </div>
  );
}
