import type { ToolCallOut } from "../api"

export default function ApprovalsPanel({
  approvals,
  onDecision,
}: {
  approvals: ToolCallOut[]
  onDecision: (toolCallId: number, approve: boolean) => void
}) {
  if (approvals.length === 0) {
    return <p className="text-xs text-slate-500">Nothing waiting on you right now.</p>
  }

  return (
    <ul className="flex flex-col gap-2">
      {approvals.map((a) => (
        <li key={a.id} className="rounded-lg border border-amber-700 bg-amber-950/40 px-3 py-2">
          <p className="text-sm text-amber-200">
            <span className="font-mono text-xs">{a.tool_name}</span>
          </p>
          <p className="mt-0.5 truncate text-xs text-amber-300/70">{a.arguments_json}</p>
          <div className="mt-2 flex gap-2">
            <button
              onClick={() => onDecision(a.id, true)}
              className="rounded bg-emerald-700 px-2 py-1 text-xs font-medium text-white hover:bg-emerald-600"
            >
              Approve
            </button>
            <button
              onClick={() => onDecision(a.id, false)}
              className="rounded bg-rose-700 px-2 py-1 text-xs font-medium text-white hover:bg-rose-600"
            >
              Deny
            </button>
          </div>
        </li>
      ))}
    </ul>
  )
}
