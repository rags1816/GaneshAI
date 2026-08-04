#ifndef PUJA_PAGE_H
#define PUJA_PAGE_H

// Devotee-facing offering submission page, embedded verbatim from the
// repo's puja.html so it can be served directly by the ESP32 (previously
// not served at all - only index.html/web_dashboard.h's admin dashboard
// was). Scanning the QR code on the admin dashboard links here.
const char PUJA_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Ganapati Bappa Puja Portal</title>
    <!-- iOS Web App Meta Tags -->
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="apple-mobile-web-app-title" content="Bappa Puja">
    
    <link rel="icon" type="image/png" sizes="512x512" href="ganesha.png">
    <link rel="apple-touch-icon" sizes="512x512" href="ganesha.png">
    
    <style>
        :root {
            --bg-color: #030812;
            --card-bg: rgba(2, 10, 23, 0.85);
            --border-color: rgba(0, 242, 254, 0.15);
            --accent-teal: #00f2fe;
            --accent-gold: #ffd700;
            --accent-orange: #ffa500;
            --text-color: #e2e8f0;
            --text-muted: #8892b0;
        }

        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Arial, sans-serif;
            margin: 0;
            padding: 15px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            box-sizing: border-box;
        }

        .puja-card {
            background: var(--card-bg);
            border: 1px solid var(--border-color);
            border-radius: 20px;
            width: 100%;
            max-width: 400px;
            padding: 24px;
            box-shadow: 0 10px 30px rgba(0, 242, 254, 0.05);
            box-sizing: border-box;
            position: relative;
            overflow: hidden;
        }

        .puja-card::before {
            content: '';
            position: absolute;
            top: -50px; left: -50px;
            width: 100px; height: 100px;
            background: radial-gradient(circle, rgba(0, 242, 254, 0.15) 0%, transparent 70%);
            pointer-events: none;
        }

        .puja-card::after {
            content: '';
            position: absolute;
            bottom: -50px; right: -50px;
            width: 100px; height: 100px;
            background: radial-gradient(circle, rgba(255, 215, 0, 0.1) 0%, transparent 70%);
            pointer-events: none;
        }

        header {
            text-align: center;
            margin-bottom: 24px;
        }

        .logo-placeholder {
            width: 70px;
            height: 70px;
            border-radius: 50%;
            background: radial-gradient(circle, rgba(255, 215, 0, 0.2) 20%, transparent 80%);
            margin: 0 auto 12px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 32px;
            box-shadow: 0 0 15px rgba(255, 215, 0, 0.1);
        }

        h1 {
            font-size: 20px;
            margin: 0;
            background: linear-gradient(to right, var(--accent-gold), var(--accent-orange));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            font-weight: 800;
            letter-spacing: 0.5px;
        }

        p {
            font-size: 11px;
            color: var(--text-muted);
            margin: 4px 0 0 0;
            line-height: 1.4;
        }

        .form-group {
            margin-bottom: 16px;
        }

        label {
            display: block;
            font-size: 10px;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: var(--accent-teal);
            margin-bottom: 6px;
        }

        .select-input, .text-input {
            width: 100%;
            background: rgba(0, 0, 0, 0.4);
            border: 1px solid rgba(0, 242, 254, 0.2);
            border-radius: 8px;
            padding: 10px;
            color: #ffffff;
            font-size: 13px;
            outline: none;
            box-sizing: border-box;
            font-family: inherit;
            transition: all 0.3s ease;
        }

        .select-input:focus, .text-input:focus {
            border-color: var(--accent-gold);
            box-shadow: 0 0 8px rgba(255, 215, 0, 0.15);
        }

        textarea.text-input {
            resize: none;
            height: 65px;
        }

        .count-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-top: 4px;
        }

        .counter-lbl {
            font-size: 9px;
            color: var(--text-muted);
        }

        .btn-submit {
            width: 100%;
            background: linear-gradient(135deg, var(--accent-gold), var(--accent-orange));
            color: #030812;
            border: none;
            border-radius: 10px;
            padding: 12px;
            font-size: 14px;
            font-weight: 700;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(255, 215, 0, 0.2);
            margin-top: 10px;
        }

        .btn-submit:hover {
            filter: brightness(1.1);
            transform: translateY(-1px);
            box-shadow: 0 6px 20px rgba(255, 215, 0, 0.35);
        }

        .btn-submit:disabled {
            background: #2a2a2a;
            color: #666;
            cursor: not-allowed;
            box-shadow: none;
            transform: none;
        }

        #success-card {
            display: none;
            text-align: center;
            padding: 10px 0;
        }

        .success-icon {
            font-size: 40px;
            margin-bottom: 12px;
            animation: pulse 1.5s infinite;
        }

        @keyframes pulse {
            0% { transform: scale(1); }
            50% { transform: scale(1.1); }
            100% { transform: scale(1); }
        }

        .btn-back {
            background: transparent;
            border: 1px dashed rgba(255, 215, 0, 0.4);
            color: var(--accent-gold);
            padding: 8px 16px;
            border-radius: 6px;
            font-size: 11px;
            cursor: pointer;
            margin-top: 20px;
            transition: all 0.3s ease;
        }

        .btn-back:hover {
            background: rgba(255, 215, 0, 0.05);
            border-color: var(--accent-gold);
        }
    </style>
</head>
<body>
    <div class="puja-card">
        <header>
            <div class="logo-placeholder">🌸</div>
            <h1>Ganapati Bappa Puja</h1>
            <p>Offer prasad & send your prayers to Lord Ganesha</p>
        </header>

        <!-- Main Form -->
        <div id="puja-form">
            <!-- 0. Devotee Name -->
            <div class="form-group">
                <label for="name-input">Your Name (Optional)</label>
                <input type="text" id="name-input" class="text-input" placeholder="Type your name..." maxlength="30">
            </div>

            <!-- 1. Offerings Selection -->
            <div class="form-group">
                <label for="offering-select">Choose your Offering</label>
                <select id="offering-select" class="select-input">
                    <option value="hibiscus">🌺 Hibiscus Flower</option>
                    <option value="garland">🌼 Marigold Garland</option>
                    <option value="modak" selected>🥟 Sweet Modak</option>
                    <option value="coconut">🥥 Fresh Coconut</option>
                </select>
            </div>

            <!-- 2. Standard Blessings Picker -->
            <div class="form-group">
                <label for="standard-wish-select">Ask for Blessing (Optional)</label>
                <select id="standard-wish-select" class="select-input">
                    <option value="">-- Choose Standard Blessing --</option>
                    <option value="Wisdom & Intellect">🧠 Wisdom & Intellect</option>
                    <option value="Prosperity & Wealth">💰 Prosperity & Wealth</option>
                    <option value="Good Health & Energy">⚡ Good Health & Energy</option>
                    <option value="Removal of Obstacles">⛰️ Removal of Obstacles</option>
                    <option value="Peace & Happiness">🕊️ Peace & Happiness</option>
                </select>
            </div>

            <!-- 3. Free Text wish/prayer -->
            <div class="form-group">
                <label for="wish-input">Personal Prayer Wish (Max 20 words)</label>
                <textarea id="wish-input" class="text-input" placeholder="Write your personal prayer here..." oninput="updateWordCount()"></textarea>
                <div class="count-row">
                    <span id="word-count-lbl" class="counter-lbl">0 / 20 words</span>
                </div>
            </div>

            <button id="submit-btn" class="btn-submit" onclick="submitPuja()">✨ Send Offering to Bappa ✨</button>
        </div>

        <!-- Success Response Screen -->
        <div id="success-card">
            <div class="success-icon">✨🙏✨</div>
            <h2 style="font-size: 16px; color: var(--accent-gold); margin: 0 0 10px 0;">Prayer Submitted</h2>
            <p style="font-size: 12px; color: var(--text-color); margin-bottom: 20px;">
                Your offering and prayer have been sent to Ganesha's Priest. Please wait for the priest to present your offering to Bappa!
            </p>
            <button class="btn-back" onclick="resetForm()">Make Another Offering</button>
        </div>
    </div>

    <script>
        // Helpers for Base64URL encoding/decoding (bypasses IIS special character path restrictions)
        function base64UrlEncode(str) {
            return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g, (match, p1) => String.fromCharCode(parseInt(p1, 16)))).replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
        }
        function base64UrlDecode(str) {
            let b64 = str.replace(/-/g, '+').replace(/_/g, '/');
            while (b64.length % 4) b64 += '=';
            return decodeURIComponent(atob(b64).split('').map(c => '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2)).join(''));
        }

        // Offering queue cloud relay - tries the primary free service
        // first; if it's down (e.g. HTTP 503), falls back to a second
        // free service automatically. Needed because devotees can be
        // outside the home Wi-Fi entirely (family in the UK away from
        // home, or in India), so this can't just be a local ESP32 call -
        // it has to work over the open internet from anywhere.
        const RELAY_APP_KEY = "sxnoamwe";
        const RELAY_ITEM_KEY = "ganesha_queue";
        const RELAY_PRIMARY_READ_URL = `https://keyvalue.immanuel.co/api/KeyVal/GetValue/${RELAY_APP_KEY}/${RELAY_ITEM_KEY}`;
        const RELAY_PRIMARY_WRITE_URL = (val) => `https://keyvalue.immanuel.co/api/KeyVal/UpdateValue/${RELAY_APP_KEY}/${RELAY_ITEM_KEY}/${val}`;
        const RELAY_FALLBACK_URL = `https://kvdb.io/9Vif1fNa3c128daX5AFe9S/${RELAY_ITEM_KEY}`;

        function relayReadQueue() {
            return fetch(`${RELAY_PRIMARY_READ_URL}?_t=${Date.now()}`)
                .then(res => { if (!res.ok) throw new Error('primary read status ' + res.status); return res.json(); })
                .then(b64data => {
                    if (b64data && b64data.trim() !== "" && b64data !== "test_value" && b64data !== "[]") {
                        return JSON.parse(base64UrlDecode(b64data));
                    }
                    return [];
                })
                .catch(err => {
                    console.warn("Primary relay (keyvalue.immanuel.co) read failed, trying fallback (kvdb.io):", err);
                    return fetch(`${RELAY_FALLBACK_URL}?_t=${Date.now()}`)
                        .then(res => (res.ok ? res.text() : ""))
                        .then(text => {
                            if (!text || text.trim() === "" || text === "[]") return [];
                            return JSON.parse(base64UrlDecode(text));
                        });
                });
        }

        function relayWriteQueue(queueArray) {
            const encodedVal = base64UrlEncode(JSON.stringify(queueArray));
            return fetch(RELAY_PRIMARY_WRITE_URL(encodedVal), { method: 'POST' })
                .then(res => { if (!res.ok) throw new Error('primary write status ' + res.status); })
                .catch(err => {
                    console.warn("Primary relay (keyvalue.immanuel.co) write failed, trying fallback (kvdb.io):", err);
                    return fetch(RELAY_FALLBACK_URL, { method: 'PUT', body: encodedVal })
                        .then(res => { if (!res.ok) throw new Error('fallback write status ' + res.status); });
                });
        }

        function updateWordCount() {
            const text = document.getElementById('wish-input').value;
            // Trim whitespace and split into words
            const words = text.trim().split(/\s+/).filter(w => w.length > 0);
            const count = words.length;
            const label = document.getElementById('word-count-lbl');
            const submitBtn = document.getElementById('submit-btn');

            label.innerText = `${count} / 20 words`;

            if (count > 20) {
                label.style.color = '#ff4b4b';
                submitBtn.disabled = true;
                return false;
            } else {
                label.style.color = '#8892b0';
                submitBtn.disabled = false;
                return true;
            }
        }

        function submitPuja() {
            const nameInput = document.getElementById('name-input');
            const offeringSelect = document.getElementById('offering-select');
            const wishInput = document.getElementById('wish-input');
            const standardWishSelect = document.getElementById('standard-wish-select');

            const name = nameInput.value.trim() !== "" ? nameInput.value.trim() : "Anonymous Devotee";
            const offering = offeringSelect.value;
            const wishText = wishInput.value.trim();
            const standardWish = standardWishSelect.value;

            // Double check word count validation
            const words = wishText.split(/\s+/).filter(w => w.length > 0);
            if (words.length > 20) {
                alert("Your prayer must be 20 words or less.");
                return;
            }

            // Construct final prayer text representation
            let prayerText = "";
            if (wishText !== "" && standardWish !== "") {
                prayerText = `${wishText} (Blessing: ${standardWish})`;
            } else if (wishText !== "") {
                prayerText = wishText;
            } else if (standardWish !== "") {
                prayerText = `Blessing: ${standardWish}`;
            }

            const newRequest = {
                id: Date.now().toString() + Math.random().toString(36).substr(2, 5),
                timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
                name: name,
                offering: offering,
                prayer: prayerText
            };

            // 1. Read, append, and save back to shared localStorage namespace (same-device tab test)
            let queue = JSON.parse(localStorage.getItem('ganesha_puja_queue') || '[]');
            queue.push(newRequest);
            localStorage.setItem('ganesha_puja_queue', JSON.stringify(queue));
            localStorage.setItem('ganesha_puja_queue_trigger', Date.now().toString());

            // Disable button during network call
            const submitBtn = document.getElementById('submit-btn');
            submitBtn.disabled = true;

            // 2. Wireless Sync to Network Relay for multi-device sync
            // (tries keyvalue.immanuel.co first, falls back to kvdb.io if
            // that's down - see relayReadQueue()/relayWriteQueue() above)
            relayReadQueue()
                .then(onlineQueue => {
                    if (!Array.isArray(onlineQueue)) onlineQueue = [];

                    // Safety check: Cap queue size at 5 items to fit IIS 1024-character path limit
                    if (onlineQueue.length >= 5) {
                        alert("Bappa is currently receiving many offerings. Please wait a few moments for the Priest to present them, then try again!");
                        submitBtn.disabled = false;
                        return;
                    }

                    onlineQueue.push(newRequest);
                    return relayWriteQueue(onlineQueue).then(() => {
                        // Transition to success screen on success
                        document.getElementById('puja-form').style.display = 'none';
                        document.getElementById('success-card').style.display = 'block';
                    });
                })
                .catch(err => {
                    console.error("Relay sync failed on both primary and fallback services:", err);
                    submitBtn.disabled = false;
                    alert("Network sync failed. Please check your internet connection and try again.");
                });
        }

        function resetForm() {
            document.getElementById('name-input').value = "";
            document.getElementById('wish-input').value = "";
            document.getElementById('standard-wish-select').value = "";
            document.getElementById('word-count-lbl').innerText = "0 / 20 words";
            document.getElementById('word-count-lbl').style.color = '#8892b0';
            document.getElementById('submit-btn').disabled = false;
            
            document.getElementById('success-card').style.display = 'none';
            document.getElementById('puja-form').style.display = 'block';
        }
    </script>
</body>
</html>
)rawliteral";

#endif // PUJA_PAGE_H
