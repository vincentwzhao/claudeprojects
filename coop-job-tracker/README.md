# Co-op Job Application Tracker

A small, dependency-free web app for tracking co-op/internship job
applications, with a focus on how many of them are CS-related.

## Features

- Log each application: company, role, date applied, category
  (CS-related vs. non-CS), status, and notes/link.
- Dashboard stats: total applications, CS-related count, % CS-related,
  interviews, and offers.
- Status breakdown bar chart (Applied, Online Assessment, Interview,
  Offer, Rejected, Ghosted).
- Filter the applications table by category and status.
- Edit or delete any entry.
- Export everything to CSV.

## Running it

No build step or server required — it's plain HTML/CSS/JS.

```bash
cd coop-job-tracker
python3 -m http.server 8080
```

Then open `http://localhost:8080` in your browser. (Opening `index.html`
directly also works in most browsers.)

## Data storage

All data is stored in your browser's `localStorage` (key
`coopJobApplications`) — nothing leaves your machine, and there's no
backend. Data persists across reloads but is local to whichever browser
you use, so use the **Export CSV** button if you want a backup or want to
move data between browsers/devices.
