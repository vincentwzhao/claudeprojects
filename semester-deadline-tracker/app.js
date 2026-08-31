const STORAGE_KEY = "semester-deadline-tracker:deadlines";

const DEFAULT_DEADLINES = [
  {
    title: "Fall term classes begin",
    date: "2026-09-09",
    category: "academic",
    notes: "First day of classes.",
  },
  {
    title: "Last day to add/drop courses without penalty",
    date: "2026-09-18",
    category: "enrollment",
    notes: "Confirm exact refund tier on the SFU deadlines page.",
  },
  {
    title: "Tuition payment due",
    date: "2026-09-22",
    category: "payment",
    notes: "Late fees (2% monthly interest for payment-plan students) apply after this date.",
  },
  {
    title: "Last day of fall exams",
    date: "2026-12-20",
    category: "academic",
    notes: "",
  },
];

function loadDeadlines() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return seedDefaults();
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed) || parsed.length === 0) return seedDefaults();
    return parsed;
  } catch {
    return seedDefaults();
  }
}

function seedDefaults() {
  const seeded = DEFAULT_DEADLINES.map((d) => ({ ...d, id: makeId(), done: false }));
  saveDeadlines(seeded);
  return seeded;
}

function saveDeadlines(deadlines) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(deadlines));
  } catch {
    // localStorage unavailable (private mode, etc.) — state just won't persist.
  }
}

function makeId() {
  return typeof crypto !== "undefined" && crypto.randomUUID
    ? crypto.randomUUID()
    : `id-${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

function daysUntil(dateStr) {
  const today = new Date();
  today.setHours(0, 0, 0, 0);
  const target = new Date(dateStr + "T00:00:00");
  return Math.round((target - today) / 86400000);
}

function formatDate(dateStr) {
  const d = new Date(dateStr + "T00:00:00");
  return d.toLocaleDateString(undefined, { weekday: "short", month: "short", day: "numeric", year: "numeric" });
}

function countdownLabel(days) {
  if (days < 0) return `${Math.abs(days)} day${Math.abs(days) === 1 ? "" : "s"} ago`;
  if (days === 0) return "Today";
  if (days === 1) return "Tomorrow";
  return `${days} days left`;
}

function countdownClass(days) {
  if (days < 0) return "past";
  if (days <= 3) return "urgent";
  if (days <= 10) return "soon";
  return "normal";
}

let deadlines = loadDeadlines();

function render() {
  deadlines.sort((a, b) => a.date.localeCompare(b.date));

  const upcoming = deadlines.filter((d) => !d.done && daysUntil(d.date) >= 0);
  const past = deadlines.filter((d) => d.done || daysUntil(d.date) < 0);

  renderList(document.getElementById("upcomingList"), upcoming);
  renderList(document.getElementById("pastList"), past.slice().reverse());

  document.getElementById("upcomingEmpty").hidden = upcoming.length > 0;
  document.getElementById("pastEmpty").hidden = past.length > 0;

  renderHero(upcoming);
}

function renderHero(upcoming) {
  const nextDaysEl = document.getElementById("nextDays");
  const nextLabelEl = document.getElementById("nextLabel");

  const nextPayment = upcoming.find((d) => d.category === "payment") || upcoming[0];

  if (!nextPayment) {
    nextDaysEl.textContent = "–";
    nextLabelEl.textContent = "No upcoming deadlines";
    return;
  }

  const days = daysUntil(nextPayment.date);
  nextDaysEl.textContent = days;
  nextLabelEl.textContent = `day${days === 1 ? "" : "s"} until "${nextPayment.title}" (${formatDate(nextPayment.date)})`;
}

function renderList(listEl, items) {
  listEl.innerHTML = "";
  for (const d of items) {
    const days = daysUntil(d.date);
    const li = document.createElement("li");
    li.className = "deadline-item" + (d.done ? " done" : "");

    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.checked = d.done;
    checkbox.addEventListener("change", () => {
      d.done = checkbox.checked;
      saveDeadlines(deadlines);
      render();
    });

    const main = document.createElement("div");
    main.className = "deadline-main";

    const title = document.createElement("div");
    title.className = "deadline-title";
    title.textContent = d.title;

    const meta = document.createElement("div");
    meta.className = "deadline-meta";

    const badge = document.createElement("span");
    badge.className = `badge ${d.category}`;
    badge.textContent = d.category;

    const dateSpan = document.createElement("span");
    dateSpan.textContent = formatDate(d.date);

    meta.append(badge, dateSpan);

    if (d.notes) {
      const notesSpan = document.createElement("span");
      notesSpan.textContent = `· ${d.notes}`;
      meta.append(notesSpan);
    }

    main.append(title, meta);

    const countdown = document.createElement("span");
    countdown.className = `countdown ${countdownClass(days)}`;
    countdown.textContent = countdownLabel(days);

    const deleteBtn = document.createElement("button");
    deleteBtn.className = "delete-btn";
    deleteBtn.setAttribute("aria-label", "Delete deadline");
    deleteBtn.textContent = "×";
    deleteBtn.addEventListener("click", () => {
      deadlines = deadlines.filter((x) => x.id !== d.id);
      saveDeadlines(deadlines);
      render();
    });

    li.append(checkbox, main, countdown, deleteBtn);
    listEl.append(li);
  }
}

document.getElementById("addForm").addEventListener("submit", (e) => {
  e.preventDefault();
  const title = document.getElementById("title").value.trim();
  const date = document.getElementById("date").value;
  const category = document.getElementById("category").value;
  const notes = document.getElementById("notes").value.trim();

  if (!title || !date) return;

  deadlines.push({ id: makeId(), title, date, category, notes, done: false });
  saveDeadlines(deadlines);
  e.target.reset();
  render();
});

document.getElementById("resetBtn").addEventListener("click", () => {
  if (!confirm("Reset to the default SFU dates? This removes any deadlines you've added.")) return;
  deadlines = seedDefaults();
  render();
});

render();
