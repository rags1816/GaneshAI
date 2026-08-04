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

const MAX_PRAYER_CHARS = 300;
const MAX_NAME_CHARS = 60;

// Per-language Claude instruction + matching Google TTS voice. "en" (the
// en-IN-Wavenet-B voice) is the only one confirmed working by a real test
// so far - hi/ta/mr follow Google's usual <lang>-IN-Wavenet-<letter>
// naming pattern but haven't been tested against the live API yet
// (this environment can't reach it), so treat them as a first guess to
// verify the same way en-IN was just proven.
const LANGUAGE_CONFIG = {
  en: {
    claudeInstruction: "Reply in English.",
    voice: {languageCode: "en-IN", name: "en-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  hi: {
    claudeInstruction: "Reply in Hindi, written in Devanagari script.",
    voice: {languageCode: "hi-IN", name: "hi-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  ta: {
    claudeInstruction: "Reply in Tamil, written in Tamil script.",
    voice: {languageCode: "ta-IN", name: "ta-IN-Wavenet-B", ssmlGender: "MALE"},
  },
  mr: {
    claudeInstruction: "Reply in Marathi, written in Devanagari script.",
    voice: {languageCode: "mr-IN", name: "mr-IN-Wavenet-B", ssmlGender: "MALE"},
  },
};

// Slower and a touch deeper than Google's default (1.0 rate, 0 pitch) -
// the first test came back sounding rushed and younger than intended for
// a blessing. Tune further once you've heard this version.
const SPEAKING_RATE = 0.85;
const PITCH_SEMITONES = -3.0;

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
      const langKey = (req.body.lang || "en").toString();
      const langConfig = LANGUAGE_CONFIG[langKey] || LANGUAGE_CONFIG.en;

      if (!prayer.trim()) {
        res.status(400).send("prayer text is required");
        return;
      }

      let blessingText;
      try {
        blessingText = await askClaudeForBlessing(name, offeringText, prayer, langConfig);
      } catch (err) {
        console.error("Claude call failed:", err);
        res.status(502).send(`Blessing generation failed (Claude): ${err.message}`);
        return;
      }

      let audioBuffer;
      try {
        audioBuffer = await synthesizeSpeech(blessingText, langConfig);
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

async function askClaudeForBlessing(name, offeringText, prayer, langConfig) {
  const prompt = `You are Lord Ganesha, speaking warmly and directly to a devotee ` +
    `named ${name}, who has offered ${offeringText} and prayed: "${prayer}". ` +
    `Reply with a short, warm blessing that responds to their specific prayer, ` +
    `as if spoken aloud to them. Under 40 words. Plain spoken text only - no ` +
    `stage directions, no quotation marks, no markdown. ${langConfig.claudeInstruction}`;

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
