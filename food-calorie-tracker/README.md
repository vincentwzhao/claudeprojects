# Calorie Tracker

A tiny, dependency-free web app for logging meals and snacks by ingredient.
Calories are computed automatically from each ingredient's amount, and totals
roll up per entry and per day.

## Usage

Open `index.html` directly in a browser, or serve the folder with any static
file server:

```bash
cd food-calorie-tracker
python3 -m http.server 8080
# then visit http://localhost:8080
```

To log an entry:

1. Fill in the entry's name (e.g. "Breakfast"), whether it's a **meal** or
   **snack**, and the date/time (defaults to now).
2. Add ingredients one at a time. Start typing a food name to autocomplete
   against the built-in database (`data.js`, ~90 common foods) — its
   calories and unit (grams or count, e.g. "1 egg") fill in automatically.
   For anything not in the database, just type a name and enter the
   calories and quantity yourself.
3. Each added ingredient appears in a running list with its calorie
   contribution; the entry's total updates live.
4. Click **Save entry** to add it to your history.

The **History** section groups saved entries by day, with a running total
per day and expandable ingredient breakdowns per entry. The banner at the
top always shows today's running total.

All data is stored in your browser's `localStorage` — nothing leaves your
machine, and there's no backend or account required.

## Extending the food database

Add entries to the `FOOD_DB` array in `data.js`:

```js
{ name: "Food name", kcal: 123, unit: "g" }     // kcal per 100 g
{ name: "Food name", kcal: 123, unit: "each" }  // kcal per one unit (egg, slice, tbsp, ...)
```
