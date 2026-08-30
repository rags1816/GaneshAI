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
- **r96** - Added a BACK PAD RAW diagnostic (same pattern as WISH PAD
  RAW) after the mouse-back pad produced zero TOUCH lines across 14s of
  continuous touching - both TOUCH_BACK_CONNECTED and touchBackEnabled
  were already true, and wiring was confirmed physically correct
  (D23, not the old D15), so this was needed to see what was actually
  happening on the pin itself.
- **r97** - Root cause of the above, and of a separate "wish pad fires
  with nobody touching it" report: all three touch pins (feet/back/
  wish) were plain high-impedance INPUT with no pull resistor at all.
  Switched to INPUT_PULLDOWN. This fixed BOTH symptoms from the same
  underlying cause - a floating/marginal connection doesn't just pick
  up noise and misfire (the wish pad's symptom), it can also sit in an
  indeterminate state that never cleanly registers a real touch (the
  mouse-back pad's symptom) - confirmed on hardware immediately after
  reflashing: mouse-back went from total silence to working end-to-end
  (touch detected, bell, mantra audible) with no rewiring needed at all.
- **r98** - Removed the WISH PAD RAW / BACK PAD RAW once-a-second Serial
  diagnostics now that r97 confirmed both pads fixed - both were
  explicitly marked "remove once confirmed working" in their own
  comments, and were flooding the monitor, making real TOUCH/WAKE lines
  hard to spot while testing.
- **r99** - Fixed a real audio overlap, not hypothetical: a fresh feet/
  back/wish-pad touch could immediately start a new mantra or bell right
  on top of an already-playing spoken blessing (two different speakers -
  DFPlayer vs the I2S amp - so nothing native stopped them colliding).
  Confirmed on hardware via exact log timestamps. All three touch
  triggers now check blessingTaskActive first and drop the touch while
  a blessing is actively speaking, rather than queuing or interrupting it.
- **r100** - Added a WISH PAD RAW EDGE diagnostic (event-triggered, not
  periodic like the old removed one) after the dashboard's Wish Pad
  toggle was confirmed on and the module's own LED was confirmed
  lighting on a real touch, yet nothing reached the firmware at all -
  needed to see whether GPIO4 was actually moving on the ESP32 side.
  Root cause turned out to be physical: a bad connector contact between
  the JST connector and the sensor module (confirmed by the user after
  reseating/fixing the connection - no further firmware change needed).
- **r101** - Added real Noto Sans <Script> webfonts (Devanagari, Tamil,
  Telugu, Gujarati, Gurmukhi, Malayalam, Bengali) to all 3 puja page
  copies, matching the fonts already used for the OLED's Indic
  rendering - puja.html previously only declared Latin system fonts.
- **r102** - Added Urdu, Thai, and Mandarin Chinese as full languages
  (voice + OLED script rendering + web fonts + RTL handling for Urdu).
  Google has no ur-PK voice at all - only ur-IN exists, confirmed
  against Google's own current voice list, so one India-recorded voice
  covers both India and Pakistan. Along the way, found and fixed a
  real pre-existing gap: Farsi and Sindhi have been in the backend's
  LANGUAGE_CONFIG for a while but were NEVER in SCRIPT_FONTS at all -
  their OLED rendering had been silently falling back to plain text
  with zero Arabic-script glyphs available this whole time, not just
  lower quality. Fixed for all three (Urdu/Farsi/Sindhi) at once,
  including setting ctx.direction = "rtl" in renderTextToXbm(), which
  had never been set for any right-to-left language before this.
- **r103** - Real bug in r102, found on hardware within minutes of going
  live: Chirp3-HD voices (Telugu/Urdu/Thai/Mandarin) reject Google TTS's
  pitch parameter entirely - every request for those 4 languages was
  failing with HTTP 502. synthesizeSpeech() now only sends pitch for
  non-Chirp voices; speakingRate is still applied to all of them. Also
  made postAndStreamAudioToAmp() log the actual response body on a
  non-200, not just the status code - this diagnosis had to be guessed
  from a bare "502" since the firmware was discarding the real error
  text the whole time.
- **r104** - Added Malay (Bahasa Melayu, ms-MY-Wavenet-B) - Latin
  script, so unlike every other language added tonight this needed no
  new OLED font work at all, and no Chirp3-HD tier exists for it either
  so r103's pitch fix doesn't apply. Also strengthened the blessing
  prompt to explicitly forbid mixing English words into a non-English
  reply (only the devotee's own name may stay in its original script) -
  a response to Thai/Chinese blessings reportedly containing English,
  though unconfirmed whether that was genuine code-switching or just
  the devotee's name staying untransliterated.
- **r105** - Mouse-back pad now plays one fixed chant ("Ganapati Bappa
  Morya, Mangala Murti Morya" + a short Vakratunda Maha Kaya, ~30s -
  MOUSE_CHANT_TRACK/_DURATION in config.h, track 17) on every touch,
  instead of rotating through mantraTracks[] like it used to. Feet
  touch and the wish pad are both untouched - feetStep still rotates
  the shared playlist exactly as before; mouseStep (now unused) was
  removed. Track 17 doesn't exist on the SD card yet - the user still
  needs to record/source the actual audio and place it as 0017.mp3 in
  the DFPlayer's MP3 folder; duration is their own ~30s estimate,
  needs re-measuring via /api/test?track=17 once the real file is on
  the card.
- **r106** - `OLED_MODEL` switched to `OLED_SSD1309_I2C` now that the
  2.4" panel is physically reinstalled and confirmed working by the
  user (the earlier lock to the 1.3" SH1106 driver dated back to r33,
  when the 2.4" never lit up on hardware - see r94-era archaeology).
  Also reserved `EYE_LED_PIN` (GPIO16, printed "RX2" on the board) and
  `EYE_LED_CONNECTED` (false) for the new fiber-optic mouse-eye LEDs:
  two high-intensity 5mm LEDs at the altar base, each coupled to its
  own 2mm end-glow fiber running into one eye, sharing this one GPIO
  through their own 120ohm resistors. Not yet wired into the sketch's
  state machine - the user is bench-testing the LED+fiber circuit on
  the breadboard first; the real digitalWrite() logic (planned to tie
  into the mouse-back chant) comes once that test confirms brightness
  through the real fiber length.
- **r107** - Wish pad's silent-touch blessing now gets its LED mood from
  Claude self-classifying its own generated blessing (the same `[mood]`
  tag every other path already uses), instead of a fixed theme->mood
  lookup table decided before Claude ever wrote anything. The old table
  (`THEME_TO_MOOD`) is kept only as a fallback if Claude ever omits the
  tag, in place of the generic "peaceful" every other path falls back
  to. Also gave Chinese its own larger OLED render size (30px vs the
  shared 24px) - confirmed on hardware as legible on the dashboard but
  not on the physical display, unlike Bengali/Thai/Urdu on the same
  redeploy; Chinese's logographic glyphs (15+ strokes each) need more
  pixels than alphabetic/abugida scripts do to stay readable on a 1-bit
  monochrome bitmap. A real bug was caught and fixed while wiring this
  in: renderTextToXbm()'s returned height had been left pointing at the
  old fixed constant even after the canvas itself switched to the
  per-language size, which would have told the firmware the image was
  6 rows shorter than the data actually packed for it.
- **r108** - Mouse eye LEDs (fiber-optic, GPIO16/RX2, bench-tested and
  confirmed as Green + 100ohm) wired into the sketch's actual state
  machine, ahead of the idol build finishing, at the user's explicit
  request. `EYE_LED_CONNECTED` flipped true; `setSystemState()` now
  turns the pin off unconditionally on every single state change
  (comment there: "the one place that always knows a transition just
  happened"), and `triggerMantra()` (the mouse-back touch handler) is
  the only place that turns it back on, right after its own call into
  setSystemState() - so it lights for exactly the ~30s the mouse-back
  chant plays, and nothing else (feet touch, offerings, the wish pad,
  Aarti, or the temple-reopening welcome mantra - which also lands in
  STATE_MANTRA_ACTIVE via openTempleFromClosed() - never light it).
  Chosen over scattering an off-call into every individual trigger
  function specifically to be robust against interruption paths without
  having to enumerate them all by hand.
- **r109** - Two real regressions caught on hardware right after r107
  shipped. (1) The wish pad's self-classified mood (r107) skewed hard
  toward "hopeful" regardless of theme - reverted to the deterministic
  theme->mood table for touchOnly specifically (kept self-classification
  for every other path, which wasn't reported as wrong): a silent touch
  has no real devotee sentiment behind the theme WE randomly assigned
  it, so there was nothing genuine for Claude to read, unlike a typed
  prayer. (2) The 6 mood LED colors weren't visually distinct enough -
  hopeful and peaceful were both blue-family, joyful and grateful were
  both gold-family - so two different, correctly-classified moods could
  look like no change at all on the ring. Redesigned all 6 to anchor on
  a distinct hue family (gold/magenta, teal, purple, green, red/orange,
  warm amber-white).
- **r110** - V2 Phase 1: offline blessing fallback + local variation, per
  the reviewed V2 spec and the follow-up developer feedback on it (both
  agreed this should be top priority - it protects the core devotional
  experience from internet dependency, not just "resilience"). Every
  path that speaks a live AI blessing (`speakBlessingOnAmp()`,
  `speakGenericBlessingOnAmp()` for the wish pad,
  `speakOfferingFallbackBlessingOnAmp()`) now falls back to
  `playOfflineBlessingFallback()` whenever `postAndStreamAudioToAmp()`
  couldn't play any real audio at all - a small rotation of pre-recorded
  generic chants (`OFFLINE_BLESSING_TRACKS` in config.h, tracks 18-20,
  not yet on the SD card - user still needs to record/source them, same
  as r105's mouse chant) instead of leaving the temple silent.
  `postAndStreamAudioToAmp()` changed from void to bool return
  (true = some real audio was actually heard, including a stream that
  stalled AFTER genuine playback started - only a total failure before
  any audio played triggers the fallback, so a devotee never hears two
  different blessings back to back). The fallback also picks a local
  mood (random from the same 6, same reasoning as the wish pad's
  deterministic pick - no real sentiment for anything to read) and a
  local OLED phrase (reusing the existing oledChildBlessingsList/
  oledAdultBlessingsList arrays, same pattern as a plain mantra touch),
  so the devotee gets a complete, varied, real experience even fully
  offline - not just audio. Testable via `/api/test?offline=1` without
  needing to actually kill Wi-Fi. offlineFallbackCount is exposed on
  `/api/state` now, ahead of the Phase 5 dashboard health panel that
  will actually surface it.
- **r111** - `OFFLINE_BLESSING_TRACKS` (r110) changed from 3 new,
  not-yet-recorded track numbers (18-20) to 3 of the existing
  mantraTracks[] entries already on the SD card (1, 2, 4 - the
  shortest ones, closest in length to a typical spoken blessing) -
  recording 3 new dedicated chants was real friction for the user with
  no real benefit over reusing what's already there. No SD card changes
  needed at all now; the fallback works immediately on reflash.
- **r112** - r111 reverted: a mantra/chant is a genre mismatch for a
  fallback meant to acknowledge a devotee's silent prayer, not just
  lower quality than a dedicated recording. Fixed properly instead -
  tracks 18-20 are real spoken blessings, generated ONCE via the
  existing synthesizeAudio endpoint (same Google TTS voice as every
  live blessing) rather than either a human recording or a repurposed
  chant. User needs to run the 3 generation calls once and place the
  resulting files as 0018/19/20.mp3.
- **r113** - V2 Phase 4: `ExperienceScene` struct (config.h) +
  `getCurrentScene()` (GanapatiAI.ino) - a small, read-only snapshot of
  state/mood/audio/display, so new code (starting with the next phase's
  LED scene engine) has one thing to query instead of checking several
  scattered globals separately. Deliberately additive, not a refactor -
  reads existing globals (currentMoodTag, blessingTaskActive,
  feetDisplayLocked, stateTimer/stateDuration) without changing how or
  when any of them get set, so none of the ~15 interacting timers this
  file already depends on were touched. Exposed at `/api/scene` for
  manual verification, same testability convention as every other
  `/api/*` route. This is the scoped-down version of the reviewed V2
  spec's "AI Ritual Orchestrator" - a struct and a function, not a class
  hierarchy or event-driven architecture, which was reviewed and
  rejected as over-engineered for a single ESP32 running one state
  machine.
- **r114** - V2 Phase 3 (first slice): a "Closure" LED scene phase for
  mood-driven blessings - `playMoodClosurePulse()` plays one gentle
  brightness pulse in the blessing's own mood color (rises then eases
  back to dark, ~400ms) right before the existing generic
  `playLedClosingSweep()` fade, called from `setSystemState()` only when
  leaving MANTRA_ACTIVE/FEET_ACTIVE with a real mood set - a plain
  mantra/feet touch (saffron/gold, no distinct mood) is untouched, same
  reasoning as why that path never picks up mood colors at all (r109).
  Deliberately scoped to just Closure for this first slice rather than
  all 4 scene phases (Recognition/Blessing/Presence/Closure) from the
  reviewed spec at once - Presence is effectively already the existing
  continuous animation, and Recognition already has a partial analog
  (the neutral saffron/gold shown while mood is still pending); adding a
  new distinct flash there risked working against the calm, non-flashy
  feel wanted for the wish pad specifically. Can add a real Recognition
  cue as a follow-up once this pulse is confirmed on hardware.
- **r115** - V2 Phase 5: Guardian-style dashboard health panel. New
  `/api/health` endpoint plus a "Device Health" section on both
  dashboard copies (`index.html`/`web_dashboard.h`) surfacing Wi-Fi
  status/RSSI, free heap, amp readiness, offline-fallback usage
  (r110-112), and the crash tracer's last-boot status - all of which
  already existed as Serial-only logging (`checkWiFiHealth()`,
  `checkHeapHealth()`, `checkBlessingTaskHealth()`'s r94 self-heal, the
  stageMagic/lastStage crash tracer) and was invisible unless someone
  happened to be watching the monitor at the right moment. Deliberately
  read-only - per the reviewed V2 feedback, this reports on existing
  conservative recovery (Wi-Fi reconnect, the 120s blessing-task self-
  heal) rather than adding any new automatic "fix" behavior; broader
  self-healing was explicitly flagged as something that can hide real
  faults instead of surfacing them. `bootCrashedLastRun`/
  `bootCrashedStage` are new globals capturing what setup()'s crash-
  check already computed locally, since stageMagic/lastStage themselves
  get overwritten moments later for the current run's own tracking.
- **r116** - V2 Phase 6: manual Temple Atmosphere toggle (Day/Evening/
  Night/Festival) on the dashboard, via `selectedAtmosphere` and a new
  `applyAtmosphere()` color wash layered on top of whatever Theme of the
  Day already picked - AMBIENT only, doesn't touch Mantra/Feet/Aarti/
  mood colors. Deliberately manual, no NTP/time-of-day automation - per
  the reviewed V2 feedback, auto-switching by clock time can surprise a
  devotee with the wrong mode after a Wi-Fi outage or wrong timezone.
  Known minor gap: the dashboard's own browser-side LED ring preview
  (`previewLeds()`) still shows the base Theme colors without the
  atmosphere wash - only the real device's ring reflects it for now.
- **r117** - V2 Phase 7: explicit intent picker on the puja pages
  (all 3 copies) - a small closed dropdown (comfort/focus/celebration/
  general), NOT automatic AI classification, per the reviewed V2
  feedback. Sent through to `generateBlessing` as `intent`, mapped
  server-side to one added sentence appended to the existing prompt
  (`INTENT_PHRASES` in index.js) - a no-op when left as "General prayer"
  or absent entirely (e.g. the wish pad's touchOnly path, which never
  sends this field). Also noticed while syncing the puja pages for this
  change: `firmware/puja_page.h` has carried some extra explanatory
  comments not present in `puja.html`/`backend/hosting/puja.html` for a
  while (pre-existing, not from tonight) - comment-only, not a
  functional difference, but worth a proper re-sync pass at some point
  per this repo's usual duplicated-file discipline.
- **r118** - Mouse eye LED changed from a flat on/off to a gentle
  breathing animation (fade in ~800ms, then a ~3s breathing cycle
  capped around 70% brightness) for the same ~30s the mouse-back chant
  plays - same single Green LED, no color change, no new hardware.
  Driven via the ESP32's LEDC PWM peripheral (`ledcSetup`/
  `ledcAttachPin`/`ledcWrite`) instead of plain `digitalWrite()`, so
  brightness can fade smoothly. `setSystemState()`'s existing
  unconditional "eyes off on every state change" now fades out over
  ~250ms instead of an instant cut, but only when breathing was
  actually active - every other transition is unaffected.
  **Real compile risk flagged, not yet confirmed on hardware:**
  `ledcSetup()`/`ledcAttachPin()` are the classic ESP32 Arduino core
  (2.x) LEDC API; if the installed core has moved to the newer 3.x API,
  these are replaced by a single `ledcAttach(pin, freq, resolution)` -
  a compile error naming `ledcSetup`/`ledcAttachPin` as undefined is the
  sign to make that swap.
- **r119** - Confirmed real: the user's installed ESP32 board package is
  3.1.3 (core 3.x), so r118's `ledcSetup()`/`ledcAttachPin()` needed the
  swap flagged in its own comment. Switched to core 3.x's single-call
  `ledcAttach(pin, freq, resolution)`, and `ledcWrite()`/`ledcRead()` now
  take the PIN number directly rather than a channel number -
  `EYE_LED_PWM_CHANNEL` no longer exists as a concept on this core,
  removed entirely rather than left unused.
- **r120 - EMERGENCY ROLLBACK** - r119 broke the live device: feet touch,
  mouse-back touch, OLED, LED ring, and mantra playback all stopped
  working after flashing it, while the wish pad (and its spoken
  blessing) kept working. `EYE_LED_CONNECTED` flipped back to `false`.
  Leading theory: `ledcAttach()` didn't succeed cleanly on GPIO16 on
  this specific board, and `setSystemState()`'s unconditional
  `ledcWrite()` on every single state change then crashed/hung the
  device on almost every interaction - matching exactly which things
  broke (everything routing through `setSystemState()`) and which
  didn't (the wish pad's flow, which doesn't route through it the same
  way). Not yet confirmed at the time - diagnose the actual PWM/GPIO16
  behavior on a SPARE board, never the live device, before attempting to
  re-enable. **Superseded by r121's finding below: this theory was
  wrong.**
- **r121** - Re-enabled `EYE_LED_CONNECTED` for a clean re-test: after
  r120's rollback, the user found the ESP32 itself was not fully seated
  in its socket, likely from handling during the r119 flash - a real,
  independent, equally-plausible explanation for r119's broad breakage
  (intermittent contact hitting multiple peripherals) separate from
  r120's ledcAttach()/GPIO16 crash theory. Testing with confirmed-solid
  seating is the only way to tell which was actually true. If the same
  breakage recurs even with solid seating, r120's theory is confirmed -
  revert to false immediately. **Confirmed on hardware immediately after
  reflashing: all pads, PIR, LED ring, OLED, DFPlayer, and the mouse eye
  breathing animation all work correctly with solid seating.** r120's
  ledcAttach()/GPIO16 crash theory was wrong - the loose board seating
  was the real and only cause of r119's breakage. The eye LED PWM code
  from r118/r119 was fine all along and needed no further changes.
- **r122** - Investigating a separate report: PIR wakes the temple (state
  moves to AMBIENT) but the bell doesn't ring, seen on both r119 and
  r120 - so not just a symptom of r119's crash. The PIR-wake branch in
  STATE_STANDBY had no Serial log at all (unlike the touch-wake branch's
  "WAKE: bell first..."), so there was no way to tell from the Serial
  Monitor whether it even runs. Added `"WAKE: PIR motion detected..."`
  right where it calls `dfPlay(BELL_TRACK)`, to pin down whether this
  branch fires at all versus firing but the bell command itself failing
  - existing PIR diagnostics (the "MOTION (line driven HIGH)" / "NOISE"
  verdict) already confirm whether the raw sensor is read correctly,
  this fills the gap after that. **Confirmed working after reflashing
  with solid board seating** - same root cause as r120/r121's broader
  investigation, not a separate PIR-specific bug.
- **r123** - Mouse eye LED changed from a flat off/breathing-pulse/off
  cycle to a dim, always-on resting glow (`EYE_LED_BASE_BRIGHTNESS`,
  ~10%) that never goes fully dark, with the existing breathing pulse
  (`EYE_LED_PEAK_BRIGHTNESS`, ~70%) rising out of and settling back into
  that resting glow specifically while the mouse-back chant plays -
  confirmed working end-to-end on real hardware, then refined per
  direct feedback that the eyes should never actually shut off.
  `setup()`'s initial write and `setSystemState()`'s per-transition
  write both changed from 0 to the base level; `updateEyeLedBreathing()`
  now oscillates between base and peak instead of 0 and peak.
- **r124** - Device Health dashboard panel (r115) expanded with MP3
  player/LED ring/PIR status - it previously only surfaced Wi-Fi/heap/
  amp/crash-tracer/offline-fallback fields, leaving three pieces of
  already-tracked state (`dfPlayerReady`, `LED_CONNECTED`/`ledEnabled`,
  `PIR_CONNECTED`/`pirEnabled`) invisible on the dashboard even though
  the firmware already knew them. `/api/health`'s JSON gained
  `dfPlayerReady`, `ledConnected`, `ledEnabled`, `pirConnected`,
  `pirEnabled` (buffer widened 512->640 bytes); both dashboard copies
  (`index.html`/`web_dashboard.h`) got matching "MP3 player" / "LED
  ring" / "PIR" rows under Device Health. No firmware behavior changed
  - purely surfacing existing state, same read-only philosophy as r115.
- **r125** - Two direct follow-up tweaks after live testing. (1) Temple
  Atmosphere (r116) reported as "no visible effect" even when watching
  the correct AMBIENT window - the wash's blend amounts (100-180 out of
  255) left Theme of the Day's base colors dominant enough to read as a
  faint tint; `applyAtmosphere()`'s Evening/Night/Festival blends all
  pushed to 200-220 so the wash color clearly leads. (2) Mouse eye LED
  resting glow raised from ~10% to 80% brightness per direct feedback
  that it read as too dim on the physical fiber runs - the breathing
  pulse during the mouse-back chant now dips DOWN from that 80% resting
  level to a 10% low point and back, instead of rising up from a dim
  10% base to a 70% peak. `EYE_LED_BASE_BRIGHTNESS` (now 204, was the
  pulse's old low end) is the resting glow AND the pulse's ceiling;
  new `EYE_LED_BREATH_LOW_BRIGHTNESS` (26) replaces the old
  `EYE_LED_PEAK_BRIGHTNESS` as the pulse's floor.
  `updateEyeLedBreathing()` rewritten to a single phase-shifted sine
  (no separate 800ms linear ramp-in) since the resting level and the
  pulse's ceiling are now the same value, so there's nothing to ramp up
  to; and `setSystemState()`'s post-chant settle now steps toward the
  base level in whichever direction is needed (previously only ever
  faded downward, which no longer matches - an interrupted chant is
  now usually caught mid-dip, below the resting level, not above it).
- **r126** - Real bug on the physical OLED's top-right state tag, found
  while investigating a direct report that a mouse-back touch showed
  "confusing" text: `STATE_FEET_ACTIVE` was falling into the exact same
  `"MANTRA"` tag as `STATE_MANTRA_ACTIVE`, so a feet touch and a
  mouse-back touch were visually indistinguishable on the physical
  display - there was never a working "FEET" tag at all. Each real touch
  source now gets its own tag: mouse-back shows "MOUSE BACK", feet shows
  "FEET". Also gave the wish pad its own "WISH" tag - it was showing
  "MANTRA" (via the same bug) then would have shown "FEET" once fixed,
  since `triggerWishPadBlessing()` deliberately reuses
  `STATE_FEET_ACTIVE`'s existing display-lock/interruption timing rather
  than adding a whole new SystemState. New `wishPadBlessingActive` flag
  (set true only by `triggerWishPadBlessing()`, cleared unconditionally
  at the top of every `setSystemState()` transition - same pattern as
  the eye LED's reset) lets the tag tell a wish-pad blessing apart from
  a real feet touch without touching the underlying shared state
  machinery. Confirmed by design, not a bug: PIR does NOT wake
  `STATE_TEMPLE_CLOSED` back to AMBIENT - only a physical touch does
  (existing code comment: "Deliberately only a touch wakes the closed
  temple - not PIR ... matching the web dashboard's TEMPLE_CLOSED
  design"), so a closed temple staying closed through motion alone is
  expected, not a fault. Known follow-up, not yet done: the dashboard's
  own "System State" text and `/api/state`'s raw state string still
  report a wish-pad blessing as `FEET_ACTIVE`, same as a real feet
  touch - fixing that fully needs `wishPadBlessingActive` threaded
  through `/api/state` and several existing `data.state === "FEET_ACTIVE"`
  checks in `web_dashboard.h` (Now Playing sync, OLED text sync) that a
  new `WISH_ACTIVE` string would otherwise silently stop matching.
- **r127** - Follow-up to r126, closing the dashboard-side half of that
  gap per direct request ("want the dashboard to sync as per principle
  with the actions done"). `/api/state` now reports a wish-pad blessing
  as `WISH_ACTIVE` instead of `FEET_ACTIVE` (gated on `wishPadBlessingActive`,
  same flag r126 added). Threaded through the specific `web_dashboard.h`/
  `index.html` checks that actually needed it to keep working correctly:
  the mood-tint and audio-reactive-breathing LED preview conditions (a
  wish blessing has a real self-classified mood, same as a feet touch),
  the idle-transition cleanup (`wasActive`/`nowIdle`, so local hums/timers
  stop correctly once a wish blessing ends), and the OLED-text content
  sync (so the dashboard's simulated OLED shows the wish pad's fixed
  "your silent prayer is heard" message instead of stale text). New
  `oledStateTag()` helper maps raw state names to the same wording as the
  real device's OLED tag (r126) - "MOUSE BACK"/"FEET"/"WISH"/"AARTI" -
  instead of the dashboard showing the raw `MANTRA_ACTIVE`/`FEET_ACTIVE`/
  `WISH_ACTIVE` strings verbatim. Deliberately did NOT touch the
  "Now Playing" library-track lookup (`data.track` matched against
  `mantraTracks[]`) - a wish-pad blessing's actual audio (bell, then a
  live-streamed TTS blessing) was never one of the DFPlayer's SD-card
  mantra tracks to begin with, so that lookup correctly stays FEET/MANTRA
  -only, unrelated to this fix. Also deliberately left alone the several
  OTHER `FEET_ACTIVE`/`MANTRA_ACTIVE` checks in `web_dashboard.h` that
  belong to the dashboard's own local button-triggered simulation
  (`changeState()` calls from its embedded Quick Blessings/offering
  panel, close-hold pad simulation, etc.) - those never fire from a
  physical wish-pad touch and don't read `/api/state` at all, so they
  were out of scope for this fix and touching them would have been
  unrelated risk.
- **r128** - Two direct follow-ups after discussing the PIR/Aarti report
  above. (1) The AMBIENT idle-timeout auto-close (`triggerAartiThenClose()`
  fired whenever nobody touched anything within `AMBIENT_TIMEOUT`, 60s)
  is removed - this device has no clock/NTP, so PIR waking AMBIENT at ANY
  time of day (a daytime passer-by, not just at night) triggered the same
  full ~4-minute closing Aarti and a closed temple every single time,
  reported directly as "too much". Closing is manual-only now: the
  dashboard's "Close Temple" button (`?action=close`) still calls
  `triggerAartiThenClose()` directly, unaffected; AMBIENT idling out on
  its own just returns to `STATE_STANDBY` like a plain idle timeout,
  ready for the next PIR trigger. Chosen over adding real time-of-day
  awareness (NTP) for the same reasoning as r116's manual-only Temple
  Atmosphere - a wrong timezone or a Wi-Fi outage at boot would silently
  misjudge "night" and auto-close (or fail to) at the wrong moment. (2)
  The mouse eye LED now flashes/breathes through the whole closing Aarti
  chant too, not just a mouse-back touch - direct request that the
  guardian's eyes react while the LED ring's existing dedicated Aarti
  flame animation (ember red -> flame orange, scripted intensity arc)
  visibly changes color around it, instead of sitting static at the
  plain resting glow. Reuses the exact same `updateEyeLedBreathing()`
  cycle as the mouse-back chant (started in `triggerAarti()` right after
  its own `setSystemState()` call, same pattern as `triggerMantra()`) -
  it just keeps repeating for `AARTI_DURATION`'s full ~4 minutes instead
  of 30s, and settles back to the resting glow via the same existing
  fade-toward-base logic once Aarti ends (to `STATE_TEMPLE_CLOSED` or
  back to `STATE_AMBIENT`). Whether the eyes should go dim/off (not just
  settle to the resting glow) specifically once `STATE_TEMPLE_CLOSED` is
  reached - and whether PIR should give a brief acknowledgment glow
  during closed without fully reopening the temple - is an open question
  raised in the same conversation, not yet decided or implemented.
- **r129** - Confirmed decision on the MP3 folder question raised in r128's
  conversation: keep one flat DFPlayer "MP3" folder (no real subfolders)
  - track NUMBER RANGES already act as the de facto category grouping
  (1-15 feet, 3 bell, 16 aarti, 17+ mouse, 18-20 offline blessings), and
  splitting into real DFPlayer folders would mean rewriting every
  `dfPlay()` call to `playFolder(folder, file)` for no functional gain.
  Direct follow-up from the same conversation: mouse-back's single fixed
  chant (r105) is now a small rotating pool, `mouseChantTracks[]`, kept
  deliberately separate from `mantraTracks[]` (feet's own shared
  playlist) per explicit request for chants "diff from feet". Reuses the
  existing `MantraTrack` struct and the same rotation pattern as
  `feetStep`, via a new `mouseStep` counter - r105 had removed `mouseStep`
  when mouse-back stopped rotating at all; r129 reintroduces it now that
  there's more than one mouse-specific chant to rotate through. Track 17
  (the original single chant) is kept as the pool's first entry; two new
  placeholder slots (21, 22 - 18-20 are already taken by the offline
  fallback tracks) are added with ~30s placeholder durations, pending the
  user recording/sourcing the actual files and re-measuring via
  `/api/test?track=21`/`22` once they're really on the SD card, same
  process as every other placeholder track this session. Add more slots
  the same way - next free track number is 23.
- **r130** - Real SD card file listing (screenshots of the actual MP3
  folder) confirmed two things and filled in the placeholders from r129.
  (1) `AARTI_TRACK`'s file (0016.mp3) was re-recorded and is now really
  3:26 (206s), not the ~4min `AARTI_DURATION`/`AARTI_FALLBACK_DURATION_MS`
  both still assumed - fixed in both config.h and web_dashboard.h/
  index.html so the closing Aarti's state timer (and the LED flame arc +
  eye breathing pulse riding on it, both r128) don't run ~34s past when
  the actual audio already finished. (2) The mouse-back pool (r129) grew
  from 3 tracks to all of 17 + 21 through 29 (10 total) once the user
  confirmed "17 and then 21 onwards is for mouse back touch" - durations
  are now the real measured lengths from the file listing (MM:SS
  resolution) instead of 30s placeholders. Flagged, not changed: track 22
  (3:59) is notably longer than every other track in this pool (12s-2:07)
  - worth the user double-checking it's the intended file, not a mix-up.
  Also flagged, not changed: mantraTracks[]'s track 14 (feet's shared
  playlist, unrelated to tonight's mouse-back/Aarti changes) has a real
  discrepancy between its firmware duration (175730ms) and the actual
  0014.mp3 file's real length (1:42/102s per the same listing) - about
  74s of silent "stuck display" per r112-era reasoning if that file was
  genuinely re-recorded shorter and firmware wasn't updated to match.
  Left alone pending the user confirming whether 0014.mp3 was
  intentionally replaced.
- **r131** - Confirmed by the user: `mantraTracks[]`'s track 14 (feet's
  shared playlist) was intentionally re-recorded shorter, same real
  length as mouseChantTracks[]'s track 25 (both 1:42/102s) - the r130
  flag was a real, deliberate change, not stale data. Duration corrected
  from 175730ms to 102000ms so feet's rotation doesn't sit on a silent
  "stuck display" for the ~74s gap between the real audio ending and the
  state machine still expecting more.
- **r132** - Track 10 (Ganeshmantra8.mp3) moved out of feet's mantraTracks[]
  rotation (now 13 tracks, not 14) and into the closing Aarti itself, per
  direct request that it "run after aarti as one block of aarti" - the
  user confirmed it should come out of feet's rotation entirely (not stay
  in both) and play immediately back-to-back with no gap after
  AARTI_TRACK. New `AARTI_PART1_DURATION`/`AARTI_PART2_TRACK`/
  `AARTI_PART2_DURATION` in config.h; `AARTI_DURATION` is now their
  combined total (331520ms) rather than AARTI_TRACK's own length, since
  that's what `setSystemState()` actually uses for `STATE_AARTI` - the LED
  flame arc and eye breathing pulse (both keyed off the state's total
  duration, r128) now naturally stretch across both parts as one
  continuous scripted sequence instead of two separate chants. New
  `aartiPart2Playing` flag (reset in `triggerAarti()`, checked in
  `updateStateMachine()`'s `STATE_AARTI` case) switches the physical track
  at exactly `AARTI_PART1_DURATION` elapsed. Also caught and fixed a real,
  separate sync gap while touching this: `web_dashboard.h`'s own JS
  mirror of `mantraTracks[]` (used for the dashboard's local Quick-
  Blessings simulation and Now-Playing track lookups) still had track 14
  at its old pre-r131 duration (175730ms) and still listed track 10 -
  neither had been touched by r131/r132's firmware-side changes since
  they're a separately-maintained duplicate; both corrected here too.
- **r133** - Two direct follow-ups. (1) Added a proper "Play Track"
  dashboard control - a dropdown of all 29 known SD card tracks (feet,
  mouse-back, bell, both Aarti parts, offline blessings) plus a Play
  button, calling a new `/api/control?action=playtrack&track=N`. Unlike
  the pre-existing `/api/test?track=N` (a raw diagnostic - `dfPlay()` and
  nothing else, no OLED/LED/Now-Playing update), the new
  `playTrackManually()` looks up the real duration from whichever pool
  the track belongs to and puts the device into `STATE_FEET_ACTIVE` so
  the OLED/LED/dashboard all show something coherent for the track's
  actual length - specifically requested so the long mouse-back chants
  (22 at 3:59, 29 at 4:13) can be played on demand rather than only
  landing by chance in the rotation. (2) Removed the dashboard's
  "One-Directional Mic" feature (browser `getUserMedia` wake-on-clap +
  crowd-reactive Aarti brightness) entirely, per direct request to clean
  up unused panels - it could never actually work in real use:
  `getUserMedia` is a secure-context-only browser API (HTTPS or
  localhost), and this dashboard is normally opened at the ESP32's own
  plain-HTTP local IP (no static IP, no HTTPS - see this file's Firmware
  section), the exact same hard browser rule already documented
  elsewhere in this file for why puja.html's own speech feature had to
  move off the ESP32 entirely. Removed as dead weight (HTML panel, all
  mic JS, the crowd-reactive brightness boost in the LED loop) rather
  than left toggleable-but-broken; the "Devotee Pitch"/"Active Language"
  selects it shared a box with are untouched and still work as before.
- **r135 - REVERTED r134** - Temple Atmosphere restored as its own
  separate control from Theme of the Day, undoing r134's merge entirely
  (`selectedAtmosphere`, `applyAtmosphere()`, the `/api/settings?atmosphere=`
  handler, `/api/state`'s `atmosphere` field, the separate dropdown, and
  `THEME_ORDER`/`themes{}` back to their original 7 weekday-only entries -
  all restored via `git revert`). Direct correction after r134 shipped:
  the two controls being separate was never accidental complexity - it
  was deliberately built so they COMBINE (any Theme + any Atmosphere
  together, e.g. Tuesday's Ganesha colors with a Night wash layered on
  top), and collapsing them into one dropdown removed exactly that
  combination the feature existed for, leaving only one selection at a
  time. r134's underlying diagnosis (all three AMBIENT-only controls are
  invisible most of the day since Standby/Closed force the ring fully
  black) was correct and stands - r128 already addressed the closely
  related "Aarti firing on every idle cycle" complaint from the same
  root cause - but consolidating Theme+Atmosphere was the wrong fix for
  it. "AMBIENT LED Pattern" (renamed from "Default LED Pattern", r134's
  other change) is kept - Pattern is a real separate axis (motion, not
  color) and the rename is uncontested; only the Theme/Atmosphere merge
  is undone. Open follow-up from the same conversation, not yet decided:
  Pattern currently sits as a third fully independent manual pick with no
  relationship to Theme/Atmosphere/anything else, and should instead be
  "linked to something other" per direct feedback - exactly what it
  should link to (a specific Theme? the blessing mood palette used
  elsewhere? something else entirely) still needs to be decided before
  implementing.
- **r136** - Two direct follow-ups, closing out the open questions from
  r135. (1) Reverted r132 entirely - track 10 (Ganeshmantra8.mp3) is
  back in feet's `mantraTracks[]` rotation (14 tracks again) and no
  longer plays after the closing Aarti; `AARTI_DURATION` is back to just
  `AARTI_TRACK`'s own real length (206000ms). `AARTI_PART1_DURATION`/
  `AARTI_PART2_TRACK`/`AARTI_PART2_DURATION`/`aartiPart2Playing` all
  removed; `web_dashboard.h`/`index.html`'s JS mirrors (the `mantraTracks`
  array, `AARTI_FALLBACK_DURATION_MS`, the "Play Track" dropdown's
  labels for 10/16) corrected to match. Direct, unambiguous correction -
  "good, no track 10 after track 16." (2) Answered the open question
  from r135: "AMBIENT LED Pattern" is now tied to the last real
  blessing's mood (new `moodPatternFor()`, reusing the same 6 moods as
  `moodColorsFor()`) instead of sitting as a third fully independent
  manual pick - joyful->Rainbow Dream, hopeful->Peacock Wave, comforting/
  peaceful->Golden Aura, empowering->Circuit Pulse, grateful->Diya
  Flicker. Falls back to the manual dropdown only when no blessing has
  set `currentMoodTag` yet (fresh boot, or after whatever last cleared
  it). Deliberately NOT mirrored in the dashboard's own local LED
  preview - the browser's local mood classifier (`BLESSING_MOODS`, a
  separate 4-mood keyword-matching approximation for the local Quick-
  Blessings simulation) uses different names entirely from the real
  backend's 6-mood vocabulary, so there's no correct local value to map
  from; same known kind of preview-only gap as Temple Atmosphere's local
  preview (r116).
- **r137 - REVERTED r136's track-10 change** - direct correction: track
  10 DOES belong with Aarti track 16, playing immediately after it as
  one continuous block - that was the original r132 intent all along,
  and r136 misread a confirmation message as asking to undo it. Fully
  reinstated: track 10 removed from `mantraTracks[]` again (13 tracks),
  `AARTI_PART1_DURATION`/`AARTI_PART2_TRACK`/`AARTI_PART2_DURATION`/
  `aartiPart2Playing` all restored, `AARTI_DURATION` back to the combined
  total (331520ms), and `web_dashboard.h`/`index.html`'s JS mirrors
  (`mantraTracks`, `AARTI_FALLBACK_DURATION_MS`, the "Play Track"
  dropdown's 10/16 labels) corrected to match again. r136's OTHER change
  (Pattern tied to blessing mood) was correct and is untouched by this.
- **r138** - Two direct follow-ups after live testing. (1) The dashboard's
  "System State" card was still showing the raw state machine name (e.g.
  "MANTRA_ACTIVE" for a mouse-back touch) even though the simulated OLED
  label right above it already used r127's friendly `oledStateTag()`
  wording - reported directly as the physical OLED correctly saying
  "MOUSE BACK" (r126) while the dashboard card still said "mantra
  active", the exact confusion r126/r127 were meant to fix. `updateUI()`
  now applies `oledStateTag()` to both. (2) The mouse eye's breathing
  dip (80%->10%->80%, r125) reported as "very gradual" on hardware - the
  cycle period is halved, new `EYE_LED_BREATH_PERIOD_MS` (1500ms, was a
  hardcoded 3000) in config.h so it's a named, easily-tunable constant
  rather than a magic number buried in `updateEyeLedBreathing()`.
- **r139** - Two direct follow-ups after live testing surfaced a real
  bug and a real request in the mouse-back chant pool. (1) Reported as
  "track 17 doesn't come up" and "want 17/21/23/26 more frequent than
  the rest" - a plain round-robin through all 10 mouse tracks meant
  each one, including 17, only recurred once every 10 touches, easy to
  mistake for "never". New `mouseChantSequence[]` (14 entries) is a
  weighted PLAY ORDER over the existing `mouseChantTracks[]` pool -
  17/21/23/26 each appear twice per 14-touch cycle, the other six once,
  interleaved so nothing repeats back-to-back. `mouseChantTracks[]`
  itself is untouched and still the canonical (track, duration) list
  other code (like `playTrackManually()`) looks up by track number.
  (2) Real, separate bug found while investigating a "the mantra number
  for the same mantra is different" report: the dashboard's Now Playing
  sync only ever searched `mantraTracks` (feet's JS list) for
  `data.track`, so a mouse-back touch's real track number (17/21-29)
  never matched anything and the panel silently kept showing stale info
  from whatever last matched. New `mouseChantTracksJS` (mouse-back's own
  JS list, never existed before) added to that lookup.
- **r140** - Device Health's crash line ("Crashed near stage N") now
  appends a friendly description of what stage N actually is, per direct
  request ("need to know in the dashboard itself when these stages
  appear what do they actually mean") - new `crashStageLabel()` in
  `web_dashboard.h`/`index.html` mirrors exactly where `lastStage` is set
  in `GanapatiAI.ino`'s `loop()`: 1=web server/Wi-Fi/health checks,
  2=reading touch/PIR sensors, 3=state machine update, 4=LED ring/eye
  animation, 5=OLED draw, 0=still inside `setup()` itself. Display-only -
  no firmware change, `/api/health`'s `bootCrashedStage` field already
  carried this number, it just wasn't explained anywhere the admin could
  see it.
- **r141** - Removed the "Devotee Pitch" (Child/Adult) dropdown from the
  dashboard, confirmed as clutter left over from the removed mic feature
  (r133) - it never reached the real device at all, and per direct
  question, was only ever driving the dashboard's own local demo (which
  canned blessing text and which LED pattern the local Quick-Blessings
  simulation showed). That simulation now always picks from the existing
  `combinedBlessings` list and always shows Peacock Wave for a simulated
  feet touch, instead of branching on a pitch nothing real ever measured.
  Also answered directly, not changed: the dashboard's 4-option "Active
  Language" (en/Sanskrit-Hindi/Marathi/Tamil) is real (synced to the
  device, controls the wish pad's spoken language) and could be extended
  to more languages - the backend already supports far more (see
  puja.html's own 12-language picker) - but would need new numeric codes
  wired through `LANG_TO_CODE`/`CODE_TO_LANG` here and
  `triggerWishPadBlessing()`'s language mapping in `GanapatiAI.ino`; the
  priest always picks it manually, there's no auto-detection for a
  silent wish-pad touch (nothing to detect language from). The wish
  pad's OLED text is also always the fixed English "Your silent prayer
  is heard..." regardless of which spoken language is selected - never
  translated to match.
- **r142** - Real SD card file listing (10 more new tracks, 0030-0039)
  confirmed and triggered a reorganization of the whole track catalog
  across feet/mouse-back/Aarti, per direct instructions. (1) Track 6's
  duration corrected from 155530ms (2:35) to its real 114000ms (1:54) -
  another case of the firmware's stored duration not matching a
  since-changed real file, same pattern as r130/r131/r137's track 10/14
  corrections. (2) New "Songs" concept: any track over 2 minutes is now
  excluded from BOTH mantraTracks[] (feet) and mouseChantTracks[]
  (mouse-back) - "too long with touch pads" - and lives only in a new
  duration-only lookup table, `songTracks[]`, for on-demand play via the
  dashboard's Play Track control. Applied retroactively across the whole
  catalog, not just the new batch: feet's former track 9 (2:16) and
  mouse-back's former tracks 28 (2:07) and 29 (4:13) all moved to
  `songTracks[]`, joined by two new long tracks, 32 (3:06) and 37 (3:28).
  (3) Track 22 (3:59, formerly mouse-back's longest chant) becomes a
  second, single-track Aarti alternative - `AARTI_ALT_TRACK`/
  `AARTI_ALT_DURATION` in config.h. New `lastTouchWasMouseBack` (set in
  triggerMantra()/triggerFeetMantra()) and `aartiUsingAltTrack`
  (snapshotted once per Aarti in triggerAarti()) make the closing ritual
  reflect whichever pad was actually touched most recently - a feet
  touch closes with the existing 16+10 block, a mouse-back touch closes
  with track 22 alone - chosen over a plain alternating rotation as the
  "whatever is easy" option that's also more meaningful than an
  arbitrary coin flip. `updateStateMachine()`'s part-2 switch (r137) is
  now gated on `!aartiUsingAltTrack` since the alt path has no second
  part. (4) New feet tracks 33/34/36/38/39 and new mouse-back tracks
  30/31/35 fill out both pools back to their pre-reorg sizes (feet 12->17
  after losing 9 and gaining 5; mouse-back stays at 10 after losing 3
  and gaining 3) - `mouseChantSequence[]`'s weighted indices (r139)
  recomputed for the new pool membership, same weighting scheme (17/21/
  23/26 still appear twice per 14-touch cycle). `web_dashboard.h`/
  `index.html`'s JS mirrors (`mantraTracks`, `mouseChantTracksJS`, the
  new `songTracksJS`, `allPlayableTracks`) all updated to match, and the
  Now Playing sync extended to also check `songTracksJS` so a manually-
  played Song syncs correctly instead of leaving the panel stale. Known
  gap, not fixed: the dashboard's own local Aarti simulation (separate
  demo state machine, `AARTI_TRACK_FILE`/`AARTI_FALLBACK_DURATION_MS`)
  still always shows the 16+10 block regardless of which pad last fired
  it locally - same kind of preview-only gap as Temple Atmosphere's and
  the mood-pattern link's local previews (r116, r136).

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
- **The machine running `firebase deploy` must use Node 20, not whatever
  is newest.** Confirmed on the user's Windows machine: with the system
  Node at v24.15.0 (functions target Node 20 per `package.json`'s
  `engines`, unrelated to this), `firebase deploy --only functions`
  went all the way to "Loading and analyzing source code..." and then
  died completely silently - no error, no stack trace, even with
  `--debug`. `node -e "require('./index.js')"` worked fine standalone,
  which pointed away from a broken dependency and toward the Firebase
  CLI's own child-process function-discovery step not tolerating a
  very new local Node major version. Installing Node 20 via
  `nvm-windows` (`nvm install 20 && nvm use 20`) and redeploying from
  that shell fixed it immediately - same files, same command, only the
  local Node version changed. If a deploy from a fresh machine ever
  goes silent at that exact "Loading and analyzing source code" line
  again, check `node --version` first before suspecting the code.
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
