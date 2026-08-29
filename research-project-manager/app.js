const STORAGE_KEY = "rpm-data-v1";

const STATUS_OPTIONS = ["Interested", "Applying", "Applied", "Interview", "Accepted", "Rejected"];

const DEFAULT_DATA = {
  pastProjects: [],
  opportunities: [
    {
      id: uid(),
      title: "Verification in Intent-Driven Network Management Automation",
      faculty: "",
      department: "",
      deadline: "",
      status: "Interested",
      tags: [
        "networking",
        "intent-based networking",
        "LLM",
        "P4",
        "PINC",
        "verification",
        "telemetry",
        "multi-agent systems",
      ],
      description:
        "The growing scale and complexity of networks necessitate automated network management. Driven by this demand and accelerated by recent advancements in large language models (LLMs), network management practices are reconsidering the adoption of intent-based networking (IBN), translating high-level intents into network configurations.\n\n" +
        "Building on our previous project and ongoing development efforts on building PINC (P4 and Intent for Network Configuration), expanding its validation pipeline, and exploring multi-agent deployments, this sub-project has three major goals: (1) enhancing the verification and feedback loops, (2) adding telemetry-based verification, (3) real-world deployment for one use-case scenario.\n\n" +
        "Why involvement in this project provides research training: The project involves learning about current solutions, an introduction to the tools and methodologies involved including programmable devices, verification methodologies, multi-agent systems, and involvement in collaborations within a team through iterations of development, testing, progress, or rejection on a base idea.",
      notes: "Source: SFU Canvas — Call for Undergraduate Research Explorers.",
      links: [],
    },
  ],
};

function uid() {
  return Math.random().toString(36).slice(2, 10);
}

function loadData() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return structuredClone(DEFAULT_DATA);
    const parsed = JSON.parse(raw);
    if (!parsed.pastProjects || !parsed.opportunities) throw new Error("bad shape");
    return parsed;
  } catch (e) {
    return structuredClone(DEFAULT_DATA);
  }
}

function saveData() {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
  } catch (e) {
    console.warn("Could not save to localStorage", e);
  }
}

let state = loadData();

function parseTags(str) {
  return str
    .split(",")
    .map((t) => t.trim())
    .filter(Boolean);
}

function tagOverlap(tagsA, tagsB) {
  const setB = new Set(tagsB.map((t) => t.toLowerCase()));
  return tagsA.filter((t) => setB.has(t.toLowerCase()));
}

function findPast(id) {
  return state.pastProjects.find((p) => p.id === id);
}
function findOpp(id) {
  return state.opportunities.find((o) => o.id === id);
}

// ---------- Rendering ----------

function render() {
  renderPastList();
  renderOppList();
  saveData();
}

function renderPastList() {
  const list = document.getElementById("pastList");
  const countEl = document.getElementById("pastCount");
  countEl.textContent = state.pastProjects.length;
  list.innerHTML = "";

  if (state.pastProjects.length === 0) {
    list.innerHTML = '<div class="empty-state">No past projects yet. Add the research/coursework you\'ve already done so the app can suggest connections.</div>';
    return;
  }

  state.pastProjects.forEach((p) => {
    const linkedCount = state.opportunities.filter((o) =>
      o.links.some((l) => l.pastProjectId === p.id)
    ).length;

    const card = document.createElement("div");
    card.className = "card";
    card.innerHTML = `
      <div class="card-top">
        <div>
          <div class="card-title">${escapeHtml(p.title)}</div>
          <div class="card-meta">${escapeHtml(p.role || "")}</div>
        </div>
        ${linkedCount > 0 ? `<span class="link-chip">${linkedCount} linked</span>` : ""}
      </div>
      <div class="card-desc">${escapeHtml(p.description || "")}</div>
      <div class="tag-row">${p.tags.map((t) => `<span class="tag">${escapeHtml(t)}</span>`).join("")}</div>
    `;
    card.addEventListener("click", () => openPastModal(p.id));
    list.appendChild(card);
  });
}

function renderOppList() {
  const list = document.getElementById("oppList");
  const countEl = document.getElementById("oppCount");
  const statusFilter = document.getElementById("statusFilter").value;
  const sortBy = document.getElementById("sortSelect").value;

  let items = [...state.opportunities];
  countEl.textContent = state.opportunities.length;

  if (statusFilter) items = items.filter((o) => o.status === statusFilter);

  items.sort((a, b) => {
    if (sortBy === "title") return a.title.localeCompare(b.title);
    if (sortBy === "status") return a.status.localeCompare(b.status);
    // deadline: empty deadlines last
    if (!a.deadline && !b.deadline) return 0;
    if (!a.deadline) return 1;
    if (!b.deadline) return -1;
    return a.deadline.localeCompare(b.deadline);
  });

  list.innerHTML = "";

  if (items.length === 0) {
    list.innerHTML = '<div class="empty-state">No opportunities match this filter.</div>';
    return;
  }

  items.forEach((o) => {
    const card = document.createElement("div");
    card.className = "card";
    const metaParts = [o.faculty, o.department].filter(Boolean).join(" · ");
    const deadlineStr = o.deadline ? `Deadline: ${o.deadline}` : "No deadline set";

    card.innerHTML = `
      <div class="card-top">
        <div>
          <div class="card-title">${escapeHtml(o.title)}</div>
          <div class="card-meta">${escapeHtml(metaParts)}${metaParts ? " · " : ""}${escapeHtml(deadlineStr)}</div>
        </div>
        <span class="status-pill status-${o.status}">${o.status}</span>
      </div>
      <div class="card-desc">${escapeHtml(o.description || "")}</div>
      <div class="tag-row">${o.tags.map((t) => `<span class="tag">${escapeHtml(t)}</span>`).join("")}</div>
      ${
        o.links.length > 0
          ? `<div class="link-row">${o.links
              .map((l) => {
                const p = findPast(l.pastProjectId);
                return p ? `<span class="link-chip">↳ ${escapeHtml(p.title)}</span>` : "";
              })
              .join("")}</div>`
          : ""
      }
    `;
    card.addEventListener("click", () => openOppModal(o.id));
    list.appendChild(card);
  });
}

function escapeHtml(str) {
  const div = document.createElement("div");
  div.textContent = str ?? "";
  return div.innerHTML;
}

// ---------- Modals ----------

const backdrop = document.getElementById("modalBackdrop");
const modal = document.getElementById("modal");

function closeModal() {
  backdrop.hidden = true;
  modal.innerHTML = "";
}

backdrop.addEventListener("click", (e) => {
  if (e.target === backdrop) closeModal();
});

function openPastModal(id) {
  const isEdit = Boolean(id);
  const p = isEdit ? findPast(id) : { id: uid(), title: "", role: "", tags: [], description: "" };

  modal.innerHTML = `
    <h3>${isEdit ? "Edit" : "Add"} past project</h3>
    <div class="field">
      <label>Title</label>
      <input id="f-title" value="${escapeAttr(p.title)}" placeholder="e.g. Undergraduate research on X" />
    </div>
    <div class="field">
      <label>Role / context</label>
      <input id="f-role" value="${escapeAttr(p.role)}" placeholder="e.g. Research assistant, course project, personal project" />
    </div>
    <div class="field">
      <label>Description</label>
      <textarea id="f-desc" placeholder="What you did, what you learned">${escapeHtml(p.description)}</textarea>
    </div>
    <div class="field">
      <label>Tags / skills (comma-separated)</label>
      <input id="f-tags" value="${escapeAttr(p.tags.join(", "))}" placeholder="e.g. networking, python, verification" />
    </div>
    <div class="modal-actions">
      <div>${isEdit ? '<button class="btn danger" id="deleteBtn">Delete</button>' : ""}</div>
      <div class="right">
        <button class="btn secondary" id="cancelBtn">Cancel</button>
        <button class="btn primary" id="saveBtn">Save</button>
      </div>
    </div>
  `;
  backdrop.hidden = false;

  document.getElementById("cancelBtn").addEventListener("click", closeModal);
  if (isEdit) {
    document.getElementById("deleteBtn").addEventListener("click", () => {
      state.pastProjects = state.pastProjects.filter((x) => x.id !== id);
      state.opportunities.forEach((o) => {
        o.links = o.links.filter((l) => l.pastProjectId !== id);
      });
      render();
      closeModal();
    });
  }
  document.getElementById("saveBtn").addEventListener("click", () => {
    const updated = {
      id: p.id,
      title: document.getElementById("f-title").value.trim() || "Untitled project",
      role: document.getElementById("f-role").value.trim(),
      description: document.getElementById("f-desc").value.trim(),
      tags: parseTags(document.getElementById("f-tags").value),
    };
    if (isEdit) {
      Object.assign(p, updated);
    } else {
      state.pastProjects.push(updated);
    }
    render();
    closeModal();
  });
}

function openOppModal(id) {
  const isEdit = Boolean(id);
  const o = isEdit
    ? findOpp(id)
    : { id: uid(), title: "", faculty: "", department: "", deadline: "", status: "Interested", tags: [], description: "", notes: "", links: [] };

  const suggestions = isEdit
    ? state.pastProjects
        .filter((p) => !o.links.some((l) => l.pastProjectId === p.id))
        .map((p) => ({ project: p, overlap: tagOverlap(o.tags, p.tags) }))
        .filter((s) => s.overlap.length > 0)
        .sort((a, b) => b.overlap.length - a.overlap.length)
    : [];

  modal.innerHTML = `
    <h3>${isEdit ? "Edit" : "Add"} opportunity</h3>
    <div class="field">
      <label>Title</label>
      <input id="f-title" value="${escapeAttr(o.title)}" placeholder="Project title" />
    </div>
    <div class="field-row">
      <div class="field">
        <label>Faculty / mentor</label>
        <input id="f-faculty" value="${escapeAttr(o.faculty)}" />
      </div>
      <div class="field">
        <label>Department</label>
        <input id="f-department" value="${escapeAttr(o.department)}" />
      </div>
    </div>
    <div class="field-row">
      <div class="field">
        <label>Deadline</label>
        <input id="f-deadline" type="date" value="${escapeAttr(o.deadline)}" />
      </div>
      <div class="field">
        <label>Status</label>
        <select id="f-status">
          ${STATUS_OPTIONS.map((s) => `<option value="${s}" ${s === o.status ? "selected" : ""}>${s}</option>`).join("")}
        </select>
      </div>
    </div>
    <div class="field">
      <label>Description</label>
      <textarea id="f-desc" placeholder="Paste the project description">${escapeHtml(o.description)}</textarea>
    </div>
    <div class="field">
      <label>Tags / topics (comma-separated)</label>
      <input id="f-tags" value="${escapeAttr(o.tags.join(", "))}" placeholder="e.g. networking, LLM, verification" />
    </div>
    <div class="field">
      <label>Your notes</label>
      <textarea id="f-notes" placeholder="Application notes, contacts, ideas for your pitch">${escapeHtml(o.notes)}</textarea>
    </div>

    ${
      isEdit
        ? `
    <div class="field">
      <label>Linked past projects</label>
      <div class="linked-list" id="linkedList">
        ${
          o.links.length === 0
            ? '<span style="color:var(--muted); font-size:0.8rem;">None linked yet.</span>'
            : o.links
                .map((l) => {
                  const proj = findPast(l.pastProjectId);
                  return proj
                    ? `<div class="linked-item"><span>${escapeHtml(proj.title)}</span><button class="btn secondary unlink-btn" data-id="${proj.id}">Unlink</button></div>`
                    : "";
                })
                .join("")
        }
      </div>
    </div>
    <div class="field">
      <label>Suggested connections (based on shared tags)</label>
      <div class="suggest-box">
        ${
          suggestions.length === 0
            ? "No matching past projects found yet — add tags above, or add past projects with overlapping tags."
            : suggestions
                .map(
                  (s) => `
          <div class="suggest-item">
            <span>${escapeHtml(s.project.title)} — <span style="color:var(--accent)">${s.overlap.map(escapeHtml).join(", ")}</span></span>
            <button class="btn primary link-btn" data-id="${s.project.id}">Link</button>
          </div>`
                )
                .join("")
        }
      </div>
    </div>
    `
        : `<p style="color:var(--muted); font-size:0.8rem;">Save this opportunity first, then reopen it to link past projects.</p>`
    }

    <div class="modal-actions">
      <div>${isEdit ? '<button class="btn danger" id="deleteBtn">Delete</button>' : ""}</div>
      <div class="right">
        <button class="btn secondary" id="cancelBtn">Cancel</button>
        <button class="btn primary" id="saveBtn">Save</button>
      </div>
    </div>
  `;
  backdrop.hidden = false;

  document.getElementById("cancelBtn").addEventListener("click", closeModal);

  if (isEdit) {
    document.getElementById("deleteBtn").addEventListener("click", () => {
      state.opportunities = state.opportunities.filter((x) => x.id !== id);
      render();
      closeModal();
    });
    modal.querySelectorAll(".link-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        o.links.push({ pastProjectId: btn.dataset.id, note: "" });
        saveData();
        openOppModal(id); // re-render modal with updated links
      });
    });
    modal.querySelectorAll(".unlink-btn").forEach((btn) => {
      btn.addEventListener("click", () => {
        o.links = o.links.filter((l) => l.pastProjectId !== btn.dataset.id);
        saveData();
        openOppModal(id);
      });
    });
  }

  document.getElementById("saveBtn").addEventListener("click", () => {
    const updated = {
      title: document.getElementById("f-title").value.trim() || "Untitled opportunity",
      faculty: document.getElementById("f-faculty").value.trim(),
      department: document.getElementById("f-department").value.trim(),
      deadline: document.getElementById("f-deadline").value,
      status: document.getElementById("f-status").value,
      description: document.getElementById("f-desc").value.trim(),
      tags: parseTags(document.getElementById("f-tags").value),
      notes: document.getElementById("f-notes").value.trim(),
    };
    if (isEdit) {
      Object.assign(o, updated);
    } else {
      state.opportunities.push({ ...o, ...updated, links: [] });
    }
    render();
    closeModal();
  });
}

function escapeAttr(str) {
  return escapeHtml(str).replace(/"/g, "&quot;");
}

// ---------- Import / export ----------

function exportData() {
  const blob = new Blob([JSON.stringify(state, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `research-projects-${new Date().toISOString().slice(0, 10)}.json`;
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}

function importData(file) {
  const reader = new FileReader();
  reader.onload = () => {
    try {
      const parsed = JSON.parse(reader.result);
      if (!parsed.pastProjects || !parsed.opportunities) throw new Error("Invalid file format");
      state = parsed;
      render();
    } catch (e) {
      alert("Could not import file: " + e.message);
    }
  };
  reader.readAsText(file);
}

// ---------- Wiring ----------

document.getElementById("addPastBtn").addEventListener("click", () => openPastModal(null));
document.getElementById("addOppBtn").addEventListener("click", () => openOppModal(null));
document.getElementById("statusFilter").addEventListener("change", render);
document.getElementById("sortSelect").addEventListener("change", render);
document.getElementById("exportBtn").addEventListener("click", exportData);
document.getElementById("importInput").addEventListener("change", (e) => {
  const file = e.target.files[0];
  if (file) importData(file);
  e.target.value = "";
});

render();
