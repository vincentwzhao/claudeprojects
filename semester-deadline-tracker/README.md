# Semester Deadline Tracker

A small standalone tracker for the payment and academic deadlines leading
into a semester (built against [SFU's Fall Dates & Deadlines page](https://www.sfu.ca/students/deadlines/fall.html)).

Open `index.html` in a browser — no build step or server required.

## Features

- Countdown to the next deadline, prioritizing payment deadlines
- Add / check off / delete deadlines, categorized as Payment, Enrollment,
  Academic, or Other
- Color-coded urgency (red ≤3 days, amber ≤10 days, green further out)
- Upcoming vs. past sections
- State persists in the browser via `localStorage`
- "Reset to default SFU dates" restores the seeded defaults

## Seeded data

This environment couldn't reach `sfu.ca` directly, so only the following
dates (confirmed via search) are pre-loaded:

| Deadline | Date |
| --- | --- |
| Fall term classes begin | Sep 9, 2026 |
| Last day to add/drop courses without penalty | Sep 18, 2026 |
| Tuition payment due | Sep 22, 2026 |
| Last day of fall exams | Dec 20, 2026 |

Check the official page for anything not listed here — e.g. the
withdrawal-with-W deadline, refund-tier cutoffs (100%/75%/50%), and the
RTW (refund/transcript withdrawal) date — and add them with the form at
the top of the page.
