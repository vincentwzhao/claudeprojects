const STORAGE_KEY = "calorie-tracker.entries";

const foodOptions = document.getElementById("food-options");
const entryNameInput = document.getElementById("entry-name");
const entryTypeInput = document.getElementById("entry-type");
const entryDateInput = document.getElementById("entry-date");
const entryTimeInput = document.getElementById("entry-time");

const ingredientForm = document.getElementById("ingredient-form");
const ingNameInput = document.getElementById("ing-name");
const ingQtyInput = document.getElementById("ing-qty");
const ingQtyLabel = document.getElementById("ing-qty-label");
const ingKcalInput = document.getElementById("ing-kcal");
const ingKcalLabel = document.getElementById("ing-kcal-label");

const ingredientListEl = document.getElementById("ingredient-list");
const builderTotal = document.getElementById("builder-total");
const builderTotalKcal = document.getElementById("builder-total-kcal");
const saveEntryBtn = document.getElementById("save-entry");

const historyEl = document.getElementById("history");
const emptyState = document.getElementById("empty-state");
const todayKcalEl = document.getElementById("today-kcal");
const todayBreakdownEl = document.getElementById("today-breakdown");

let draftIngredients = [];

function todayStr() {
  const d = new Date();
  return d.toISOString().slice(0, 10);
}

function nowStr() {
  const d = new Date();
  return `${String(d.getHours()).padStart(2, "0")}:${String(d.getMinutes()).padStart(2, "0")}`;
}

function loadEntries() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw ? JSON.parse(raw) : [];
  } catch {
    return [];
  }
}

function saveEntries(entries) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(entries));
}

function entryTotal(entry) {
  return entry.ingredients.reduce((sum, i) => sum + i.calories, 0);
}

function escapeHtml(str) {
  const div = document.createElement("div");
  div.textContent = str;
  return div.innerHTML;
}

function formatDateHeading(dateStr) {
  const d = new Date(dateStr + "T00:00:00");
  const today = todayStr();
  const yesterday = new Date();
  yesterday.setDate(yesterday.getDate() - 1);
  const yesterdayStr = yesterday.toISOString().slice(0, 10);

  if (dateStr === today) return "Today";
  if (dateStr === yesterdayStr) return "Yesterday";
  return d.toLocaleDateString(undefined, { weekday: "long", month: "short", day: "numeric", year: "numeric" });
}

function formatTime(timeStr) {
  const [h, m] = timeStr.split(":").map(Number);
  const d = new Date();
  d.setHours(h, m);
  return d.toLocaleTimeString(undefined, { hour: "numeric", minute: "2-digit" });
}

// ---- food lookup / autocomplete ----

function findFood(name) {
  const target = name.trim().toLowerCase();
  return FOOD_DB.find((f) => f.name.toLowerCase() === target);
}

function populateFoodOptions() {
  foodOptions.innerHTML = FOOD_DB.map((f) => `<option value="${escapeHtml(f.name)}"></option>`).join("");
}

function updateIngredientFormForFood(food) {
  if (food.unit === "each") {
    ingQtyLabel.textContent = "Quantity (count)";
    ingKcalLabel.textContent = "Calories (per unit)";
    ingQtyInput.value = 1;
  } else {
    ingQtyLabel.textContent = "Quantity (g)";
    ingKcalLabel.textContent = "Calories (per 100 g)";
    ingQtyInput.value = 100;
  }
  ingKcalInput.value = food.kcal;
}

ingNameInput.addEventListener("input", () => {
  const food = findFood(ingNameInput.value);
  if (food) updateIngredientFormForFood(food);
});

// ---- builder: current draft ingredients ----

function renderDraft() {
  ingredientListEl.innerHTML = "";
  draftIngredients.forEach((ing, idx) => {
    const li = document.createElement("li");
    li.className = "ingredient-item";
    li.innerHTML = `
      <span class="ing-name">${escapeHtml(ing.name)}</span>
      <span class="ing-qty">${ing.quantity} ${ing.unit === "each" ? "×" : "g"}</span>
      <span class="ing-kcal">${Math.round(ing.calories)} kcal</span>
      <button type="button" data-idx="${idx}" class="remove-ing" aria-label="Remove">&times;</button>
    `;
    ingredientListEl.appendChild(li);
  });

  const total = draftIngredients.reduce((sum, i) => sum + i.calories, 0);
  builderTotal.hidden = draftIngredients.length === 0;
  builderTotalKcal.textContent = `${Math.round(total)} kcal`;
  saveEntryBtn.disabled = draftIngredients.length === 0;
}

ingredientListEl.addEventListener("click", (e) => {
  const btn = e.target.closest(".remove-ing");
  if (!btn) return;
  draftIngredients.splice(Number(btn.dataset.idx), 1);
  renderDraft();
});

ingredientForm.addEventListener("submit", (e) => {
  e.preventDefault();
  const name = ingNameInput.value.trim();
  if (!name) return;

  const known = findFood(name);
  const unit = known ? known.unit : ingQtyLabel.textContent.includes("count") ? "each" : "g";
  const quantity = Number(ingQtyInput.value) || 0;
  const kcalPer = Number(ingKcalInput.value) || 0;
  const baseAmount = unit === "each" ? 1 : 100;
  const calories = (kcalPer / baseAmount) * quantity;

  draftIngredients.push({ name, unit, quantity, calories });
  renderDraft();

  ingredientForm.reset();
  ingQtyLabel.textContent = "Quantity (g)";
  ingKcalLabel.textContent = "Calories (per 100 g)";
  ingQtyInput.value = 100;
  ingNameInput.focus();
});

saveEntryBtn.addEventListener("click", () => {
  if (draftIngredients.length === 0) return;

  const type = entryTypeInput.value;
  const name = entryNameInput.value.trim() || (type === "meal" ? "Meal" : "Snack");

  const entries = loadEntries();
  entries.push({
    id: crypto.randomUUID(),
    type,
    name,
    date: entryDateInput.value || todayStr(),
    time: entryTimeInput.value || nowStr(),
    ingredients: draftIngredients,
  });
  saveEntries(entries);

  draftIngredients = [];
  renderDraft();
  entryNameInput.value = "";
  entryDateInput.value = todayStr();
  entryTimeInput.value = nowStr();

  render();
});

// ---- history rendering ----

function render() {
  const entries = loadEntries();
  emptyState.hidden = entries.length > 0;

  const byDate = {};
  for (const entry of entries) {
    (byDate[entry.date] ??= []).push(entry);
  }
  const dates = Object.keys(byDate).sort((a, b) => (a < b ? 1 : -1));

  historyEl.innerHTML = "";
  for (const date of dates) {
    const dayEntries = byDate[date].sort((a, b) => (a.time < b.time ? 1 : -1));
    const dayTotal = dayEntries.reduce((sum, e) => sum + entryTotal(e), 0);

    const dayEl = document.createElement("div");
    dayEl.className = "day-group";
    dayEl.innerHTML = `
      <div class="day-heading">
        <span>${formatDateHeading(date)}</span>
        <span class="day-total">${Math.round(dayTotal)} kcal</span>
      </div>
    `;

    const ul = document.createElement("ul");
    ul.className = "entry-list";
    for (const entry of dayEntries) {
      const total = entryTotal(entry);
      const li = document.createElement("li");
      li.className = "entry-item";
      li.innerHTML = `
        <div class="entry-row">
          <span class="entry-badge badge-${entry.type}">${entry.type === "meal" ? "Meal" : "Snack"}</span>
          <span class="entry-name">${escapeHtml(entry.name)}</span>
          <span class="entry-time">${formatTime(entry.time)}</span>
          <span class="entry-kcal">${Math.round(total)} kcal</span>
          <button type="button" class="toggle-details" data-id="${entry.id}">Details</button>
          <button type="button" class="delete-entry" data-id="${entry.id}" aria-label="Delete">&times;</button>
        </div>
        <ul class="entry-ingredients" id="details-${entry.id}" hidden>
          ${entry.ingredients
            .map(
              (i) =>
                `<li>${escapeHtml(i.name)} — ${i.quantity}${i.unit === "each" ? "×" : "g"} (${Math.round(i.calories)} kcal)</li>`
            )
            .join("")}
        </ul>
      `;
      ul.appendChild(li);
    }
    dayEl.appendChild(ul);
    historyEl.appendChild(dayEl);
  }

  // today summary
  const today = todayStr();
  const todayEntries = entries.filter((e) => e.date === today);
  const todayTotal = todayEntries.reduce((sum, e) => sum + entryTotal(e), 0);
  todayKcalEl.textContent = `${Math.round(todayTotal)} kcal`;
  const meals = todayEntries.filter((e) => e.type === "meal").length;
  const snacks = todayEntries.filter((e) => e.type === "snack").length;
  todayBreakdownEl.textContent = todayEntries.length
    ? `${meals} meal${meals === 1 ? "" : "s"} · ${snacks} snack${snacks === 1 ? "" : "s"}`
    : "Nothing logged yet";
}

historyEl.addEventListener("click", (e) => {
  const toggleBtn = e.target.closest(".toggle-details");
  if (toggleBtn) {
    const details = document.getElementById(`details-${toggleBtn.dataset.id}`);
    details.hidden = !details.hidden;
    return;
  }

  const delBtn = e.target.closest(".delete-entry");
  if (delBtn) {
    let entries = loadEntries();
    entries = entries.filter((e) => e.id !== delBtn.dataset.id);
    saveEntries(entries);
    render();
  }
});

// ---- init ----

populateFoodOptions();
entryDateInput.value = todayStr();
entryTimeInput.value = nowStr();
render();
