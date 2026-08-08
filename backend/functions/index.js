// GanapatiAI - generateBlessing Cloud Function
//
// Takes a devotee's typed wish (from the QR/Puja form, or the dashboard's
// quick-offering panel), asks Claude to write a short personalized
// blessing in reply, converts that reply to speech with Google
// Text-to-Speech, and hands both back in one response:
//   - the blessing text, in the X-Blessing-Text response header
//   - the spoken audio (WAV), as the raw response body
//
// The ESP32 reads the header for the OLED display and streams the body
// straight to the MAX98357A amp over I2S - no base64/JSON audio decoding
// needed on the device, which matters given its limited RAM.
//
// Both API keys are Firebase secrets, never hardcoded or committed - see
// backend/README.md for how to set them.

const {onRequest} = require("firebase-functions/v2/https");
const {defineSecret} = require("firebase-functions/params");

const ANTHROPIC_API_KEY = defineSecret("ANTHROPIC_API_KEY");
const GOOGLE_TTS_API_KEY = defineSecret("GOOGLE_TTS_API_KEY");

const OFFERING_NAMES = {
  hibiscus: "a hibiscus flower",
  garland: "a marigold garland",
  modak: "a sweet modak",
  coconut: "a fresh coconut",
};

// Touching the wish pad carries no text at all for Claude to react to, so
// left alone it tends to converge on the same handful of phrasings across
// repeated touches. Picking a random theme per touch and asking Claude to
// weave it in gives each blessing a genuinely different focus, not just
// different wording around the same idea.
const WISH_PAD_THEMES = [
  "removing obstacles from their path",
  "wisdom and clear thinking",
  "courage in difficult moments",
  "prosperity and abundance",
  "peace within their home and family",
  "good health and vitality",
  "success in new beginnings",
  "patience and inner calm",
  "protection from harm",
  "joy and contentment",
  "strength to persevere",
  "harmony with those they love",
  "love and deep affection for those around them",
  "lasting happiness, not just a passing moment",
  "success in their efforts and ambitions",
  "a true sense of belonging, of being at home",
  "comfort and rest for a weary heart",
  "grace and gentle, unearned kindness",
  "renewal and a fresh start after hard times",
  "clarity of mind and awakening to what matters",
  "gratitude for what is already theirs",
  "wholeness, with nothing broken left unmended",
  "confidence to rise after a setback",
  "lightness of heart and simple delight",
];

// Sentiment-aware LED mood - the altar's LED ring shows a color matched
// to the emotional tone of the blessing, not a fixed color regardless of
// what was actually prayed for. Six moods, each mapped to a distinct LED
// color pair on the firmware side (see moodColorsFor() in GanapatiAI.ino) -
// kept as a small fixed enum, not free text, so the firmware can switch
// on it directly with no fuzzy string matching.
const MOODS = ["joyful", "hopeful", "comforting", "peaceful", "empowering", "grateful"];

// Deterministic theme->mood mapping for the wish pad's touchOnly path -
// that path has no real prayer text for Claude to read sentiment from
// (silent touch, no words), but it DOES already pick a random theme from
// WISH_PAD_THEMES above, so mapping that same theme to a mood keeps the
// LED color and the spoken blessing's actual content in sync, with no
// extra LLM call or parsing risk. Index-aligned with WISH_PAD_THEMES.
const THEME_TO_MOOD = [
  "empowering", "peaceful", "empowering", "joyful", "peaceful", "hopeful",
  "hopeful", "peaceful", "empowering", "joyful", "empowering", "peaceful",
  "comforting", "joyful", "hopeful", "comforting", "comforting", "comforting",
  "hopeful", "peaceful", "grateful", "comforting", "empowering", "joyful",
];

const MAX_PRAYER_CHARS = 300;
const MAX_NAME_CHARS = 60;

// Per-language Claude instruction + matching Google TTS voice. Real-test
// status so far: en, hi, mr confirmed working. ta partially worked (drifted
// into English partway through) - the stronger "entirely in ... from start
// to finish, never switching to English" wording below is the fix for
// that, not yet re-verified. te/pa follow the same naming pattern as the
// confirmed ones but are untested. sd is a guess Google likely doesn't
// even support - included because it costs nothing to try, but expect it
// may fail differently (a "voice not found"/"language not supported"
// error rather than a wrong-voice-name error). bn is new and untested,
// same naming pattern as the confirmed ones.
const LANGUAGE_CONFIG = {
  en: {
    claudeInstruction: "Reply entirely in English, from start to finish.",
    voice: {languageCode: "en-IN", name: "en-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  hi: {
    claudeInstruction: "Reply entirely in Hindi (Devanagari script), from start to finish, never switching to English.",
    voice: {languageCode: "hi-IN", name: "hi-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  ta: {
    claudeInstruction: "Reply entirely in Tamil (Tamil script), from start to finish, never switching to English.",
    voice: {languageCode: "ta-IN", name: "ta-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  mr: {
    claudeInstruction: "Reply entirely in Marathi (Devanagari script), from start to finish, never switching to English.",
    voice: {languageCode: "mr-IN", name: "mr-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  te: {
    claudeInstruction: "Reply entirely in Telugu (Telugu script), from start to finish, never switching to English.",
    voice: {languageCode: "te-IN", name: "te-IN-Standard-B", ssmlGender: "MALE"},
  },
  ml: {
    claudeInstruction: "Reply entirely in Malayalam (Malayalam script), from start to finish, never switching to English.",
    voice: {languageCode: "ml-IN", name: "ml-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  bn: {
    claudeInstruction: "Reply entirely in Bengali (Bengali script), from start to finish, never switching to English.",
    voice: {languageCode: "bn-IN", name: "bn-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  gu: {
    claudeInstruction: "Reply entirely in Gujarati (Gujarati script), from start to finish, never switching to English.",
    voice: {languageCode: "gu-IN", name: "gu-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  pa: {
    claudeInstruction: "Reply entirely in Punjabi (Gurmukhi script), from start to finish, never switching to English.",
    voice: {languageCode: "pa-IN", name: "pa-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  sd: {
    claudeInstruction: "Reply entirely in Sindhi, from start to finish, never switching to English.",
    voice: {languageCode: "sd-IN", name: "sd-IN-Standard-B", ssmlGender: "MALE"},
  },
  fa: {
    claudeInstruction: "Reply entirely in Persian/Farsi (Persian script), from start to finish, never switching to English.",
    voice: {languageCode: "fa-IR", name: "fa-IR-Wavenet-B", ssmlGender: "MALE"},
  },
};

// Pushed further after real feedback: family listening to the first
// version (0.85 rate, -3 pitch) said it sounded rushed, monotonous, and
// not "godly" - too close to a normal announcement voice. This is
// deliberately close to the edge of what Google's Wavenet voices can
// take before words start blurring together; if a specific language
// comes back garbled rather than just slow/deep, ease PITCH_SEMITONES
// back toward -3 to -4 for that voice specifically rather than reverting
// everything.
const SPEAKING_RATE = 0.72;
const PITCH_SEMITONES = -5.0;

// Reverb ("the voice fills a temple") is applied as actual audio DSP
// after Google TTS returns the WAV, not a TTS parameter - Google's API
// has no reverb/echo control, only pitch and rate. See applyReverb()
// below for the algorithm (Schroeder comb+allpass, run through Node
// directly on the raw PCM16 samples - no external audio library needed).
const REVERB_WET_MIX = 0.30; // 30% wet - audible without drowning the words
const REVERB_TAIL_SECONDS = 1.2;

// Lightweight TTS-only endpoint - takes text that's ALREADY been decided
// (the blessing the devotee already saw/heard on their own phone, read
// back out of the priest queue) and just synthesizes speech for it, with
// no Claude call. Used by the ESP32 itself when a priest approves an
// offering, so the altar's own speaker can speak the SAME blessing the
// devotee got, rather than generating a second, different one and paying
// for a Claude call the devotee's phone already made.
const MAX_BLESSING_CHARS = 500;

exports.synthesizeAudio = onRequest(
    {secrets: [GOOGLE_TTS_API_KEY], cors: true},
    async (req, res) => {
      if (req.method !== "POST") {
        res.status(405).send("POST only");
        return;
      }

      const text = (req.body.text || "").toString().slice(0, MAX_BLESSING_CHARS);
      if (!text.trim()) {
        res.status(400).send("text is required");
        return;
      }
      const langKey = (req.body.lang || "en").toString();
      const langConfig = LANGUAGE_CONFIG[langKey] || LANGUAGE_CONFIG.en;

      let audioBuffer;
      try {
        audioBuffer = await synthesizeSpeech(text, langConfig);
      } catch (err) {
        console.error("Google TTS call failed:", err);
        res.status(502).send(`Speech synthesis failed: ${err.message}`);
        return;
      }

      res.set("Content-Type", "audio/wav");
      res.status(200).send(audioBuffer);
    },
);

exports.generateBlessing = onRequest(
    {secrets: [ANTHROPIC_API_KEY, GOOGLE_TTS_API_KEY], cors: true},
    async (req, res) => {
      if (req.method !== "POST") {
        res.status(405).send("POST only");
        return;
      }

      const name = (req.body.name || "a devotee").toString().slice(0, MAX_NAME_CHARS);
      const offeringKey = (req.body.offering || "").toString();
      const offeringText = OFFERING_NAMES[offeringKey] || "an offering";
      const prayer = (req.body.prayer || "").toString().slice(0, MAX_PRAYER_CHARS);
      const standardWish = (req.body.standardWish || "").toString().slice(0, 60);
      // Set by the physical wish pad (see triggerWishPadBlessing() in
      // GanapatiAI.ino) - a devotee touching it at the altar, with no
      // offering and no typed/spoken wish at all, the way one would touch
      // a deity's feet in silent prayer. Distinct from the puja.html
      // "silent wish" case, which still has a real offering attached.
      const touchOnly = req.body.touchOnly === true;
      const langKey = (req.body.lang || "en").toString();
      const langConfig = LANGUAGE_CONFIG[langKey] || LANGUAGE_CONFIG.en;
      // Voice can be requested separately from the text language - e.g.
      // text stays in English but is read in an Indian-accented voice.
      // Untested combination: TTS voices generally expect the phonemes of
      // their own language, so English text read by e.g. a Hindi voice
      // may come out garbled rather than just "accented" - worth a real
      // listen before relying on it.
      const voiceLangKey = (req.body.voiceLang || langKey).toString();
      const voiceConfig = LANGUAGE_CONFIG[voiceLangKey] || langConfig;

      // No typed wish is a real, deliberate case, not an error - many
      // devotees pray silently rather than writing their wish down.
      // askClaudeForBlessing() branches its prompt based on whether
      // prayer is empty.

      let blessingText;
      let blessingMood;
      try {
        ({text: blessingText, mood: blessingMood} =
          await askClaudeForBlessing(name, offeringText, prayer, standardWish, langConfig, touchOnly));
      } catch (err) {
        console.error("Claude call failed:", err);
        res.status(502).send(`Blessing generation failed (Claude): ${err.message}`);
        return;
      }

      let audioBuffer;
      try {
        audioBuffer = await synthesizeSpeech(blessingText, voiceConfig);
      } catch (err) {
        console.error("Google TTS call failed:", err);
        res.status(502).send(`Blessing generation failed (voice): ${err.message}`);
        return;
      }

      res.set("X-Blessing-Text", encodeURIComponent(blessingText));
      res.set("X-Blessing-Mood", blessingMood);
      res.set("Access-Control-Expose-Headers", "X-Blessing-Text, X-Blessing-Mood");
      res.set("Content-Type", "audio/wav");
      res.status(200).send(audioBuffer);
    },
);

// Pulls Claude's self-reported mood out of its reply, tolerating real
// deviation from the requested `[mood]`-at-the-start format - a prompt
// this size (language instructions repeated twice, word limit, "no
// markdown", THEN the mood-tag instruction tacked on last) genuinely
// might not get followed to the letter every time, and every blessing
// silently defaulting to the same mood (confirmed as the actual
// symptom reported: two very different prayers both showing the same
// LED color) is a real, reported failure mode, not a hypothetical one.
// Three tiers, each looser than the last:
//   1. `[mood]` exactly at the start (the requested format).
//   2. `[mood]` anywhere in the first 40 characters (a short unwanted
//      preamble before the tag, rather than no tag at all).
//   3. A bare mood word, no brackets, anywhere in the first 60
//      characters (Claude wrote the mood as English rather than
//      formatting it as instructed).
// Only truly gives up (constant "peaceful") if none of those find
// anything - and logs the raw text either way, so a real Claude
// formatting drift shows up in Firebase Functions logs immediately
// instead of silently blending into "working, just always peaceful".
function extractMood(text) {
  const head = text.slice(0, 60);

  let match = head.match(/^\[(\w+)\]\s*/i);
  if (match && MOODS.includes(match[1].toLowerCase())) {
    return {mood: match[1].toLowerCase(), cleanText: text.slice(match[0].length).trim()};
  }

  match = head.slice(0, 40).match(/\[(\w+)\]/i);
  if (match && MOODS.includes(match[1].toLowerCase())) {
    const idx = text.indexOf(match[0]);
    return {mood: match[1].toLowerCase(), cleanText: (text.slice(0, idx) + text.slice(idx + match[0].length)).trim()};
  }

  const wordMatch = head.match(new RegExp(`\\b(${MOODS.join("|")})\\b`, "i"));
  if (wordMatch) {
    // Bare word left in place deliberately - a stray mood word loose in
    // real English prose reads oddly if silently cut, and this branch
    // means Claude already deviated from instructions once; trust the
    // text less, not more.
    return {mood: wordMatch[1].toLowerCase(), cleanText: text};
  }

  return {mood: "peaceful", cleanText: text};
}

// Returns {text, mood} - mood drives the LED ring's color for the
// duration of this blessing (see MOODS/THEME_TO_MOOD above). touchOnly
// gets its mood deterministically from the theme already chosen (no
// prayer text exists for Claude to read sentiment from); every other
// case asks Claude to self-classify by prefixing its reply with the
// mood word in brackets, which is then parsed out and stripped before
// the text is spoken/displayed - the devotee never sees the tag itself.
async function askClaudeForBlessing(name, offeringText, prayer, standardWish, langConfig, touchOnly) {
  let situationText;
  let deterministicMood = null;
  if (touchOnly) {
    const themeIdx = Math.floor(Math.random() * WISH_PAD_THEMES.length);
    const theme = WISH_PAD_THEMES[themeIdx];
    deterministicMood = THEME_TO_MOOD[themeIdx];
    situationText = `who has come before you at the altar and gently touched the wish pad - ` +
      `the way one would touch a deity's feet - offering no words, only a silent prayer held ` +
      `in their heart. Reply with a short, warm blessing that acknowledges their silent prayer ` +
      `is heard and accepted, blessing them regardless of what they silently wish for, as if ` +
      `spoken aloud to them. Center this particular blessing around ${theme}, phrased freshly ` +
      `and naturally rather than formulaically - avoid opening the same way you would for a ` +
      `different theme.`;
  } else if (prayer.trim()) {
    situationText = `who has offered ${offeringText} and prayed: "${prayer}". Reply with ` +
      `a short, warm blessing that responds to their specific prayer, as if spoken aloud to them.`;
  } else if (standardWish.trim()) {
    situationText = `who has offered ${offeringText} and silently asked, in their heart, ` +
      `for blessings related to "${standardWish}" without writing it down. Reply with a ` +
      `short, warm blessing that responds to that, as if spoken aloud to them.`;
  } else {
    situationText = `who has offered ${offeringText} and come with a wish held silently ` +
      `in their heart, not written down. Reply with a short, warm, generic blessing that ` +
      `acknowledges their unspoken prayer and blesses them regardless of what they silently ` +
      `wish for, as if spoken aloud to them.`;
  }

  let prompt = `${langConfig.claudeInstruction} You are Lord Ganesha, speaking warmly ` +
    `and directly to a devotee named ${name}, ${situationText} Under 40 words. Plain ` +
    `spoken text only - no stage directions, no quotation marks, no markdown. Remember: ` +
    `${langConfig.claudeInstruction}`;
  if (!deterministicMood) {
    prompt += ` Before the blessing itself, on the very first line, write ONLY one word ` +
      `classifying its emotional mood - exactly one of: ${MOODS.join(", ")}. Format that ` +
      `first line as [mood] with square brackets, e.g. [hopeful], then a line break, then ` +
      `the blessing text itself (still in ${langConfig.claudeInstruction.includes("English") ? "English" : "the language above"}, ` +
      `the mood word is the only exception).`;
  }

  const response = await fetch("https://api.anthropic.com/v1/messages", {
    method: "POST",
    headers: {
      "content-type": "application/json",
      "x-api-key": ANTHROPIC_API_KEY.value(),
      "anthropic-version": "2023-06-01",
    },
    body: JSON.stringify({
      model: "claude-haiku-4-5-20251001",
      max_tokens: 150,
      messages: [{role: "user", content: prompt}],
    }),
  });

  if (!response.ok) {
    throw new Error(`Anthropic API returned ${response.status}: ${await response.text()}`);
  }

  const data = await response.json();
  let text = data.content && data.content[0] && data.content[0].text ?
    data.content[0].text.trim() : "";
  if (!text) {
    throw new Error("Claude returned no text");
  }

  let mood = deterministicMood;
  if (!mood) {
    const rawStart = text.slice(0, 60);
    const extracted = extractMood(text);
    mood = extracted.mood;
    text = extracted.cleanText;
    console.log(`MOOD: raw start="${rawStart}" -> parsed=${mood}`);
  }

  return {text, mood};
}

async function synthesizeSpeech(text, langConfig) {
  const url = `https://texttospeech.googleapis.com/v1/text:synthesize?key=${GOOGLE_TTS_API_KEY.value()}`;
  const response = await fetch(url, {
    method: "POST",
    headers: {"content-type": "application/json"},
    body: JSON.stringify({
      input: {text},
      voice: langConfig.voice,
      audioConfig: {
        audioEncoding: "LINEAR16",
        sampleRateHertz: 16000,
        speakingRate: SPEAKING_RATE,
        pitch: PITCH_SEMITONES,
      },
    }),
  });

  if (!response.ok) {
    throw new Error(`Google TTS returned ${response.status}: ${await response.text()}`);
  }

  const data = await response.json();
  if (!data.audioContent) {
    throw new Error("Google TTS returned no audio");
  }
  const rawWav = Buffer.from(data.audioContent, "base64");

  // If the reverb DSP throws for any reason, fall back to the plain
  // (dry) voice rather than failing the whole blessing - a slightly
  // less atmospheric voice beats no voice at all.
  try {
    const {sampleRate, pcmData} = parseWav(rawWav);
    const wetPcm = applyReverb(pcmData, sampleRate);
    return buildWav(wetPcm, sampleRate);
  } catch (err) {
    console.error("Reverb DSP failed, returning dry audio instead:", err);
    return rawWav;
  }
}

// Minimal WAV (RIFF/PCM) reader - scans for the "fmt " and "data" chunks
// rather than assuming the canonical 44-byte header, the same defensive
// approach the ESP32 firmware's own WAV parser uses (skipToWavData() in
// GanapatiAI.ino) - robust against Google ever adding extra chunks.
function parseWav(buffer) {
  if (buffer.toString("ascii", 0, 4) !== "RIFF" || buffer.toString("ascii", 8, 12) !== "WAVE") {
    throw new Error("Not a RIFF/WAVE file");
  }
  let offset = 12;
  let sampleRate = 16000;
  let pcmData = null;
  while (offset + 8 <= buffer.length) {
    const chunkId = buffer.toString("ascii", offset, offset + 4);
    const chunkSize = buffer.readUInt32LE(offset + 4);
    const bodyStart = offset + 8;
    if (chunkId === "fmt ") {
      sampleRate = buffer.readUInt32LE(bodyStart + 4);
    } else if (chunkId === "data") {
      pcmData = buffer.subarray(bodyStart, bodyStart + chunkSize);
    }
    offset = bodyStart + chunkSize + (chunkSize % 2); // chunks are word-aligned
  }
  if (!pcmData) throw new Error("No data chunk found in WAV");
  return {sampleRate, pcmData};
}

// Writes a canonical 44-byte-header mono 16-bit PCM WAV - what the ESP32
// side and every normal WAV consumer expects.
function buildWav(pcmData, sampleRate) {
  const numChannels = 1;
  const bitsPerSample = 16;
  const byteRate = sampleRate * numChannels * bitsPerSample / 8;
  const blockAlign = numChannels * bitsPerSample / 8;
  const header = Buffer.alloc(44);
  header.write("RIFF", 0, "ascii");
  header.writeUInt32LE(36 + pcmData.length, 4);
  header.write("WAVE", 8, "ascii");
  header.write("fmt ", 12, "ascii");
  header.writeUInt32LE(16, 16); // fmt chunk size (PCM)
  header.writeUInt16LE(1, 20); // audio format = PCM
  header.writeUInt16LE(numChannels, 22);
  header.writeUInt32LE(sampleRate, 24);
  header.writeUInt32LE(byteRate, 28);
  header.writeUInt16LE(blockAlign, 32);
  header.writeUInt16LE(bitsPerSample, 34);
  header.write("data", 36, "ascii");
  header.writeUInt32LE(pcmData.length, 40);
  return Buffer.concat([header, pcmData]);
}

// "Fills the whole space" reverb - a classic Schroeder design (four
// parallel feedback comb filters summed together, then two series
// all-pass filters to diffuse the sound without adding obvious metallic
// coloration). Runs directly on the raw PCM16 samples; no external audio
// library, so no extra Cloud Function dependency or cold-start cost.
// Delay times are the well-known ratios (mutually near-prime) that avoid
// audible resonance/flutter. Adds REVERB_TAIL_SECONDS of ringing silence
// after the voice ends, same as a real room continuing to reflect sound
// after the speaker stops.
function applyReverb(pcmBuffer, sampleRate) {
  const numSamples = pcmBuffer.length / 2;
  const tailSamples = Math.round(sampleRate * REVERB_TAIL_SECONDS);
  const totalSamples = numSamples + tailSamples;

  const input = new Float32Array(totalSamples); // trailing zeros = the tail's silent "input"
  for (let i = 0; i < numSamples; i++) {
    input[i] = pcmBuffer.readInt16LE(i * 2) / 32768;
  }

  const combDelaysMs = [29.7, 37.1, 41.1, 43.7];
  const combGain = 0.78;
  const combSum = new Float32Array(totalSamples);
  for (const ms of combDelaysMs) {
    const delaySamples = Math.max(1, Math.round(sampleRate * ms / 1000));
    const delayLine = new Float32Array(delaySamples);
    let idx = 0;
    for (let i = 0; i < totalSamples; i++) {
      const delayed = delayLine[idx];
      const y = input[i] + delayed * combGain;
      delayLine[idx] = y;
      idx = (idx + 1) % delaySamples;
      combSum[i] += y * 0.25;
    }
  }

  function allpass(signal, delayMs, gain) {
    const delaySamples = Math.max(1, Math.round(sampleRate * delayMs / 1000));
    const out = new Float32Array(signal.length);
    const delayLine = new Float32Array(delaySamples);
    let idx = 0;
    for (let i = 0; i < signal.length; i++) {
      const delayed = delayLine[idx];
      const y = -gain * signal[i] + delayed;
      delayLine[idx] = signal[i] + gain * y;
      idx = (idx + 1) % delaySamples;
      out[i] = y;
    }
    return out;
  }

  let wet = allpass(combSum, 5.0, 0.7);
  wet = allpass(wet, 1.7, 0.7);

  const outBuf = Buffer.alloc(totalSamples * 2);
  for (let i = 0; i < totalSamples; i++) {
    let v = input[i] * (1 - REVERB_WET_MIX) + wet[i] * REVERB_WET_MIX;
    v = Math.max(-1, Math.min(1, v)); // hard-limit - the comb/allpass feedback can occasionally overshoot
    outBuf.writeInt16LE(Math.round(v * 32767), i * 2);
  }
  return outBuf;
}
