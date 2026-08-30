import type { Conversation } from "../api"

export default function Sidebar({
  conversations,
  activeId,
  onSelect,
  onNew,
}: {
  conversations: Conversation[]
  activeId: number | null
  onSelect: (id: number) => void
  onNew: () => void
}) {
  return (
    <div className="flex h-full flex-col border-r border-slate-800 bg-slate-950">
      <div className="p-3">
        <button
          onClick={onNew}
          className="w-full rounded-lg border border-slate-700 px-3 py-2 text-left text-sm text-slate-200 hover:bg-slate-800"
        >
          + New chat
        </button>
      </div>
      <div className="flex-1 overflow-y-auto px-2 pb-3">
        {conversations.map((c) => (
          <button
            key={c.id}
            onClick={() => onSelect(c.id)}
            className={`mb-1 block w-full truncate rounded-lg px-3 py-2 text-left text-sm ${
              c.id === activeId ? "bg-slate-800 text-white" : "text-slate-400 hover:bg-slate-900"
            }`}
            title={c.title}
          >
            {c.title || "New conversation"}
          </button>
        ))}
      </div>
    </div>
  )
}
