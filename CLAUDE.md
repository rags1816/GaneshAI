# GaneshAI / GanapatiAI

An ESP32-based physical altar device (OLED display, I2S amp, LEDs, touch
sensors, wish pad) paired with a Firebase backend (Claude blessing
generation + Google TTS) and two browser-facing pages (admin dashboard,
devotee puja/offering form).

## Version history

`config.h`'s FIRMWARE_VERSION is the source of truth for what's on the
board (see the Firmware section below) - this list is a running summary
of what each recent version actually changed and why, kept here so the
reasoning survives even when the git log scrolls out of context.

- **r86** - Reverb DSP (Schroeder comb+allpass) added to generateBlessing/
  synthesizeAudio server-side; deeper/slower voice; sentiment-aware LED
  mood via X-Blessing-Mood header.
- **r87** - Hardened mood parsing (two different wishes were getting the
  same LED color).
- **r88** - Correctly-shaped Indic script rendering on the OLED via
  `@napi-rs/canvas` (renderTextImage Cloud Function) - never successfully
  deployed until the r93/r94 debugging session found the real
  package.json drift blocking it.
- **r89** - Added an `AMP: connecting...` log line before the TLS
  handshake/POST in postAndStreamAudioToAmp() - the silence in that gap
  (several seconds, real on ESP32) was being misread as a hang.
- **r90** - Raised the phone-side AI-blessing-upgrade timeout 15s->25s;
  r86's reverb DSP added real latency the old timeout never accounted
  for, so offerings were timing out before the translated/personalized
  text arrived and got stuck showing raw text permanently.
- **r91** - `WISH_PAD_CONNECTED` flipped false->true in config.h - the
  pad's own onboard LED lighting on touch is independent hardware; the
  firmware was still gated off and never read the pin at all.
- **r92** - Wish pad now speaks in the dashboard's selected language
  instead of always English.
- **r93** - Fixed the hardcoded "Puja page: r88" footer label that
  didn't update when r90 shipped; added this file.
- **r94** - Fixed `blessingTaskActive` getting stuck `true` forever if
  the amp's background network call hung (a real ESP32
  WiFiClientSecure/TLS risk) - every future offering/wish-pad touch
  would silently skip the spoken blessing (bell still rang, unrelated
  flag) until a physical reboot. Now self-heals after 120s via
  checkBlessingTaskHealth(), polled from loop().
- **r95** - Fixed a real double-approval race: renderQueue()'s 4s relay
  poll could redraw an already-tapped Approve button before that tap's
  own relay write landed, inviting a second tap that fired a real second
  bell+blessing on the physical device. Also gave Marathi its own
  language code (3) instead of sharing code 1 with Sanskrit/Hindi -
  selecting Marathi on the dashboard was silently playing the wish pad's
  blessing in Hindi instead. (Punjabi was never a wish-pad option at
  all - it only exists on puja.html's separate 9-language per-offering
  picker, unrelated to the wish pad's 4-option dashboard setting.)

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
