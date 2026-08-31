# Book Tracker

A single-file web app for tracking books and audiobooks read across
Audible, Libby, physical copies, or anywhere else.

## Usage

Open `index.html` in a browser (double-click it, or serve it with any
static file server). No build step, no backend, no dependencies.

- Add a book with title, author, source (Audible/Libby/Physical/Kindle/
  Spotify/Other), format, status (finished/reading/want to read), a
  finished date, a 0–5 star rating, and notes.
- Filter by status with the tabs, or search across title/author/notes.
- Stats at the top show total finished, finished this year, currently
  reading, and your most-used source.
- **Export JSON** downloads a backup of your library; **Import JSON**
  merges an exported file back in.

## Data storage

Books are stored in the browser's `localStorage`, scoped to wherever you
open `index.html` from. That means data is per-browser and per-origin —
it won't sync across devices or browsers on its own. Use Export/Import to
move your library or keep a backup.
