# GaneshAI / GanapatiAI

An ESP32-based physical altar device (OLED display, I2S amp, LEDs, touch
sensors, wish pad) paired with a Firebase backend (Claude blessing
generation + Google TTS) and two browser-facing pages (admin dashboard,
devotee puja/offering form).

## Duplicated files - THE #1 SOURCE OF BUGS IN THIS REPO

Several pages exist as multiple near-identical copies because the same
HTML/JS has to be served from more than one place (GitHub Pages, Firebase
Hosting, and embedded directly in the ESP32 firmware as a C string). None
of this is auto-generated - every copy must be edited by hand, every time.
Forgetting one is the single most common bug in this project's history
(confirmed repeatedly: puja.html's AI-blessing timeout fix (r90) initially
missed two of its three copies; index.html drifted ~15 commits behind
web_dashboard.h before anyone noticed offerings had stopped displaying).

**Puja/offering form - 3 copies, must always match:**
- `puja.html` (repo root) - served by GitHub Pages at
  `rags1816.github.io/GaneshAI/puja.html`. Auto-deploys on push to `main`.
- `backend/hosting/puja.html` - served by Firebase Hosting at
  `ganapatiai.web.app/puja.html`. Requires a manual deploy, does NOT
  auto-deploy on git push:
  `cd backend && firebase deploy --only hosting --project ganapatiai`
- `firmware/puja_page.h` - same HTML/JS embedded as a C string
  (`const char PUJA_HTML[] PROGMEM = R"rawliteral(...)rawliteral";`)
  inside the `#ifndef PUJA_PAGE_H` guard, served directly by the ESP32
  over its own WiFi. Requires a physical reflash to take effect.

All three have an identical hardcoded footer label,
`Puja page: YYYY-MM-DD-rNN` - it is a plain string, NOT derived from any
version constant, so it will silently go stale unless bumped by hand in
all three files on every change to any of them.

**Admin dashboard - 2 copies, must always match:**
- `index.html` (repo root) - served by GitHub Pages at
  `rags1816.github.io/GaneshAI/`. Auto-deploys on push to `main`.
- `firmware/web_dashboard.h` - same HTML/JS embedded as
  `const char INDEX_HTML[] PROGMEM = R"rawliteral(...)rawliteral";`
  inside `#ifndef WEB_DASHBOARD_H`, served by the ESP32. Requires reflash.

To resync `index.html` from `web_dashboard.h` after a firmware-only edit,
extract the literal's contents (from the line after `R"rawliteral(` to the
line before the matching `)rawliteral";`) and write it verbatim as
`index.html` - that's the whole relationship, no other transformation.

**When changing the puja form or dashboard: edit all copies in the same
commit. Don't treat the firmware one as canonical and the web ones as an
afterthought, or vice versa - they need to be identical.**

## Backend (Cloud Functions)

- `backend/functions/index.js` - `generateBlessing`, `synthesizeAudio`,
  `renderTextImage`. Requires a manual deploy, does NOT auto-deploy on git
  push: `cd backend && firebase deploy --only functions --project ganapatiai`
- Firebase/GCP project ID: `ganapatiai`.
- If a dependency is added to `backend/functions/package.json` (e.g. r88
  added `@napi-rs/canvas`), every local clone/download needs a fresh
  `npm install` in `backend/functions` before it will deploy - a stale
  `package.json` makes `npm install` silently report "up to date" while
  actually installing nothing, which then crashes the deploy's source
  analysis step with no useful error message.
- Shared offering queue lives in Firebase Realtime Database at
  `https://ganapatiai-default-rtdb.europe-west1.firebasedatabase.app/ganesha_queue.json`
  (public read/write, plain JSON, no auth) - both puja pages and the
  dashboard read/write it directly from the browser. Capped at 5 items
  client-side (`puja.html`'s `proceedWithOffering()`) to fit an IIS path
  length limit; a full queue silently blocks new submissions until a
  priest approves/rejects existing ones from a dashboard, which is what
  actually removes items from it.

## Firmware

- `firmware/GanapatiAI.ino` + `firmware/config.h` +
  (embedded) `firmware/puja_page.h` + `firmware/web_dashboard.h` +
  `firmware/indic_fonts.h` - the whole Arduino sketch, all 5 files
  required together.
- `config.h`'s `FIRMWARE_VERSION` string must be bumped on every change to
  any of the 5 firmware files - it's what the boot log and dashboard
  report, and is the only reliable way to confirm what's actually flashed
  versus what's merely been downloaded.
- No static IP - boards get whatever DHCP assigns, which can change
  across reflashes/reboots. The current IP is always shown on the
  physical OLED at boot and printed to Serial Monitor.

## User's local setup (not synced via git - manual file drops)

- ESP32 flash folder: `C:\Users\DELL8\OneDrive\Desktop\Ganapathi\GanapatiAI`
  (the 5 firmware files only).
- Firebase deploy folder: `C:\Users\DELL8\Downloads\GaneshAI\backend`
  (a full manual download of `backend/`, not a git clone - needs each
  changed file re-downloaded by hand, it will not update itself).
- Neither location is a git checkout. When a fix touches firmware or
  backend files, the user needs the actual updated files delivered to
  them (not just told "it's fixed in the repo") before they can act on it.
