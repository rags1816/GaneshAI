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

// Slower and a touch deeper than Google's default (1.0 rate, 0 pitch) -
// the first test came back sounding rushed and younger than intended for
// a blessing. Tune further once you've heard this version.
const SPEAKING_RATE = 0.85;
const PITCH_SEMITONES = -3.0;

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
      try {
        blessingText = await askClaudeForBlessing(name, offeringText, prayer, standardWish, langConfig, touchOnly);
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
      res.set("Access-Control-Expose-Headers", "X-Blessing-Text");
      res.set("Content-Type", "audio/wav");
      res.status(200).send(audioBuffer);
    },
);

async function askClaudeForBlessing(name, offeringText, prayer, standardWish, langConfig, touchOnly) {
  let situationText;
  if (touchOnly) {
    const theme = WISH_PAD_THEMES[Math.floor(Math.random() * WISH_PAD_THEMES.length)];
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

  const prompt = `${langConfig.claudeInstruction} You are Lord Ganesha, speaking warmly ` +
    `and directly to a devotee named ${name}, ${situationText} Under 40 words. Plain ` +
    `spoken text only - no stage directions, no quotation marks, no markdown. Remember: ` +
    `${langConfig.claudeInstruction}`;

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
  const text = data.content && data.content[0] && data.content[0].text ?
    data.content[0].text.trim() : "";
  if (!text) {
    throw new Error("Claude returned no text");
  }
  return text;
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
  return Buffer.from(data.audioContent, "base64");
}
