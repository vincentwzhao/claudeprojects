# Cancel Reminder

A tiny, dependency-free web app that tracks your subscriptions and tells you
when it's time to cancel them — before they renew and charge you again.

## Usage

Open `index.html` directly in a browser, or serve the folder with any static
file server:

```bash
cd subscription-cancel-reminder
python3 -m http.server 8080
# then visit http://localhost:8080
```

For each subscription, add its name, cost, billing cycle, next renewal date,
and how many days ahead you want to be warned. The app:

- Sorts subscriptions by urgency and highlights anything overdue or due
  within your reminder window in red, with a banner at the top.
- Sends a browser notification (if you grant permission) when something
  needs cancelling.
- Lets you mark a subscription **Renewed** (rolls the renewal date forward
  one billing cycle) or **Cancelled** (removes it).

All data is stored in your browser's `localStorage` — nothing leaves your
machine, and there's no backend or account required.
