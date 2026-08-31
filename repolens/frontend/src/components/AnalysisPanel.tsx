import type { RepositoryAnalysis } from "../api/client";

export function ArchitecturePanel({ analysis }: { analysis: RepositoryAnalysis }) {
  return (
    <div className="panel">
      <h2>Architecture</h2>

      <section>
        <h3>Languages</h3>
        <ul className="kv-list">
          {Object.entries(analysis.languages).map(([lang, count]) => (
            <li key={lang}><span>{lang}</span><span className="muted">{count} files</span></li>
          ))}
        </ul>
      </section>

      <section>
        <h3>Frameworks</h3>
        {analysis.frameworks.length ? (
          <div className="tag-list">{analysis.frameworks.map((f) => <span className="tag" key={f}>{f}</span>)}</div>
        ) : <p className="muted">None detected.</p>}
      </section>

      <section>
        <h3>Entry Points</h3>
        <ul className="kv-list">
          {analysis.entry_points.map((e) => (
            <li key={e.path}><code>{e.path}</code><span className="muted">{e.reason}</span></li>
          ))}
        </ul>
      </section>

      <section>
        <h3>Important Directories</h3>
        <ul className="kv-list">
          {analysis.important_directories.map((d) => (
            <li key={d.path}><code>{d.path}/</code><span className="muted">{d.description} ({d.file_count})</span></li>
          ))}
        </ul>
      </section>

      <section>
        <h3>API Routes ({analysis.api_route_count})</h3>
        <ul className="route-list">
          {analysis.api_routes.slice(0, 30).map((r, i) => (
            <li key={i}>
              <span className={`method method-${r.method.toLowerCase()}`}>{r.method}</span>
              <code>{r.path}</code>
              <span className="muted">{r.file}:{r.line}</span>
            </li>
          ))}
        </ul>
      </section>

      <section>
        <h3>Database Layer</h3>
        {analysis.database_layer.length
          ? <ul className="kv-list">{analysis.database_layer.map((d, i) => <li key={i}><span>{d.type}</span><span className="muted">{d.evidence}</span></li>)}</ul>
          : <p className="muted">None detected.</p>}
      </section>

      <section>
        <h3>Authentication</h3>
        {analysis.authentication.length
          ? <ul className="kv-list">{analysis.authentication.map((a, i) => <li key={i}><span>{a.type}</span><span className="muted">{a.evidence}</span></li>)}</ul>
          : <p className="muted">None detected.</p>}
      </section>

      <section>
        <h3>External Services</h3>
        {analysis.external_services.length
          ? <div className="tag-list">{analysis.external_services.map((s, i) => <span className="tag" key={i}>{s.type}</span>)}</div>
          : <p className="muted">None detected.</p>}
      </section>

      <section>
        <h3>Tests</h3>
        <p>
          Framework: <strong>{analysis.tests.framework ?? "not detected"}</strong>
          {analysis.tests.directories.length > 0 && <> · Directories: {analysis.tests.directories.join(", ")}</>}
        </p>
      </section>

      <section>
        <h3>Build System</h3>
        <ul className="kv-list">
          {analysis.build_system.map((b, i) => (
            <li key={i}><span>{b.tool}</span><span className="muted">{b.manifest ?? b.script}</span></li>
          ))}
        </ul>
      </section>

      <section>
        <h3>Potential Issues</h3>
        {analysis.potential_issues.length
          ? <ul className="issue-list">{analysis.potential_issues.map((issue, i) => <li key={i}>{issue}</li>)}</ul>
          : <p className="muted">None flagged.</p>}
      </section>
    </div>
  );
}

export function DependenciesPanel({ analysis }: { analysis: RepositoryAnalysis }) {
  const ecosystems = Object.entries(analysis.dependencies);
  return (
    <div className="panel">
      <h2>Dependencies</h2>
      {ecosystems.length === 0 && <p className="muted">No dependency manifest detected.</p>}
      {ecosystems.map(([ecosystem, deps]) => (
        <section key={ecosystem}>
          <h3>{ecosystem} ({deps.length})</h3>
          <div className="tag-list">
            {deps.map((d, i) => (
              <span className="tag" key={i}>{d.name}{d.version ? `@${d.version}` : ""}</span>
            ))}
          </div>
        </section>
      ))}
    </div>
  );
}
