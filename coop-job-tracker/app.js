const STORAGE_KEY = "coopJobApplications";

const STATUS_LABELS = {
  applied: "Applied",
  oa: "Online Assessment",
  interview: "Interview",
  offer: "Offer",
  rejected: "Rejected",
  ghosted: "Ghosted",
};

const STATUS_ORDER = ["applied", "oa", "interview", "offer", "rejected", "ghosted"];

function loadApplications() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw ? JSON.parse(raw) : [];
  } catch (err) {
    console.error("Failed to load applications from storage", err);
    return [];
  }
}

function saveApplications(apps) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(apps));
}

let applications = loadApplications();

const els = {
  form: document.getElementById("app-form"),
  id: document.getElementById("app-id"),
  company: document.getElementById("field-company"),
  role: document.getElementById("field-role"),
  date: document.getElementById("field-date"),
  category: document.getElementById("field-category"),
  status: document.getElementById("field-status"),
  notes: document.getElementById("field-notes"),
  formTitle: document.getElementById("form-title"),
  submitBtn: document.getElementById("submit-btn"),
  cancelEditBtn: document.getElementById("cancel-edit-btn"),
  filterCategory: document.getElementById("filter-category"),
  filterStatus: document.getElementById("filter-status"),
  tableBody: document.getElementById("app-table-body"),
  emptyState: document.getElementById("empty-state"),
  exportBtn: document.getElementById("export-csv-btn"),
  statTotal: document.getElementById("stat-total"),
  statCs: document.getElementById("stat-cs"),
  statCsPct: document.getElementById("stat-cs-pct"),
  statInterviews: document.getElementById("stat-interviews"),
  statOffers: document.getElementById("stat-offers"),
  statusChart: document.getElementById("status-chart"),
};

function escapeHtml(str) {
  const div = document.createElement("div");
  div.textContent = str ?? "";
  return div.innerHTML;
}

function resetForm() {
  els.form.reset();
  els.id.value = "";
  els.formTitle.textContent = "Add Application";
  els.submitBtn.textContent = "Add Application";
  els.cancelEditBtn.hidden = true;
}

function startEdit(app) {
  els.id.value = app.id;
  els.company.value = app.company;
  els.role.value = app.role;
  els.date.value = app.date;
  els.category.value = app.category;
  els.status.value = app.status;
  els.notes.value = app.notes || "";
  els.formTitle.textContent = "Edit Application";
  els.submitBtn.textContent = "Save Changes";
  els.cancelEditBtn.hidden = false;
  els.company.focus();
}

function deleteApplication(id) {
  applications = applications.filter((a) => a.id !== id);
  saveApplications(applications);
  render();
}

function getFilteredApplications() {
  const category = els.filterCategory.value;
  const status = els.filterStatus.value;
  return applications.filter((a) => {
    if (category !== "all" && a.category !== category) return false;
    if (status !== "all" && a.status !== status) return false;
    return true;
  });
}

function renderStats() {
  const total = applications.length;
  const csCount = applications.filter((a) => a.category === "cs").length;
  const interviews = applications.filter((a) => a.status === "interview").length;
  const offers = applications.filter((a) => a.status === "offer").length;
  const csPct = total ? Math.round((csCount / total) * 100) : 0;

  els.statTotal.textContent = total;
  els.statCs.textContent = csCount;
  els.statCsPct.textContent = `${csPct}%`;
  els.statInterviews.textContent = interviews;
  els.statOffers.textContent = offers;
}

function renderStatusChart() {
  const total = applications.length;
  els.statusChart.innerHTML = "";

  STATUS_ORDER.forEach((key) => {
    const count = applications.filter((a) => a.status === key).length;
    const pct = total ? (count / total) * 100 : 0;

    const row = document.createElement("div");
    row.className = "bar-row";

    const label = document.createElement("span");
    label.textContent = STATUS_LABELS[key];

    const track = document.createElement("div");
    track.className = "bar-track";
    const fill = document.createElement("div");
    fill.className = "bar-fill";
    fill.style.width = `${pct}%`;
    track.appendChild(fill);

    const countEl = document.createElement("span");
    countEl.textContent = count;

    row.appendChild(label);
    row.appendChild(track);
    row.appendChild(countEl);
    els.statusChart.appendChild(row);
  });
}

function renderTable() {
  const rows = getFilteredApplications().sort((a, b) => (a.date < b.date ? 1 : -1));
  els.tableBody.innerHTML = "";
  els.emptyState.hidden = rows.length > 0;

  rows.forEach((app) => {
    const tr = document.createElement("tr");
    tr.innerHTML = `
      <td>${escapeHtml(app.company)}</td>
      <td>${escapeHtml(app.role)}</td>
      <td>${escapeHtml(app.date)}</td>
      <td><span class="badge ${app.category === "cs" ? "cs" : "non-cs"}">${
        app.category === "cs" ? "CS-Related" : "Non-CS"
      }</span></td>
      <td><span class="badge status-${app.status}">${STATUS_LABELS[app.status]}</span></td>
      <td class="notes-cell">${escapeHtml(app.notes)}</td>
      <td></td>
    `;

    const actionsCell = tr.lastElementChild;
    const editBtn = document.createElement("button");
    editBtn.className = "icon-btn";
    editBtn.textContent = "Edit";
    editBtn.addEventListener("click", () => startEdit(app));

    const deleteBtn = document.createElement("button");
    deleteBtn.className = "icon-btn";
    deleteBtn.textContent = "Delete";
    deleteBtn.addEventListener("click", () => {
      if (confirm(`Delete application to ${app.company}?`)) {
        deleteApplication(app.id);
      }
    });

    actionsCell.appendChild(editBtn);
    actionsCell.appendChild(deleteBtn);
    els.tableBody.appendChild(tr);
  });
}

function render() {
  renderStats();
  renderStatusChart();
  renderTable();
}

function exportCsv() {
  const header = ["Company", "Role", "Date Applied", "Category", "Status", "Notes"];
  const rows = applications
    .slice()
    .sort((a, b) => (a.date < b.date ? 1 : -1))
    .map((a) => [
      a.company,
      a.role,
      a.date,
      a.category === "cs" ? "CS-Related" : "Non-CS",
      STATUS_LABELS[a.status],
      a.notes || "",
    ]);

  const csv = [header, ...rows]
    .map((row) => row.map((cell) => `"${String(cell).replace(/"/g, '""')}"`).join(","))
    .join("\n");

  const blob = new Blob([csv], { type: "text/csv" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = "coop-job-applications.csv";
  a.click();
  URL.revokeObjectURL(url);
}

els.form.addEventListener("submit", (e) => {
  e.preventDefault();

  const editingId = els.id.value;
  const data = {
    company: els.company.value.trim(),
    role: els.role.value.trim(),
    date: els.date.value,
    category: els.category.value,
    status: els.status.value,
    notes: els.notes.value.trim(),
  };

  if (editingId) {
    applications = applications.map((a) => (a.id === editingId ? { ...a, ...data } : a));
  } else {
    applications.push({ id: crypto.randomUUID(), ...data });
  }

  saveApplications(applications);
  resetForm();
  render();
});

els.cancelEditBtn.addEventListener("click", resetForm);
els.filterCategory.addEventListener("change", renderTable);
els.filterStatus.addEventListener("change", renderTable);
els.exportBtn.addEventListener("click", exportCsv);

render();
