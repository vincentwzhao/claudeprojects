# Research Project Manager

A small client-side tool for tracking SFU Undergraduate Research Explorers
opportunities (or any research project you're considering) and linking them
to research/coursework you've already done.

## Usage

Open `index.html` in a browser — no build step or server required. All data
is stored locally in your browser (`localStorage`), so use the **Export
data** / **Import data** buttons to back up or move your data between
browsers/devices.

- **Past Projects**: things you've already done — research, coursework,
  personal projects. Give each one comma-separated tags describing its
  skills/topics (e.g. `networking, python, verification`).
- **Opportunities**: projects you're considering applying to (e.g. listings
  from the Call for Undergraduate Research Explorers). Track faculty
  contact, department, deadline, and application status.
- **Connections**: when editing an opportunity, the app suggests past
  projects whose tags overlap with the opportunity's tags, and you can link
  them with one click. Linked projects show up on both cards, so you can
  see at a glance which past experience supports which application — useful
  when writing your statement of interest.

## Seed data

The app ships with one real opportunity already entered — "Verification in
Intent-Driven Network Management Automation" — transcribed from the SFU
Canvas Call for Undergraduate Research Explorers page. Add the remaining
listings from that page the same way (click **+ Add opportunity**), and fill
in your own past projects to start getting connection suggestions.
