export interface RepoSummary {
  id: string;
  name: string;
  source: string;
  is_local: boolean;
  file_count: number;
  languages: Record<string, number>;
  frameworks: string[];
  entry_points: { path: string; reason: string }[];
  steps: string[];
}

export interface Reference {
  file: string;
  line: number | null;
}

export interface ToolCall {
  tool: string;
  input: Record<string, unknown>;
  output_summary: string;
}

export interface AgentResponse {
  answer: string;
  references: Reference[];
  trace: ToolCall[];
}

export interface RepositoryAnalysis {
  languages: Record<string, number>;
  frameworks: string[];
  entry_points: { path: string; reason: string }[];
  important_directories: { path: string; description: string; file_count: number }[];
  major_modules: { name: string; file_count: number }[];
  api_routes: { method: string; path: string; file: string; line: number; framework: string }[];
  api_route_count: number;
  database_layer: { type: string; evidence: string }[];
  authentication: { type: string; evidence: string }[];
  external_services: { type: string; evidence: string }[];
  configuration: string[];
  tests: { directories: string[]; framework: string | null };
  build_system: { tool: string; manifest?: string; script?: string }[];
  dependencies: Record<string, { name: string; version?: string }[]>;
  potential_issues: string[];
  file_count: number;
}

export interface DirEntry {
  name: string;
  type: "file" | "dir";
}

class ApiError extends Error {
  status: number;

  constructor(status: number, message: string) {
    super(message);
    this.status = status;
  }
}

async function request<T>(path: string, options?: RequestInit): Promise<T> {
  const res = await fetch(path, {
    ...options,
    headers: { "Content-Type": "application/json", ...(options?.headers ?? {}) },
  });
  if (!res.ok) {
    let detail = res.statusText;
    try {
      const body = await res.json();
      detail = body.detail ?? JSON.stringify(body);
    } catch {
      /* ignore parse errors */
    }
    throw new ApiError(res.status, detail);
  }
  return res.json() as Promise<T>;
}

export const api = {
  loadRepo: (source: string) =>
    request<RepoSummary>("/api/repos", { method: "POST", body: JSON.stringify({ source }) }),

  getRepo: (repoId: string) => request<RepoSummary>(`/api/repos/${repoId}`),

  getAnalysis: (repoId: string) => request<RepositoryAnalysis>(`/api/repos/${repoId}/analysis`),

  listFiles: (repoId: string, path = ".") =>
    request<{ path: string; entries: DirEntry[] }>(
      `/api/repos/${repoId}/files?path=${encodeURIComponent(path)}`
    ),

  chat: (repoId: string, message: string) =>
    request<AgentResponse>(`/api/repos/${repoId}/chat`, {
      method: "POST",
      body: JSON.stringify({ message }),
    }),

  explain: (repoId: string, concept: string) =>
    request<AgentResponse>(`/api/repos/${repoId}/explain`, {
      method: "POST",
      body: JSON.stringify({ concept }),
    }),

  trace: (repoId: string, feature: string) =>
    request<AgentResponse>(`/api/repos/${repoId}/trace`, {
      method: "POST",
      body: JSON.stringify({ feature }),
    }),

  project: (repoId: string) =>
    request<AgentResponse>(`/api/repos/${repoId}/project`, { method: "POST" }),
};

export { ApiError };
