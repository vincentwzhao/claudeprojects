const STORAGE_KEY = "cancel-reminder.subscriptions";

const form = document.getElementById("sub-form");
const list = document.getElementById("sub-list");
const emptyState = document.getElementById("empty-state");
const banner = document.getElementById("banner");

function loadSubs() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw ? JSON.parse(raw) : [];
  } catch {
    return [];
  }
}

function saveSubs(subs) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(subs));
}

function daysUntil(dateStr) {
  const today = new Date();
  today.setHours(0, 0, 0, 0);
  const target = new Date(dateStr + "T00:00:00");
  return Math.round((target - today) / 86400000);
}

function statusFor(sub) {
  const days = daysUntil(sub.renewalDate);
  if (days < 0) return { key: "overdue", label: "Overdue — cancel now" };
  if (days <= sub.leadDays) return { key: "due-soon", label: "Cancel now" };
  if (days <= sub.leadDays + 7) return { key: "upcoming", label: `In ${days}d` };
  return { key: "ok", label: `In ${days}d` };
}

function advanceDate(dateStr, cycle) {
  const d = new Date(dateStr + "T00:00:00");
  if (cycle === "weekly") d.setDate(d.getDate() + 7);
  else if (cycle === "yearly") d.setFullYear(d.getFullYear() + 1);
  else d.setMonth(d.getMonth() + 1);
  return d.toISOString().slice(0, 10);
}

function formatDate(dateStr) {
  return new Date(dateStr + "T00:00:00").toLocaleDateString(undefined, {
    month: "short",
    day: "numeric",
    year: "numeric",
  });
}

function render() {
  const subs = loadSubs();
  subs.sort((a, b) => daysUntil(a.renewalDate) - daysUntil(b.renewalDate));

  list.innerHTML = "";
  emptyState.hidden = subs.length > 0;

  const needsAction = [];

  for (const sub of subs) {
    const status = statusFor(sub);
    if (status.key === "overdue" || status.key === "due-soon") {
      needsAction.push(sub);
    }

    const li = document.createElement("li");
    li.className = `sub-item status-${status.key}`;

    const costLabel = sub.cost ? `$${Number(sub.cost).toFixed(2)} / ${sub.cycle}` : sub.cycle;

    li.innerHTML = `
      <div class="sub-info">
        <span class="sub-name">${escapeHtml(sub.name)}</span>
        <span class="sub-meta">${costLabel} · renews ${formatDate(sub.renewalDate)}</span>
      </div>
      <span class="status-pill">${status.label}</span>
      <div class="sub-actions">
        <button data-action="renew" data-id="${sub.id}">Renewed</button>
        <button data-action="delete" data-id="${sub.id}">Cancelled</button>
      </div>
    `;
    list.appendChild(li);
  }

  updateBanner(needsAction);
}

function updateBanner(needsAction) {
  if (needsAction.length === 0) {
    banner.hidden = true;
    banner.textContent = "";
    return;
  }
  banner.hidden = false;
  const names = needsAction.map((s) => s.name).join(", ");
  banner.textContent =
    needsAction.length === 1
      ? `Cancel ${names} before it renews.`
      : `Cancel these before they renew: ${names}`;

  notify(needsAction);
}

let lastNotifiedKey = "";
function notify(needsAction) {
  if (!("Notification" in window) || Notification.permission !== "granted") return;
  const key = needsAction
    .map((s) => s.id)
    .sort()
    .join(",");
  if (key === lastNotifiedKey) return;
  lastNotifiedKey = key;
  const names = needsAction.map((s) => s.name).join(", ");
  new Notification("Cancel Reminder", {
    body:
      needsAction.length === 1
        ? `Time to cancel ${names}.`
        : `Time to cancel: ${names}.`,
  });
}

function escapeHtml(str) {
  const div = document.createElement("div");
  div.textContent = str;
  return div.innerHTML;
}

form.addEventListener("submit", (e) => {
  e.preventDefault();
  const subs = loadSubs();
  subs.push({
    id: crypto.randomUUID(),
    name: document.getElementById("name").value.trim(),
    cost: document.getElementById("cost").value,
    renewalDate: document.getElementById("renewal-date").value,
    cycle: document.getElementById("cycle").value,
    leadDays: Number(document.getElementById("lead-days").value) || 0,
  });
  saveSubs(subs);
  form.reset();
  document.getElementById("lead-days").value = 3;
  render();

  if ("Notification" in window && Notification.permission === "default") {
    Notification.requestPermission();
  }
});

list.addEventListener("click", (e) => {
  const btn = e.target.closest("button[data-action]");
  if (!btn) return;
  const { action, id } = btn.dataset;
  let subs = loadSubs();

  if (action === "delete") {
    subs = subs.filter((s) => s.id !== id);
  } else if (action === "renew") {
    subs = subs.map((s) =>
      s.id === id ? { ...s, renewalDate: advanceDate(s.renewalDate, s.cycle) } : s
    );
  }

  saveSubs(subs);
  render();
});

if ("Notification" in window && Notification.permission === "default") {
  Notification.requestPermission();
}

render();
