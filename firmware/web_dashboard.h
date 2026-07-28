#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Ganapati AI - Live App Simulation (Advanced Voice & Themes)</title>
    <!-- iOS Web App Meta Tags -->
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="apple-mobile-web-app-title" content="Ganapati AI">
    <!-- App Icons & Manifest -->
    <link rel="manifest" href="manifest.json">
    <link rel="icon" type="image/png" sizes="512x512" href="ganesha.png">
    <link rel="apple-touch-icon" sizes="512x512" href="ganesha.png">
    <style>
        :root {
            --bg-color: #030812;
            --card-bg: rgba(10, 25, 47, 0.65);
            --border-color: rgba(0, 242, 254, 0.25);
            --text-color: #e2f1ff;
            --accent-teal: #00f2fe;
            --accent-blue: #4facfe;
            --accent-gold: #ffd700;
            --accent-green: #00ff87;
            --accent-purple: #ec008c;
            --accent-red: #ff4b4b;
        }

        body {
            font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;
            background: radial-gradient(circle at center, #0a1931 0%, var(--bg-color) 100%);
            color: var(--text-color);
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            overflow-x: hidden;
        }

        /* Phone mockup wrapper */
        .phone-frame {
            width: 100%;
            max-width: 410px;
            background: var(--card-bg);
            border: 2px solid var(--border-color);
            border-radius: 40px;
            padding: 25px;
            box-shadow: 0 25px 50px rgba(0,0,0,0.6), 0 0 30px rgba(0, 242, 254, 0.15);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            position: relative;
        }

        header {
            text-align: center;
            margin-bottom: 15px;
        }

        .peacock-feather {
            width: 40px;
            height: 40px;
            margin: 0 auto 6px;
            background: linear-gradient(135deg, var(--accent-teal), var(--accent-blue));
            clip-path: polygon(50% 0%, 100% 38%, 82% 80%, 50% 100%, 18% 80%, 0% 38%);
            position: relative;
            animation: float 3s ease-in-out infinite;
        }

        .peacock-feather::after {
            content: '';
            position: absolute;
            top: 25%;
            left: 25%;
            width: 50%;
            height: 50%;
            background: radial-gradient(var(--accent-gold), var(--accent-green), transparent);
            border-radius: 50%;
        }

        @keyframes float {
            0%, 100% { transform: translateY(0); }
            50% { transform: translateY(-6px); }
        }

        h1 {
            font-size: 20px;
            margin: 0;
            background: linear-gradient(to right, var(--accent-teal), var(--accent-gold));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            font-weight: 800;
            letter-spacing: 0.5px;
        }

        h2 {
            font-size: 10px;
            color: #8892b0;
            text-transform: uppercase;
            letter-spacing: 2px;
            margin: 1px 0 0 0;
        }

        /* Simulated Hardware Area */
        .hardware-preview {
            background: rgba(2, 10, 23, 0.85);
            border: 1px solid rgba(0, 242, 254, 0.2);
            border-radius: 20px;
            padding: 12px;
            margin-bottom: 15px;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 12px;
        }

        .hardware-title {
            font-size: 9px;
            color: #8892b0;
            text-transform: uppercase;
            letter-spacing: 1.5px;
            align-self: flex-start;
        }

        /* simulated OLED */
        .oled-screen {
            width: 180px;
            height: 80px;
            background-color: #000000;
            border: 2px solid #333333;
            border-radius: 8px;
            position: relative;
            overflow: hidden;
            font-family: 'Segoe UI', Arial, sans-serif;
            padding: 4px;
            box-shadow: inset 0 0 10px rgba(0,242,254,0.3);
        }

        .oled-border {
            border: 1px solid #00f2fe;
            width: calc(100% - 10px);
            height: calc(100% - 10px);
            position: absolute;
            top: 4px;
            left: 4px;
            box-sizing: border-box;
            pointer-events: none;
        }

        .oled-line-top {
            border-bottom: 1px solid #00f2fe;
            width: 100%;
            height: 14px;
            position: absolute;
            top: 0; left: 0;
            font-size: 7px;
            color: #00f2fe;
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0 4px;
            box-sizing: border-box;
        }

        .oled-line-bottom {
            border-top: 1px solid #00f2fe;
            width: 100%;
            height: 14px;
            position: absolute;
            bottom: 0; left: 0;
            font-size: 7px;
            color: #00f2fe;
            display: flex;
            align-items: center;
            padding: 0 4px;
            box-sizing: border-box;
        }

        .oled-main-area {
            position: absolute;
            top: 16px;
            bottom: 16px;
            left: 4px;
            right: 4px;
            display: flex;
            justify-content: center;
            align-items: center;
            overflow: hidden;
        }

        .oled-text {
            color: #00f2fe;
            font-size: 10px;
            white-space: nowrap;
            position: absolute;
            transform: translateX(180px);
        }

        /* simulated LED Ring (Hollow Core - No Om Logo) */
        .led-ring-container {
            width: 90px;
            height: 90px;
            position: relative;
            display: flex;
            justify-content: center;
            align-items: center;
            background: radial-gradient(circle, transparent 35%, rgba(0, 242, 254, 0.05) 70%);
            border-radius: 50%;
        }

        .led-dot {
            width: 6px;
            height: 6px;
            border-radius: 50%;
            background-color: #111;
            position: absolute;
            transition: background-color 0.1s, box-shadow 0.1s;
        }

        /* Controls Section */
        .status-container {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-bottom: 12px;
        }

        .status-card {
            background: rgba(2, 12, 27, 0.5);
            border: 1px solid rgba(0, 242, 254, 0.1);
            border-radius: 12px;
            padding: 8px;
            text-align: center;
        }

        .status-label {
            font-size: 9px;
            color: #8892b0;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 2px;
        }

        .status-val {
            font-size: 13px;
            font-weight: bold;
            color: #ffffff;
        }

        .status-active {
            color: var(--accent-green);
            text-shadow: 0 0 8px rgba(0, 255, 135, 0.3);
        }

        .section-title {
            font-size: 11px;
            color: var(--accent-teal);
            margin: 12px 0 6px 0;
            border-bottom: 1px solid rgba(0, 242, 254, 0.15);
            padding-bottom: 2px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .collapsible-section {
            margin-bottom: 4px;
        }
        .collapsible-section > summary.section-title {
            cursor: pointer;
            list-style: none;
            display: flex;
            align-items: center;
            justify-content: space-between;
            user-select: none;
        }
        .collapsible-section > summary.section-title::-webkit-details-marker {
            display: none;
        }
        .collapsible-section > summary.section-title::after {
            content: "\25BE"; /* ▾ */
            font-size: 10px;
            transition: transform 0.15s ease;
        }
        .collapsible-section[open] > summary.section-title::after {
            transform: rotate(180deg);
        }

        .btn-group {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px;
            margin-bottom: 8px;
        }

        .btn-triple {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 6px;
            margin-bottom: 8px;
        }

        button {
            background: linear-gradient(135deg, rgba(79, 172, 254, 0.1), rgba(0, 242, 254, 0.1));
            border: 1px solid var(--border-color);
            color: #ffffff;
            border-radius: 8px;
            padding: 8px;
            font-size: 12px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            gap: 2px;
        }

        button:hover {
            background: linear-gradient(135deg, rgba(79, 172, 254, 0.25), rgba(0, 242, 254, 0.25));
            border-color: var(--accent-teal);
            transform: translateY(-1px);
        }

        .btn-action {
            background: linear-gradient(135deg, var(--accent-teal), var(--accent-blue));
            color: #030812;
            border: none;
        }

        .btn-action:hover {
            filter: brightness(1.1);
            box-shadow: 0 4px 10px rgba(0, 242, 254, 0.3);
        }

        .btn-stroke {
            background: linear-gradient(135deg, var(--accent-gold), var(--accent-purple));
            color: #030812;
            border: none;
        }
        .btn-stroke:hover {
            filter: brightness(1.1);
            box-shadow: 0 4px 10px rgba(255, 215, 0, 0.3);
        }

        .offering-btn {
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid rgba(255, 255, 255, 0.1);
            transition: all 0.3s ease;
            padding: 4px;
            border-radius: 8px;
            cursor: pointer;
        }
        .offering-btn:hover {
            border-color: rgba(255, 215, 0, 0.4);
            background: rgba(255, 215, 0, 0.05);
        }
        .active-offering {
            border-color: #ffd700 !important;
            background: rgba(255, 215, 0, 0.12) !important;
            box-shadow: 0 0 8px rgba(255, 215, 0, 0.2);
        }

        /* Priest Moderation Queue CSS */
        .queue-item {
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid rgba(0, 242, 254, 0.1);
            border-radius: 8px;
            padding: 8px 10px;
            display: flex;
            flex-direction: column;
            gap: 4px;
            font-size: 11px;
        }
        .queue-item-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            color: var(--accent-gold);
            font-weight: 600;
        }
        .queue-item-time {
            font-size: 8px;
            color: var(--text-muted);
        }
        .queue-item-text {
            color: #ffffff;
            font-style: italic;
            word-wrap: break-word;
            margin: 2px 0;
            text-align: left;
        }
        .queue-item-actions {
            display: flex;
            gap: 6px;
            margin-top: 4px;
        }
        .btn-approve {
            background: linear-gradient(135deg, #4caf50, #2e7d32) !important;
            color: white !important;
            border: none !important;
            border-radius: 4px !important;
            padding: 4px 8px !important;
            font-size: 9px !important;
            font-weight: bold !important;
            cursor: pointer !important;
            flex: 1 !important;
            display: flex !important;
            align-items: center !important;
            justify-content: center !important;
            gap: 2px !important;
            height: auto !important;
            flex-direction: row !important;
        }
        .btn-approve:hover {
            filter: brightness(1.15) !important;
            transform: translateY(-0.5px) !important;
        }
        .btn-reject {
            background: linear-gradient(135deg, #f44336, #c62828) !important;
            color: white !important;
            border: none !important;
            border-radius: 4px !important;
            padding: 4px 8px !important;
            font-size: 9px !important;
            font-weight: bold !important;
            cursor: pointer !important;
            width: auto !important;
            display: flex !important;
            align-items: center !important;
            justify-content: center !important;
            height: auto !important;
            flex-direction: row !important;
        }
        .btn-reject:hover {
            filter: brightness(1.15) !important;
            transform: translateY(-0.5px) !important;
        }

        .btn-stop {
            background: rgba(255, 75, 75, 0.1);
            border: 1px solid rgba(255, 75, 75, 0.3);
            color: #ff4b4b;
        }

        .btn-stop:hover {
            background: rgba(255, 75, 75, 0.25);
            border-color: #ff4b4b;
        }

        .slider-container {
            margin-bottom: 10px;
        }

        .slider-label {
            display: flex;
            justify-content: space-between;
            font-size: 11px;
            margin-bottom: 3px;
            color: #a8b2d1;
        }

        .slider {
            -webkit-appearance: none;
            width: 100%;
            height: 4px;
            border-radius: 2px;
            background: rgba(255, 255, 255, 0.1);
            outline: none;
        }

        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 14px;
            height: 14px;
            border-radius: 50%;
            background: var(--accent-teal);
            cursor: pointer;
            box-shadow: 0 0 6px var(--accent-teal);
        }

        .select-input {
            width: 100%;
            background: rgba(2, 12, 27, 0.8);
            border: 1px solid var(--border-color);
            color: #ffffff;
            border-radius: 8px;
            padding: 8px;
            font-size: 12px;
            outline: none;
            cursor: pointer;
        }

        .toggle-container {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 5px 0;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 36px;
            height: 18px;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider-toggle {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(255, 255, 255, 0.15);
            transition: .4s;
            border-radius: 18px;
        }

        .slider-toggle:before {
            position: absolute;
            content: "";
            height: 12px;
            width: 12px;
            left: 3px;
            bottom: 3px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }

        input:checked + .slider-toggle {
            background-color: var(--accent-teal);
        }

        input:checked + .slider-toggle:before {
            transform: translateX(18px);
        }

        footer {
            text-align: center;
            font-size: 9px;
            color: #8892b0;
            margin-top: 15px;
            border-top: 1px solid rgba(255, 255, 255, 0.05);
            padding-top: 8px;
        }
    </style>
</head>
<body>
    <!-- Admin Passcode Modal Overlay -->
    <div id="passcode-overlay" style="position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: #030812; z-index: 10000; display: flex; flex-direction: column; align-items: center; justify-content: center; font-family: 'Segoe UI', Arial, sans-serif; box-sizing: border-box; padding: 20px;">
        <div style="background: rgba(2, 10, 23, 0.9); border: 1px solid rgba(0, 242, 254, 0.2); border-radius: 20px; padding: 30px; text-align: center; max-width: 320px; width: 100%; box-shadow: 0 10px 30px rgba(0, 242, 254, 0.1); box-sizing: border-box;">
            <div style="font-size: 32px; margin-bottom: 12px;">🔐</div>
            <h2 style="margin: 0 0 8px 0; font-size: 18px; color: #ffd700;">Admin Console</h2>
            <p style="margin: 0 0 20px 0; font-size: 11px; color: #8892b0; line-height: 1.4;">This dashboard is password-protected. Please enter Ganesha's Admin Passcode.</p>
            <input type="password" id="passcode-input" class="select-input" placeholder="Enter PIN..." style="text-align: center; font-size: 16px; letter-spacing: 4px; padding: 8px; margin-bottom: 15px; border-color: rgba(0,242,254,0.3); background: rgba(0,0,0,0.5); border-radius: 8px; color: #fff; width: 100%; box-sizing: border-box;" onkeydown="checkPasscodeEnter(event)">
            <div style="display: flex; gap: 10px; width: 100%;">
                <button class="btn-action" style="flex: 1; padding: 10px; background: linear-gradient(135deg, #00f2fe, #4facfe); color: #030812; font-weight: bold; border: none; border-radius: 8px; cursor: pointer; height: auto; flex-direction: row; display: flex; align-items: center; justify-content: center;" onclick="validatePasscode()">Unlock</button>
                <button class="btn-stop" style="flex: 1; padding: 10px; background: rgba(255,255,255,0.05); color: #8892b0; border: 1px solid rgba(255,255,255,0.1); border-radius: 8px; cursor: pointer; height: auto; flex-direction: row; display: flex; align-items: center; justify-content: center;" onclick="cancelPasscode()">Cancel</button>
            </div>
            <div id="passcode-error" style="color: #ff4b4b; font-size: 10px; margin-top: 10px; display: none;">Invalid Passcode! Please try again.</div>
        </div>
    </div>

    <script>
        function checkPasscodeEnter(e) {
            if (e.key === "Enter") validatePasscode();
        }
        function validatePasscode() {
            const pin = document.getElementById('passcode-input').value;
            if (pin === "1816") {
                sessionStorage.setItem('ganesha_admin_auth', 'true');
                document.getElementById('passcode-overlay').style.display = 'none';
            } else {
                document.getElementById('passcode-error').style.display = 'block';
                document.getElementById('passcode-input').value = '';
            }
        }
        function cancelPasscode() {
            window.location.href = "puja.html";
        }
        // Run authentication check immediately
        if (sessionStorage.getItem('ganesha_admin_auth') === 'true') {
            document.write('<style>#passcode-overlay { display: none !important; }</style>');
            window.addEventListener('DOMContentLoaded', () => {
                document.getElementById('passcode-overlay').style.display = 'none';
            });
        }
    </script>

    <div class="phone-frame">
        <header>
            <div class="peacock-feather"></div>
            <h1>Ganapati AI</h1>
            <h2>Control Hub 2026</h2>
        </header>

        <!-- Live Physical Setup Mockup -->
        <div class="hardware-preview">
            <div class="hardware-title">Live Hardware Simulation</div>
            
            <div class="led-ring-container" id="led-ring">
                <!-- 24 LEDs generated by JS -->
            </div>

            <div class="oled-screen">
                <div class="oled-border">
                    <div class="oled-line-top">
                        <span>GANAPATI AI</span>
                        <span id="oled-state-lbl">[STANDBY]</span>
                    </div>
                    <div class="oled-main-area">
                        <div class="oled-text" id="oled-scroller">Welcome!</div>
                    </div>
                    <div class="oled-line-bottom">
                        <span id="oled-hits-lbl">Devotional Hits: 0</span>
                    </div>
                </div>
            </div>
        </div>

        <!-- UI App Controls -->
        <div class="status-container">
            <div class="status-card">
                <div class="status-label">System State</div>
                <div id="state-val" class="status-val">STANDBY</div>
            </div>
            <div class="status-card">
                <div class="status-label">Blessings Today</div>
                <div id="count-val" class="status-val">0</div>
            </div>
        </div>

        <div class="section-title">🎵 Now Playing</div>
        <div id="now-playing-panel" style="background: rgba(2,12,27,0.3); border-radius: 10px; padding: 10px; border: 1px solid var(--border-color); margin-bottom: 12px;">
            <div id="now-playing-track" style="font-size: 12px; color: #fff; margin-bottom: 6px;">Nothing playing</div>
            <div style="background: rgba(255,255,255,0.08); border-radius: 6px; height: 6px; overflow: hidden; margin-bottom: 4px;">
                <div id="now-playing-bar" style="background: linear-gradient(90deg, var(--accent-teal), #ffd700); height: 100%; width: 0%; transition: width 0.4s linear;"></div>
            </div>
            <div style="display: flex; justify-content: space-between; font-size: 10px; color: #8892b0;">
                <span id="now-playing-elapsed">0:00</span>
                <span id="now-playing-total">0:00</span>
            </div>
        </div>

        <details class="collapsible-section">
        <summary class="section-title">📜 Song Library</summary>
        <div id="song-library-list" style="max-height: 220px; overflow-y: auto; background: rgba(2,12,27,0.3); border-radius: 10px; padding: 6px; border: 1px solid var(--border-color); margin-bottom: 12px;">
            <!-- populated by renderSongLibrary() -->
        </div>
        </details>

        <!-- Advanced Pitch Simulator Config -->
        <div style="background: rgba(2,12,27,0.3); border-radius: 10px; padding: 8px; margin-bottom: 12px; border: 1px dashed var(--border-color);">
            <div class="slider-label" style="font-weight: 600; font-size: 10px; color: var(--accent-teal);">AI MIC SETTINGS (SIMULATION)</div>
            <div style="display: flex; gap: 8px; margin-top: 4px;">
                <div style="flex: 1;">
                    <span style="font-size: 8px; text-transform: uppercase; color: #8892b0;">Devotee Pitch</span>
                    <select id="voice-pitch-select" class="select-input" style="padding: 4px; font-size: 10px;">
                        <option value="child">Child (High Pitch, >240Hz)</option>
                        <option value="adult" selected>Adult (Low Pitch, <240Hz)</option>
                    </select>
                </div>
                <div style="flex: 1;">
                    <span style="font-size: 8px; text-transform: uppercase; color: #8892b0;">Active Language</span>
                    <select id="lang-select" class="select-input" style="padding: 4px; font-size: 10px;" onchange="updateLanguage()">
                        <option value="en">English</option>
                        <option value="sa">Sanskrit / Hindi</option>
                        <option value="mr">Marathi</option>
                        <option value="ta">Tamil</option>
                    </select>
                </div>
            </div>
            <div style="margin-top: 8px; padding-top: 8px; border-top: 1px dashed var(--border-color); display: flex; justify-content: space-between; align-items: center;">
                <div>
                    <span style="font-size: 8px; text-transform: uppercase; color: #8892b0;">One-Directional Mic</span><br>
                    <span id="mic-status" style="font-size: 9px; color: var(--accent-teal);">Off</span>
                </div>
                <label class="switch">
                    <input type="checkbox" id="mic-toggle" onchange="toggleMic()">
                    <span class="slider round"></span>
                </label>
            </div>
        </div>

        <div class="section-title">Manual Triggers</div>
        <div class="btn-group">
            <button class="btn-action" id="mouse-pad-btn"
                onmousedown="onMousePadDown()" onmouseup="onMousePadUp()" onmouseleave="onMousePadCancel()"
                ontouchstart="onMousePadDown()" ontouchend="onMousePadUp()" ontouchcancel="onMousePadCancel()">
                <span>🐭 Mouse Back</span>
                <span style="font-size: 8px; font-weight: normal;">(Mantra: 30s)</span>
            </button>
            <button class="btn-action" id="feet-pad-btn"
                onmousedown="onFeetPadDown()" onmouseup="onFeetPadUp()" onmouseleave="onFeetPadCancel()"
                ontouchstart="onFeetPadDown()" ontouchend="onFeetPadUp()" ontouchcancel="onFeetPadCancel()">
                <span>👣 Feet Touch</span>
                <span style="font-size: 8px; font-weight: normal;">(Ganesha Mantras: 30s)</span>
            </button>
        </div>
        <div style="font-size: 10px; opacity: 0.6; margin: -6px 0 10px 2px;">Hold both pads together for 15s to close the temple for the night</div>
        <div style="display: flex; gap: 8px; margin-bottom: 8px;">
            <button class="btn-stop" style="flex: 1;" onclick="triggerStop()">⏹️ Stop Sound / Reset State</button>
        </div>
        <div style="display: flex; gap: 8px; margin-bottom: 12px;">
            <button class="btn-stop" style="flex: 1;" onclick="closeTemple()">🔔 Close Temple (Night Mode)</button>
            <button class="btn-action" style="flex: 1;" onclick="openTemple()">🌅 Open Temple</button>
        </div>

        <details class="collapsible-section">
        <summary class="section-title">🌸 Virtual Puja & Prayers</summary>
        <div style="background: rgba(255, 215, 0, 0.05); border: 1px solid rgba(255, 215, 0, 0.2); border-radius: 12px; padding: 10px; margin-bottom: 12px; box-shadow: 0 0 10px rgba(255, 215, 0, 0.05);">
            <input type="text" id="puja-name-input" class="select-input" placeholder="Devotee Name (Optional)..." style="font-size: 11px; padding: 6px; margin-bottom: 6px; background: rgba(0,0,0,0.4); border-color: rgba(255, 215, 0, 0.2); width: 100%; box-sizing: border-box; border-radius: 6px; color: #fff;">
            <input type="text" id="puja-input" class="select-input" placeholder="Type a prayer or wish here..." style="font-size: 11px; padding: 6px; margin-bottom: 8px; background: rgba(0,0,0,0.4); border-color: rgba(255, 215, 0, 0.2); width: 100%; box-sizing: border-box; border-radius: 6px; color: #fff;">

            <div style="font-size: 8px; text-transform: uppercase; color: #8892b0; margin-bottom: 4px;">Choose an offering:</div>
            <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; margin-bottom: 8px;">
                <button id="opt-hibiscus" class="offering-btn active-offering" onclick="selectOffering('hibiscus')">
                    <span style="font-size: 16px;">🌺</span>
                    <span style="font-size: 7px; font-weight: normal; color: #a8b2d1;">Flower</span>
                </button>
                <button id="opt-garland" class="offering-btn" onclick="selectOffering('garland')">
                    <span style="font-size: 16px;">🌼</span>
                    <span style="font-size: 7px; font-weight: normal; color: #a8b2d1;">Garland</span>
                </button>
                <button id="opt-modak" class="offering-btn" onclick="selectOffering('modak')">
                    <span style="font-size: 16px;">🥟</span>
                    <span style="font-size: 7px; font-weight: normal; color: #a8b2d1;">Modak</span>
                </button>
                <button id="opt-coconut" class="offering-btn" onclick="selectOffering('coconut')">
                    <span style="font-size: 16px;">🥥</span>
                    <span style="font-size: 7px; font-weight: normal; color: #a8b2d1;">Coconut</span>
                </button>
            </div>

            <button class="btn-action" style="width: 100%; padding: 8px; background: linear-gradient(135deg, #ffd700, #ffa500); color: #050d1a; font-weight: bold; border-color: #ffd700; border-radius: 8px; display: flex; flex-direction: row; gap: 4px; align-items: center; justify-content: center; height: auto;" onclick="submitPuja()">
                <span>✨ Offer Prasad & Send Prayer ✨</span>
            </button>
        </div>
        </details>

        <details class="collapsible-section">
        <summary class="section-title">📱 Devotee QR Code</summary>
        <div style="background: rgba(255, 215, 0, 0.05); border: 1px solid rgba(255, 215, 0, 0.2); border-radius: 12px; padding: 14px; margin-bottom: 12px; text-align: center;">
            <!-- padding must provide a real QR "quiet zone" (>= 4 modules of
                 blank margin) or phone cameras may fail to scan it, even
                 though it looks fine on screen - at 160px/~25 modules that's
                 ~25px+ per side, not a token few pixels. -->
            <div id="qr-code-container" style="display: inline-block; background: #fff; padding: 28px; border-radius: 8px; line-height: 0;"></div>
            <div id="qr-code-status" style="font-size: 10px; color: var(--text-muted); margin-top: 8px;">Devotees scan this to submit their own offering &amp; prayer from their phone.</div>
        </div>
        </details>

        <details class="collapsible-section">
        <summary class="section-title">🕌 Priest Queue</summary>
        <div style="background: rgba(255, 215, 0, 0.05); border: 1px solid rgba(255, 215, 0, 0.2); border-radius: 12px; padding: 10px; margin-bottom: 12px; box-shadow: 0 0 10px rgba(255, 215, 0, 0.05);">
            <div id="queue-status" style="font-size: 10px; color: var(--text-muted); margin-bottom: 8px; text-align: left;">No pending offerings.</div>
            <div id="queue-list" style="display: flex; flex-direction: column; gap: 8px; max-height: 250px; overflow-y: auto; margin-bottom: 8px;">
                <!-- Queue items will be generated here dynamically -->
            </div>
            <div style="display: flex; gap: 8px;">
                <button class="btn-stop" style="flex: 1; padding: 6px; font-size: 10px; border-color: rgba(255,255,255,0.1); background: rgba(255,255,255,0.03); border-radius: 6px; flex-direction: row; gap: 4px; align-items: center; justify-content: center; height: auto;" onclick="clearAllQueue()">
                    <span>🧹</span> <span>Clear Queue</span>
                </button>
            </div>
        </div>
        </details>

        <details class="collapsible-section">
        <summary class="section-title">Theme of the Day</summary>
        <div style="margin-bottom: 12px;">
            <select id="theme-select" class="select-input" onchange="updateTheme()">
                <option value="tue">Tuesday: Ganesha Theme (Maroon & Gold)</option>
                <option value="mon">Monday: Shiva Theme (Ice Blue & Cyan)</option>
                <option value="wed">Wednesday: Wisdom Theme (Peacock Teal)</option>
                <option value="thu">Thursday: Guru Theme (Bright Gold/Yellow)</option>
                <option value="fri">Friday: Shakti Theme (Lotus Pink & Violet)</option>
                <option value="sat">Saturday: Discipline Theme (Indigo & Purple)</option>
                <option value="sun">Sunday: Sun Theme (Ruby Red & Gold)</option>
            </select>
        </div>
        </details>

        <details class="collapsible-section">
        <summary class="section-title">LED Customization</summary>
        <div class="slider-container">
            <div class="slider-label">
                <span>Brightness</span>
                <span id="bright-lbl">60%</span>
            </div>
            <input type="range" id="bright-slider" class="slider" min="0" max="255" value="150" oninput="updateBrightness()">
        </div>
        <div style="margin-bottom: 12px;">
            <div class="slider-label">Default LED Pattern</div>
            <select id="pattern-select" class="select-input" onchange="updatePattern()">
                <option value="0">Peacock Wave</option>
                <option value="1">Circuit Pulse</option>
                <option value="2">Golden Aura</option>
                <option value="3">Rainbow Dream</option>
                <option value="4">Diya Flicker (Warm)</option>
            </select>
        </div>
        </details>

        <details class="collapsible-section">
        <summary class="section-title">Audio Settings</summary>
        <div class="slider-container" style="margin-bottom: 12px;">
            <div class="slider-label">
                <span>Volume</span>
                <span id="vol-lbl">15</span>
            </div>
            <input type="range" id="vol-slider" class="slider" min="0" max="30" value="15" oninput="updateVolume()">
        </div>
        </details>

        <details class="collapsible-section" open>
        <summary class="section-title">System Settings</summary>
        <div class="toggle-container">
            <span style="font-size: 12px; color: #a8b2d1;">PIR Motion Detector</span>
            <label class="switch">
                <input type="checkbox" id="pir-toggle" checked onchange="togglePIR()">
                <span class="slider-toggle"></span>
            </label>
        </div>

        <div class="toggle-container" style="margin-bottom: 5px;">
            <span style="font-size: 12px; color: #a8b2d1;">Simulate PIR Detection</span>
            <button style="padding: 2px 6px; font-size: 9px;" onclick="triggerPIR()">Trigger PIR</button>
        </div>
        </details>

        <footer>
            Lord of Wisdom &bull; 2026 Theme
        </footer>
    </div>

    <!-- QRCode for JavaScript library (MIT licensed, Copyright (c) 2009 Kazuhiko Arase,
         via davidshimjs/qrcodejs) - generates the Devotee QR Code below client-side,
         no external network dependency. -->
    <script>
/**
 * @fileoverview
 * - Using the 'QRCode for Javascript library'
 * - Fixed dataset of 'QRCode for Javascript library' for support full-spec.
 * - this library has no dependencies.
 * 
 * @author davidshimjs
 * @see <a href="http://www.d-project.com/" target="_blank">http://www.d-project.com/</a>
 * @see <a href="http://jeromeetienne.github.com/jquery-qrcode/" target="_blank">http://jeromeetienne.github.com/jquery-qrcode/</a>
 */
var QRCode;

(function () {
	//---------------------------------------------------------------------
	// QRCode for JavaScript
	//
	// Copyright (c) 2009 Kazuhiko Arase
	//
	// URL: http://www.d-project.com/
	//
	// Licensed under the MIT license:
	//   http://www.opensource.org/licenses/mit-license.php
	//
	// The word "QR Code" is registered trademark of 
	// DENSO WAVE INCORPORATED
	//   http://www.denso-wave.com/qrcode/faqpatent-e.html
	//
	//---------------------------------------------------------------------
	function QR8bitByte(data) {
		this.mode = QRMode.MODE_8BIT_BYTE;
		this.data = data;
		this.parsedData = [];

		// Added to support UTF-8 Characters
		for (var i = 0, l = this.data.length; i < l; i++) {
			var byteArray = [];
			var code = this.data.charCodeAt(i);

			if (code > 0x10000) {
				byteArray[0] = 0xF0 | ((code & 0x1C0000) >>> 18);
				byteArray[1] = 0x80 | ((code & 0x3F000) >>> 12);
				byteArray[2] = 0x80 | ((code & 0xFC0) >>> 6);
				byteArray[3] = 0x80 | (code & 0x3F);
			} else if (code > 0x800) {
				byteArray[0] = 0xE0 | ((code & 0xF000) >>> 12);
				byteArray[1] = 0x80 | ((code & 0xFC0) >>> 6);
				byteArray[2] = 0x80 | (code & 0x3F);
			} else if (code > 0x80) {
				byteArray[0] = 0xC0 | ((code & 0x7C0) >>> 6);
				byteArray[1] = 0x80 | (code & 0x3F);
			} else {
				byteArray[0] = code;
			}

			this.parsedData.push(byteArray);
		}

		this.parsedData = Array.prototype.concat.apply([], this.parsedData);

		if (this.parsedData.length != this.data.length) {
			this.parsedData.unshift(191);
			this.parsedData.unshift(187);
			this.parsedData.unshift(239);
		}
	}

	QR8bitByte.prototype = {
		getLength: function (buffer) {
			return this.parsedData.length;
		},
		write: function (buffer) {
			for (var i = 0, l = this.parsedData.length; i < l; i++) {
				buffer.put(this.parsedData[i], 8);
			}
		}
	};

	function QRCodeModel(typeNumber, errorCorrectLevel) {
		this.typeNumber = typeNumber;
		this.errorCorrectLevel = errorCorrectLevel;
		this.modules = null;
		this.moduleCount = 0;
		this.dataCache = null;
		this.dataList = [];
	}

	QRCodeModel.prototype={addData:function(data){var newData=new QR8bitByte(data);this.dataList.push(newData);this.dataCache=null;},isDark:function(row,col){if(row<0||this.moduleCount<=row||col<0||this.moduleCount<=col){throw new Error(row+","+col);}
	return this.modules[row][col];},getModuleCount:function(){return this.moduleCount;},make:function(){this.makeImpl(false,this.getBestMaskPattern());},makeImpl:function(test,maskPattern){this.moduleCount=this.typeNumber*4+17;this.modules=new Array(this.moduleCount);for(var row=0;row<this.moduleCount;row++){this.modules[row]=new Array(this.moduleCount);for(var col=0;col<this.moduleCount;col++){this.modules[row][col]=null;}}
	this.setupPositionProbePattern(0,0);this.setupPositionProbePattern(this.moduleCount-7,0);this.setupPositionProbePattern(0,this.moduleCount-7);this.setupPositionAdjustPattern();this.setupTimingPattern();this.setupTypeInfo(test,maskPattern);if(this.typeNumber>=7){this.setupTypeNumber(test);}
	if(this.dataCache==null){this.dataCache=QRCodeModel.createData(this.typeNumber,this.errorCorrectLevel,this.dataList);}
	this.mapData(this.dataCache,maskPattern);},setupPositionProbePattern:function(row,col){for(var r=-1;r<=7;r++){if(row+r<=-1||this.moduleCount<=row+r)continue;for(var c=-1;c<=7;c++){if(col+c<=-1||this.moduleCount<=col+c)continue;if((0<=r&&r<=6&&(c==0||c==6))||(0<=c&&c<=6&&(r==0||r==6))||(2<=r&&r<=4&&2<=c&&c<=4)){this.modules[row+r][col+c]=true;}else{this.modules[row+r][col+c]=false;}}}},getBestMaskPattern:function(){var minLostPoint=0;var pattern=0;for(var i=0;i<8;i++){this.makeImpl(true,i);var lostPoint=QRUtil.getLostPoint(this);if(i==0||minLostPoint>lostPoint){minLostPoint=lostPoint;pattern=i;}}
	return pattern;},createMovieClip:function(target_mc,instance_name,depth){var qr_mc=target_mc.createEmptyMovieClip(instance_name,depth);var cs=1;this.make();for(var row=0;row<this.modules.length;row++){var y=row*cs;for(var col=0;col<this.modules[row].length;col++){var x=col*cs;var dark=this.modules[row][col];if(dark){qr_mc.beginFill(0,100);qr_mc.moveTo(x,y);qr_mc.lineTo(x+cs,y);qr_mc.lineTo(x+cs,y+cs);qr_mc.lineTo(x,y+cs);qr_mc.endFill();}}}
	return qr_mc;},setupTimingPattern:function(){for(var r=8;r<this.moduleCount-8;r++){if(this.modules[r][6]!=null){continue;}
	this.modules[r][6]=(r%2==0);}
	for(var c=8;c<this.moduleCount-8;c++){if(this.modules[6][c]!=null){continue;}
	this.modules[6][c]=(c%2==0);}},setupPositionAdjustPattern:function(){var pos=QRUtil.getPatternPosition(this.typeNumber);for(var i=0;i<pos.length;i++){for(var j=0;j<pos.length;j++){var row=pos[i];var col=pos[j];if(this.modules[row][col]!=null){continue;}
	for(var r=-2;r<=2;r++){for(var c=-2;c<=2;c++){if(r==-2||r==2||c==-2||c==2||(r==0&&c==0)){this.modules[row+r][col+c]=true;}else{this.modules[row+r][col+c]=false;}}}}}},setupTypeNumber:function(test){var bits=QRUtil.getBCHTypeNumber(this.typeNumber);for(var i=0;i<18;i++){var mod=(!test&&((bits>>i)&1)==1);this.modules[Math.floor(i/3)][i%3+this.moduleCount-8-3]=mod;}
	for(var i=0;i<18;i++){var mod=(!test&&((bits>>i)&1)==1);this.modules[i%3+this.moduleCount-8-3][Math.floor(i/3)]=mod;}},setupTypeInfo:function(test,maskPattern){var data=(this.errorCorrectLevel<<3)|maskPattern;var bits=QRUtil.getBCHTypeInfo(data);for(var i=0;i<15;i++){var mod=(!test&&((bits>>i)&1)==1);if(i<6){this.modules[i][8]=mod;}else if(i<8){this.modules[i+1][8]=mod;}else{this.modules[this.moduleCount-15+i][8]=mod;}}
	for(var i=0;i<15;i++){var mod=(!test&&((bits>>i)&1)==1);if(i<8){this.modules[8][this.moduleCount-i-1]=mod;}else if(i<9){this.modules[8][15-i-1+1]=mod;}else{this.modules[8][15-i-1]=mod;}}
	this.modules[this.moduleCount-8][8]=(!test);},mapData:function(data,maskPattern){var inc=-1;var row=this.moduleCount-1;var bitIndex=7;var byteIndex=0;for(var col=this.moduleCount-1;col>0;col-=2){if(col==6)col--;while(true){for(var c=0;c<2;c++){if(this.modules[row][col-c]==null){var dark=false;if(byteIndex<data.length){dark=(((data[byteIndex]>>>bitIndex)&1)==1);}
	var mask=QRUtil.getMask(maskPattern,row,col-c);if(mask){dark=!dark;}
	this.modules[row][col-c]=dark;bitIndex--;if(bitIndex==-1){byteIndex++;bitIndex=7;}}}
	row+=inc;if(row<0||this.moduleCount<=row){row-=inc;inc=-inc;break;}}}}};QRCodeModel.PAD0=0xEC;QRCodeModel.PAD1=0x11;QRCodeModel.createData=function(typeNumber,errorCorrectLevel,dataList){var rsBlocks=QRRSBlock.getRSBlocks(typeNumber,errorCorrectLevel);var buffer=new QRBitBuffer();for(var i=0;i<dataList.length;i++){var data=dataList[i];buffer.put(data.mode,4);buffer.put(data.getLength(),QRUtil.getLengthInBits(data.mode,typeNumber));data.write(buffer);}
	var totalDataCount=0;for(var i=0;i<rsBlocks.length;i++){totalDataCount+=rsBlocks[i].dataCount;}
	if(buffer.getLengthInBits()>totalDataCount*8){throw new Error("code length overflow. ("
	+buffer.getLengthInBits()
	+">"
	+totalDataCount*8
	+")");}
	if(buffer.getLengthInBits()+4<=totalDataCount*8){buffer.put(0,4);}
	while(buffer.getLengthInBits()%8!=0){buffer.putBit(false);}
	while(true){if(buffer.getLengthInBits()>=totalDataCount*8){break;}
	buffer.put(QRCodeModel.PAD0,8);if(buffer.getLengthInBits()>=totalDataCount*8){break;}
	buffer.put(QRCodeModel.PAD1,8);}
	return QRCodeModel.createBytes(buffer,rsBlocks);};QRCodeModel.createBytes=function(buffer,rsBlocks){var offset=0;var maxDcCount=0;var maxEcCount=0;var dcdata=new Array(rsBlocks.length);var ecdata=new Array(rsBlocks.length);for(var r=0;r<rsBlocks.length;r++){var dcCount=rsBlocks[r].dataCount;var ecCount=rsBlocks[r].totalCount-dcCount;maxDcCount=Math.max(maxDcCount,dcCount);maxEcCount=Math.max(maxEcCount,ecCount);dcdata[r]=new Array(dcCount);for(var i=0;i<dcdata[r].length;i++){dcdata[r][i]=0xff&buffer.buffer[i+offset];}
	offset+=dcCount;var rsPoly=QRUtil.getErrorCorrectPolynomial(ecCount);var rawPoly=new QRPolynomial(dcdata[r],rsPoly.getLength()-1);var modPoly=rawPoly.mod(rsPoly);ecdata[r]=new Array(rsPoly.getLength()-1);for(var i=0;i<ecdata[r].length;i++){var modIndex=i+modPoly.getLength()-ecdata[r].length;ecdata[r][i]=(modIndex>=0)?modPoly.get(modIndex):0;}}
	var totalCodeCount=0;for(var i=0;i<rsBlocks.length;i++){totalCodeCount+=rsBlocks[i].totalCount;}
	var data=new Array(totalCodeCount);var index=0;for(var i=0;i<maxDcCount;i++){for(var r=0;r<rsBlocks.length;r++){if(i<dcdata[r].length){data[index++]=dcdata[r][i];}}}
	for(var i=0;i<maxEcCount;i++){for(var r=0;r<rsBlocks.length;r++){if(i<ecdata[r].length){data[index++]=ecdata[r][i];}}}
	return data;};var QRMode={MODE_NUMBER:1<<0,MODE_ALPHA_NUM:1<<1,MODE_8BIT_BYTE:1<<2,MODE_KANJI:1<<3};var QRErrorCorrectLevel={L:1,M:0,Q:3,H:2};var QRMaskPattern={PATTERN000:0,PATTERN001:1,PATTERN010:2,PATTERN011:3,PATTERN100:4,PATTERN101:5,PATTERN110:6,PATTERN111:7};var QRUtil={PATTERN_POSITION_TABLE:[[],[6,18],[6,22],[6,26],[6,30],[6,34],[6,22,38],[6,24,42],[6,26,46],[6,28,50],[6,30,54],[6,32,58],[6,34,62],[6,26,46,66],[6,26,48,70],[6,26,50,74],[6,30,54,78],[6,30,56,82],[6,30,58,86],[6,34,62,90],[6,28,50,72,94],[6,26,50,74,98],[6,30,54,78,102],[6,28,54,80,106],[6,32,58,84,110],[6,30,58,86,114],[6,34,62,90,118],[6,26,50,74,98,122],[6,30,54,78,102,126],[6,26,52,78,104,130],[6,30,56,82,108,134],[6,34,60,86,112,138],[6,30,58,86,114,142],[6,34,62,90,118,146],[6,30,54,78,102,126,150],[6,24,50,76,102,128,154],[6,28,54,80,106,132,158],[6,32,58,84,110,136,162],[6,26,54,82,110,138,166],[6,30,58,86,114,142,170]],G15:(1<<10)|(1<<8)|(1<<5)|(1<<4)|(1<<2)|(1<<1)|(1<<0),G18:(1<<12)|(1<<11)|(1<<10)|(1<<9)|(1<<8)|(1<<5)|(1<<2)|(1<<0),G15_MASK:(1<<14)|(1<<12)|(1<<10)|(1<<4)|(1<<1),getBCHTypeInfo:function(data){var d=data<<10;while(QRUtil.getBCHDigit(d)-QRUtil.getBCHDigit(QRUtil.G15)>=0){d^=(QRUtil.G15<<(QRUtil.getBCHDigit(d)-QRUtil.getBCHDigit(QRUtil.G15)));}
	return((data<<10)|d)^QRUtil.G15_MASK;},getBCHTypeNumber:function(data){var d=data<<12;while(QRUtil.getBCHDigit(d)-QRUtil.getBCHDigit(QRUtil.G18)>=0){d^=(QRUtil.G18<<(QRUtil.getBCHDigit(d)-QRUtil.getBCHDigit(QRUtil.G18)));}
	return(data<<12)|d;},getBCHDigit:function(data){var digit=0;while(data!=0){digit++;data>>>=1;}
	return digit;},getPatternPosition:function(typeNumber){return QRUtil.PATTERN_POSITION_TABLE[typeNumber-1];},getMask:function(maskPattern,i,j){switch(maskPattern){case QRMaskPattern.PATTERN000:return(i+j)%2==0;case QRMaskPattern.PATTERN001:return i%2==0;case QRMaskPattern.PATTERN010:return j%3==0;case QRMaskPattern.PATTERN011:return(i+j)%3==0;case QRMaskPattern.PATTERN100:return(Math.floor(i/2)+Math.floor(j/3))%2==0;case QRMaskPattern.PATTERN101:return(i*j)%2+(i*j)%3==0;case QRMaskPattern.PATTERN110:return((i*j)%2+(i*j)%3)%2==0;case QRMaskPattern.PATTERN111:return((i*j)%3+(i+j)%2)%2==0;default:throw new Error("bad maskPattern:"+maskPattern);}},getErrorCorrectPolynomial:function(errorCorrectLength){var a=new QRPolynomial([1],0);for(var i=0;i<errorCorrectLength;i++){a=a.multiply(new QRPolynomial([1,QRMath.gexp(i)],0));}
	return a;},getLengthInBits:function(mode,type){if(1<=type&&type<10){switch(mode){case QRMode.MODE_NUMBER:return 10;case QRMode.MODE_ALPHA_NUM:return 9;case QRMode.MODE_8BIT_BYTE:return 8;case QRMode.MODE_KANJI:return 8;default:throw new Error("mode:"+mode);}}else if(type<27){switch(mode){case QRMode.MODE_NUMBER:return 12;case QRMode.MODE_ALPHA_NUM:return 11;case QRMode.MODE_8BIT_BYTE:return 16;case QRMode.MODE_KANJI:return 10;default:throw new Error("mode:"+mode);}}else if(type<41){switch(mode){case QRMode.MODE_NUMBER:return 14;case QRMode.MODE_ALPHA_NUM:return 13;case QRMode.MODE_8BIT_BYTE:return 16;case QRMode.MODE_KANJI:return 12;default:throw new Error("mode:"+mode);}}else{throw new Error("type:"+type);}},getLostPoint:function(qrCode){var moduleCount=qrCode.getModuleCount();var lostPoint=0;for(var row=0;row<moduleCount;row++){for(var col=0;col<moduleCount;col++){var sameCount=0;var dark=qrCode.isDark(row,col);for(var r=-1;r<=1;r++){if(row+r<0||moduleCount<=row+r){continue;}
	for(var c=-1;c<=1;c++){if(col+c<0||moduleCount<=col+c){continue;}
	if(r==0&&c==0){continue;}
	if(dark==qrCode.isDark(row+r,col+c)){sameCount++;}}}
	if(sameCount>5){lostPoint+=(3+sameCount-5);}}}
	for(var row=0;row<moduleCount-1;row++){for(var col=0;col<moduleCount-1;col++){var count=0;if(qrCode.isDark(row,col))count++;if(qrCode.isDark(row+1,col))count++;if(qrCode.isDark(row,col+1))count++;if(qrCode.isDark(row+1,col+1))count++;if(count==0||count==4){lostPoint+=3;}}}
	for(var row=0;row<moduleCount;row++){for(var col=0;col<moduleCount-6;col++){if(qrCode.isDark(row,col)&&!qrCode.isDark(row,col+1)&&qrCode.isDark(row,col+2)&&qrCode.isDark(row,col+3)&&qrCode.isDark(row,col+4)&&!qrCode.isDark(row,col+5)&&qrCode.isDark(row,col+6)){lostPoint+=40;}}}
	for(var col=0;col<moduleCount;col++){for(var row=0;row<moduleCount-6;row++){if(qrCode.isDark(row,col)&&!qrCode.isDark(row+1,col)&&qrCode.isDark(row+2,col)&&qrCode.isDark(row+3,col)&&qrCode.isDark(row+4,col)&&!qrCode.isDark(row+5,col)&&qrCode.isDark(row+6,col)){lostPoint+=40;}}}
	var darkCount=0;for(var col=0;col<moduleCount;col++){for(var row=0;row<moduleCount;row++){if(qrCode.isDark(row,col)){darkCount++;}}}
	var ratio=Math.abs(100*darkCount/moduleCount/moduleCount-50)/5;lostPoint+=ratio*10;return lostPoint;}};var QRMath={glog:function(n){if(n<1){throw new Error("glog("+n+")");}
	return QRMath.LOG_TABLE[n];},gexp:function(n){while(n<0){n+=255;}
	while(n>=256){n-=255;}
	return QRMath.EXP_TABLE[n];},EXP_TABLE:new Array(256),LOG_TABLE:new Array(256)};for(var i=0;i<8;i++){QRMath.EXP_TABLE[i]=1<<i;}
	for(var i=8;i<256;i++){QRMath.EXP_TABLE[i]=QRMath.EXP_TABLE[i-4]^QRMath.EXP_TABLE[i-5]^QRMath.EXP_TABLE[i-6]^QRMath.EXP_TABLE[i-8];}
	for(var i=0;i<255;i++){QRMath.LOG_TABLE[QRMath.EXP_TABLE[i]]=i;}
	function QRPolynomial(num,shift){if(num.length==undefined){throw new Error(num.length+"/"+shift);}
	var offset=0;while(offset<num.length&&num[offset]==0){offset++;}
	this.num=new Array(num.length-offset+shift);for(var i=0;i<num.length-offset;i++){this.num[i]=num[i+offset];}}
	QRPolynomial.prototype={get:function(index){return this.num[index];},getLength:function(){return this.num.length;},multiply:function(e){var num=new Array(this.getLength()+e.getLength()-1);for(var i=0;i<this.getLength();i++){for(var j=0;j<e.getLength();j++){num[i+j]^=QRMath.gexp(QRMath.glog(this.get(i))+QRMath.glog(e.get(j)));}}
	return new QRPolynomial(num,0);},mod:function(e){if(this.getLength()-e.getLength()<0){return this;}
	var ratio=QRMath.glog(this.get(0))-QRMath.glog(e.get(0));var num=new Array(this.getLength());for(var i=0;i<this.getLength();i++){num[i]=this.get(i);}
	for(var i=0;i<e.getLength();i++){num[i]^=QRMath.gexp(QRMath.glog(e.get(i))+ratio);}
	return new QRPolynomial(num,0).mod(e);}};function QRRSBlock(totalCount,dataCount){this.totalCount=totalCount;this.dataCount=dataCount;}
	QRRSBlock.RS_BLOCK_TABLE=[[1,26,19],[1,26,16],[1,26,13],[1,26,9],[1,44,34],[1,44,28],[1,44,22],[1,44,16],[1,70,55],[1,70,44],[2,35,17],[2,35,13],[1,100,80],[2,50,32],[2,50,24],[4,25,9],[1,134,108],[2,67,43],[2,33,15,2,34,16],[2,33,11,2,34,12],[2,86,68],[4,43,27],[4,43,19],[4,43,15],[2,98,78],[4,49,31],[2,32,14,4,33,15],[4,39,13,1,40,14],[2,121,97],[2,60,38,2,61,39],[4,40,18,2,41,19],[4,40,14,2,41,15],[2,146,116],[3,58,36,2,59,37],[4,36,16,4,37,17],[4,36,12,4,37,13],[2,86,68,2,87,69],[4,69,43,1,70,44],[6,43,19,2,44,20],[6,43,15,2,44,16],[4,101,81],[1,80,50,4,81,51],[4,50,22,4,51,23],[3,36,12,8,37,13],[2,116,92,2,117,93],[6,58,36,2,59,37],[4,46,20,6,47,21],[7,42,14,4,43,15],[4,133,107],[8,59,37,1,60,38],[8,44,20,4,45,21],[12,33,11,4,34,12],[3,145,115,1,146,116],[4,64,40,5,65,41],[11,36,16,5,37,17],[11,36,12,5,37,13],[5,109,87,1,110,88],[5,65,41,5,66,42],[5,54,24,7,55,25],[11,36,12],[5,122,98,1,123,99],[7,73,45,3,74,46],[15,43,19,2,44,20],[3,45,15,13,46,16],[1,135,107,5,136,108],[10,74,46,1,75,47],[1,50,22,15,51,23],[2,42,14,17,43,15],[5,150,120,1,151,121],[9,69,43,4,70,44],[17,50,22,1,51,23],[2,42,14,19,43,15],[3,141,113,4,142,114],[3,70,44,11,71,45],[17,47,21,4,48,22],[9,39,13,16,40,14],[3,135,107,5,136,108],[3,67,41,13,68,42],[15,54,24,5,55,25],[15,43,15,10,44,16],[4,144,116,4,145,117],[17,68,42],[17,50,22,6,51,23],[19,46,16,6,47,17],[2,139,111,7,140,112],[17,74,46],[7,54,24,16,55,25],[34,37,13],[4,151,121,5,152,122],[4,75,47,14,76,48],[11,54,24,14,55,25],[16,45,15,14,46,16],[6,147,117,4,148,118],[6,73,45,14,74,46],[11,54,24,16,55,25],[30,46,16,2,47,17],[8,132,106,4,133,107],[8,75,47,13,76,48],[7,54,24,22,55,25],[22,45,15,13,46,16],[10,142,114,2,143,115],[19,74,46,4,75,47],[28,50,22,6,51,23],[33,46,16,4,47,17],[8,152,122,4,153,123],[22,73,45,3,74,46],[8,53,23,26,54,24],[12,45,15,28,46,16],[3,147,117,10,148,118],[3,73,45,23,74,46],[4,54,24,31,55,25],[11,45,15,31,46,16],[7,146,116,7,147,117],[21,73,45,7,74,46],[1,53,23,37,54,24],[19,45,15,26,46,16],[5,145,115,10,146,116],[19,75,47,10,76,48],[15,54,24,25,55,25],[23,45,15,25,46,16],[13,145,115,3,146,116],[2,74,46,29,75,47],[42,54,24,1,55,25],[23,45,15,28,46,16],[17,145,115],[10,74,46,23,75,47],[10,54,24,35,55,25],[19,45,15,35,46,16],[17,145,115,1,146,116],[14,74,46,21,75,47],[29,54,24,19,55,25],[11,45,15,46,46,16],[13,145,115,6,146,116],[14,74,46,23,75,47],[44,54,24,7,55,25],[59,46,16,1,47,17],[12,151,121,7,152,122],[12,75,47,26,76,48],[39,54,24,14,55,25],[22,45,15,41,46,16],[6,151,121,14,152,122],[6,75,47,34,76,48],[46,54,24,10,55,25],[2,45,15,64,46,16],[17,152,122,4,153,123],[29,74,46,14,75,47],[49,54,24,10,55,25],[24,45,15,46,46,16],[4,152,122,18,153,123],[13,74,46,32,75,47],[48,54,24,14,55,25],[42,45,15,32,46,16],[20,147,117,4,148,118],[40,75,47,7,76,48],[43,54,24,22,55,25],[10,45,15,67,46,16],[19,148,118,6,149,119],[18,75,47,31,76,48],[34,54,24,34,55,25],[20,45,15,61,46,16]];QRRSBlock.getRSBlocks=function(typeNumber,errorCorrectLevel){var rsBlock=QRRSBlock.getRsBlockTable(typeNumber,errorCorrectLevel);if(rsBlock==undefined){throw new Error("bad rs block @ typeNumber:"+typeNumber+"/errorCorrectLevel:"+errorCorrectLevel);}
	var length=rsBlock.length/3;var list=[];for(var i=0;i<length;i++){var count=rsBlock[i*3+0];var totalCount=rsBlock[i*3+1];var dataCount=rsBlock[i*3+2];for(var j=0;j<count;j++){list.push(new QRRSBlock(totalCount,dataCount));}}
	return list;};QRRSBlock.getRsBlockTable=function(typeNumber,errorCorrectLevel){switch(errorCorrectLevel){case QRErrorCorrectLevel.L:return QRRSBlock.RS_BLOCK_TABLE[(typeNumber-1)*4+0];case QRErrorCorrectLevel.M:return QRRSBlock.RS_BLOCK_TABLE[(typeNumber-1)*4+1];case QRErrorCorrectLevel.Q:return QRRSBlock.RS_BLOCK_TABLE[(typeNumber-1)*4+2];case QRErrorCorrectLevel.H:return QRRSBlock.RS_BLOCK_TABLE[(typeNumber-1)*4+3];default:return undefined;}};function QRBitBuffer(){this.buffer=[];this.length=0;}
	QRBitBuffer.prototype={get:function(index){var bufIndex=Math.floor(index/8);return((this.buffer[bufIndex]>>>(7-index%8))&1)==1;},put:function(num,length){for(var i=0;i<length;i++){this.putBit(((num>>>(length-i-1))&1)==1);}},getLengthInBits:function(){return this.length;},putBit:function(bit){var bufIndex=Math.floor(this.length/8);if(this.buffer.length<=bufIndex){this.buffer.push(0);}
	if(bit){this.buffer[bufIndex]|=(0x80>>>(this.length%8));}
	this.length++;}};var QRCodeLimitLength=[[17,14,11,7],[32,26,20,14],[53,42,32,24],[78,62,46,34],[106,84,60,44],[134,106,74,58],[154,122,86,64],[192,152,108,84],[230,180,130,98],[271,213,151,119],[321,251,177,137],[367,287,203,155],[425,331,241,177],[458,362,258,194],[520,412,292,220],[586,450,322,250],[644,504,364,280],[718,560,394,310],[792,624,442,338],[858,666,482,382],[929,711,509,403],[1003,779,565,439],[1091,857,611,461],[1171,911,661,511],[1273,997,715,535],[1367,1059,751,593],[1465,1125,805,625],[1528,1190,868,658],[1628,1264,908,698],[1732,1370,982,742],[1840,1452,1030,790],[1952,1538,1112,842],[2068,1628,1168,898],[2188,1722,1228,958],[2303,1809,1283,983],[2431,1911,1351,1051],[2563,1989,1423,1093],[2699,2099,1499,1139],[2809,2213,1579,1219],[2953,2331,1663,1273]];
	
	function _isSupportCanvas() {
		return typeof CanvasRenderingContext2D != "undefined";
	}
	
	// android 2.x doesn't support Data-URI spec
	function _getAndroid() {
		var android = false;
		var sAgent = navigator.userAgent;
		
		if (/android/i.test(sAgent)) { // android
			android = true;
			var aMat = sAgent.toString().match(/android ([0-9]\.[0-9])/i);
			
			if (aMat && aMat[1]) {
				android = parseFloat(aMat[1]);
			}
		}
		
		return android;
	}
	
	var svgDrawer = (function() {

		var Drawing = function (el, htOption) {
			this._el = el;
			this._htOption = htOption;
		};

		Drawing.prototype.draw = function (oQRCode) {
			var _htOption = this._htOption;
			var _el = this._el;
			var nCount = oQRCode.getModuleCount();
			var nWidth = Math.floor(_htOption.width / nCount);
			var nHeight = Math.floor(_htOption.height / nCount);

			this.clear();

			function makeSVG(tag, attrs) {
				var el = document.createElementNS('http://www.w3.org/2000/svg', tag);
				for (var k in attrs)
					if (attrs.hasOwnProperty(k)) el.setAttribute(k, attrs[k]);
				return el;
			}

			var svg = makeSVG("svg" , {'viewBox': '0 0 ' + String(nCount) + " " + String(nCount), 'width': '100%', 'height': '100%', 'fill': _htOption.colorLight});
			svg.setAttributeNS("http://www.w3.org/2000/xmlns/", "xmlns:xlink", "http://www.w3.org/1999/xlink");
			_el.appendChild(svg);

			svg.appendChild(makeSVG("rect", {"fill": _htOption.colorLight, "width": "100%", "height": "100%"}));
			svg.appendChild(makeSVG("rect", {"fill": _htOption.colorDark, "width": "1", "height": "1", "id": "template"}));

			for (var row = 0; row < nCount; row++) {
				for (var col = 0; col < nCount; col++) {
					if (oQRCode.isDark(row, col)) {
						var child = makeSVG("use", {"x": String(col), "y": String(row)});
						child.setAttributeNS("http://www.w3.org/1999/xlink", "href", "#template")
						svg.appendChild(child);
					}
				}
			}
		};
		Drawing.prototype.clear = function () {
			while (this._el.hasChildNodes())
				this._el.removeChild(this._el.lastChild);
		};
		return Drawing;
	})();

	var useSVG = document.documentElement.tagName.toLowerCase() === "svg";

	// Drawing in DOM by using Table tag
	var Drawing = useSVG ? svgDrawer : !_isSupportCanvas() ? (function () {
		var Drawing = function (el, htOption) {
			this._el = el;
			this._htOption = htOption;
		};
			
		/**
		 * Draw the QRCode
		 * 
		 * @param {QRCode} oQRCode
		 */
		Drawing.prototype.draw = function (oQRCode) {
            var _htOption = this._htOption;
            var _el = this._el;
			var nCount = oQRCode.getModuleCount();
			var nWidth = Math.floor(_htOption.width / nCount);
			var nHeight = Math.floor(_htOption.height / nCount);
			var aHTML = ['<table style="border:0;border-collapse:collapse;">'];
			
			for (var row = 0; row < nCount; row++) {
				aHTML.push('<tr>');
				
				for (var col = 0; col < nCount; col++) {
					aHTML.push('<td style="border:0;border-collapse:collapse;padding:0;margin:0;width:' + nWidth + 'px;height:' + nHeight + 'px;background-color:' + (oQRCode.isDark(row, col) ? _htOption.colorDark : _htOption.colorLight) + ';"></td>');
				}
				
				aHTML.push('</tr>');
			}
			
			aHTML.push('</table>');
			_el.innerHTML = aHTML.join('');
			
			// Fix the margin values as real size.
			var elTable = _el.childNodes[0];
			var nLeftMarginTable = (_htOption.width - elTable.offsetWidth) / 2;
			var nTopMarginTable = (_htOption.height - elTable.offsetHeight) / 2;
			
			if (nLeftMarginTable > 0 && nTopMarginTable > 0) {
				elTable.style.margin = nTopMarginTable + "px " + nLeftMarginTable + "px";	
			}
		};
		
		/**
		 * Clear the QRCode
		 */
		Drawing.prototype.clear = function () {
			this._el.innerHTML = '';
		};
		
		return Drawing;
	})() : (function () { // Drawing in Canvas
		function _onMakeImage() {
			this._elImage.src = this._elCanvas.toDataURL("image/png");
			this._elImage.style.display = "block";
			this._elCanvas.style.display = "none";			
		}
		
		// Android 2.1 bug workaround
		// http://code.google.com/p/android/issues/detail?id=5141
		if (this._android && this._android <= 2.1) {
	    	var factor = 1 / window.devicePixelRatio;
	        var drawImage = CanvasRenderingContext2D.prototype.drawImage; 
	    	CanvasRenderingContext2D.prototype.drawImage = function (image, sx, sy, sw, sh, dx, dy, dw, dh) {
	    		if (("nodeName" in image) && /img/i.test(image.nodeName)) {
		        	for (var i = arguments.length - 1; i >= 1; i--) {
		            	arguments[i] = arguments[i] * factor;
		        	}
	    		} else if (typeof dw == "undefined") {
	    			arguments[1] *= factor;
	    			arguments[2] *= factor;
	    			arguments[3] *= factor;
	    			arguments[4] *= factor;
	    		}
	    		
	        	drawImage.apply(this, arguments); 
	    	};
		}
		
		/**
		 * Check whether the user's browser supports Data URI or not
		 * 
		 * @private
		 * @param {Function} fSuccess Occurs if it supports Data URI
		 * @param {Function} fFail Occurs if it doesn't support Data URI
		 */
		function _safeSetDataURI(fSuccess, fFail) {
            var self = this;
            self._fFail = fFail;
            self._fSuccess = fSuccess;

            // Check it just once
            if (self._bSupportDataURI === null) {
                var el = document.createElement("img");
                var fOnError = function() {
                    self._bSupportDataURI = false;

                    if (self._fFail) {
                        self._fFail.call(self);
                    }
                };
                var fOnSuccess = function() {
                    self._bSupportDataURI = true;

                    if (self._fSuccess) {
                        self._fSuccess.call(self);
                    }
                };

                el.onabort = fOnError;
                el.onerror = fOnError;
                el.onload = fOnSuccess;
                el.src = "data:image/gif;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg=="; // the Image contains 1px data.
                return;
            } else if (self._bSupportDataURI === true && self._fSuccess) {
                self._fSuccess.call(self);
            } else if (self._bSupportDataURI === false && self._fFail) {
                self._fFail.call(self);
            }
		};
		
		/**
		 * Drawing QRCode by using canvas
		 * 
		 * @constructor
		 * @param {HTMLElement} el
		 * @param {Object} htOption QRCode Options 
		 */
		var Drawing = function (el, htOption) {
    		this._bIsPainted = false;
    		this._android = _getAndroid();
		
			this._htOption = htOption;
			this._elCanvas = document.createElement("canvas");
			this._elCanvas.width = htOption.width;
			this._elCanvas.height = htOption.height;
			el.appendChild(this._elCanvas);
			this._el = el;
			this._oContext = this._elCanvas.getContext("2d");
			this._bIsPainted = false;
			this._elImage = document.createElement("img");
			this._elImage.alt = "Scan me!";
			this._elImage.style.display = "none";
			this._el.appendChild(this._elImage);
			this._bSupportDataURI = null;
		};
			
		/**
		 * Draw the QRCode
		 * 
		 * @param {QRCode} oQRCode 
		 */
		Drawing.prototype.draw = function (oQRCode) {
            var _elImage = this._elImage;
            var _oContext = this._oContext;
            var _htOption = this._htOption;
            
			var nCount = oQRCode.getModuleCount();
			var nWidth = _htOption.width / nCount;
			var nHeight = _htOption.height / nCount;
			var nRoundedWidth = Math.round(nWidth);
			var nRoundedHeight = Math.round(nHeight);

			_elImage.style.display = "none";
			this.clear();
			
			for (var row = 0; row < nCount; row++) {
				for (var col = 0; col < nCount; col++) {
					var bIsDark = oQRCode.isDark(row, col);
					var nLeft = col * nWidth;
					var nTop = row * nHeight;
					_oContext.strokeStyle = bIsDark ? _htOption.colorDark : _htOption.colorLight;
					_oContext.lineWidth = 1;
					_oContext.fillStyle = bIsDark ? _htOption.colorDark : _htOption.colorLight;					
					_oContext.fillRect(nLeft, nTop, nWidth, nHeight);
					
					// 안티 앨리어싱 방지 처리
					_oContext.strokeRect(
						Math.floor(nLeft) + 0.5,
						Math.floor(nTop) + 0.5,
						nRoundedWidth,
						nRoundedHeight
					);
					
					_oContext.strokeRect(
						Math.ceil(nLeft) - 0.5,
						Math.ceil(nTop) - 0.5,
						nRoundedWidth,
						nRoundedHeight
					);
				}
			}
			
			this._bIsPainted = true;
		};
			
		/**
		 * Make the image from Canvas if the browser supports Data URI.
		 */
		Drawing.prototype.makeImage = function () {
			if (this._bIsPainted) {
				_safeSetDataURI.call(this, _onMakeImage);
			}
		};
			
		/**
		 * Return whether the QRCode is painted or not
		 * 
		 * @return {Boolean}
		 */
		Drawing.prototype.isPainted = function () {
			return this._bIsPainted;
		};
		
		/**
		 * Clear the QRCode
		 */
		Drawing.prototype.clear = function () {
			this._oContext.clearRect(0, 0, this._elCanvas.width, this._elCanvas.height);
			this._bIsPainted = false;
		};
		
		/**
		 * @private
		 * @param {Number} nNumber
		 */
		Drawing.prototype.round = function (nNumber) {
			if (!nNumber) {
				return nNumber;
			}
			
			return Math.floor(nNumber * 1000) / 1000;
		};
		
		return Drawing;
	})();
	
	/**
	 * Get the type by string length
	 * 
	 * @private
	 * @param {String} sText
	 * @param {Number} nCorrectLevel
	 * @return {Number} type
	 */
	function _getTypeNumber(sText, nCorrectLevel) {			
		var nType = 1;
		var length = _getUTF8Length(sText);
		
		for (var i = 0, len = QRCodeLimitLength.length; i <= len; i++) {
			var nLimit = 0;
			
			switch (nCorrectLevel) {
				case QRErrorCorrectLevel.L :
					nLimit = QRCodeLimitLength[i][0];
					break;
				case QRErrorCorrectLevel.M :
					nLimit = QRCodeLimitLength[i][1];
					break;
				case QRErrorCorrectLevel.Q :
					nLimit = QRCodeLimitLength[i][2];
					break;
				case QRErrorCorrectLevel.H :
					nLimit = QRCodeLimitLength[i][3];
					break;
			}
			
			if (length <= nLimit) {
				break;
			} else {
				nType++;
			}
		}
		
		if (nType > QRCodeLimitLength.length) {
			throw new Error("Too long data");
		}
		
		return nType;
	}

	function _getUTF8Length(sText) {
		var replacedText = encodeURI(sText).toString().replace(/\%[0-9a-fA-F]{2}/g, 'a');
		return replacedText.length + (replacedText.length != sText ? 3 : 0);
	}
	
	/**
	 * @class QRCode
	 * @constructor
	 * @example 
	 * new QRCode(document.getElementById("test"), "http://jindo.dev.naver.com/collie");
	 *
	 * @example
	 * var oQRCode = new QRCode("test", {
	 *    text : "http://naver.com",
	 *    width : 128,
	 *    height : 128
	 * });
	 * 
	 * oQRCode.clear(); // Clear the QRCode.
	 * oQRCode.makeCode("http://map.naver.com"); // Re-create the QRCode.
	 *
	 * @param {HTMLElement|String} el target element or 'id' attribute of element.
	 * @param {Object|String} vOption
	 * @param {String} vOption.text QRCode link data
	 * @param {Number} [vOption.width=256]
	 * @param {Number} [vOption.height=256]
	 * @param {String} [vOption.colorDark="#000000"]
	 * @param {String} [vOption.colorLight="#ffffff"]
	 * @param {QRCode.CorrectLevel} [vOption.correctLevel=QRCode.CorrectLevel.H] [L|M|Q|H] 
	 */
	QRCode = function (el, vOption) {
		this._htOption = {
			width : 256, 
			height : 256,
			typeNumber : 4,
			colorDark : "#000000",
			colorLight : "#ffffff",
			correctLevel : QRErrorCorrectLevel.H
		};
		
		if (typeof vOption === 'string') {
			vOption	= {
				text : vOption
			};
		}
		
		// Overwrites options
		if (vOption) {
			for (var i in vOption) {
				this._htOption[i] = vOption[i];
			}
		}
		
		if (typeof el == "string") {
			el = document.getElementById(el);
		}

		if (this._htOption.useSVG) {
			Drawing = svgDrawer;
		}
		
		this._android = _getAndroid();
		this._el = el;
		this._oQRCode = null;
		this._oDrawing = new Drawing(this._el, this._htOption);
		
		if (this._htOption.text) {
			this.makeCode(this._htOption.text);	
		}
	};
	
	/**
	 * Make the QRCode
	 * 
	 * @param {String} sText link data
	 */
	QRCode.prototype.makeCode = function (sText) {
		this._oQRCode = new QRCodeModel(_getTypeNumber(sText, this._htOption.correctLevel), this._htOption.correctLevel);
		this._oQRCode.addData(sText);
		this._oQRCode.make();
		this._el.title = sText;
		this._oDrawing.draw(this._oQRCode);			
		this.makeImage();
	};
	
	/**
	 * Make the Image from Canvas element
	 * - It occurs automatically
	 * - Android below 3 doesn't support Data-URI spec.
	 * 
	 * @private
	 */
	QRCode.prototype.makeImage = function () {
		if (typeof this._oDrawing.makeImage == "function" && (!this._android || this._android >= 3)) {
			this._oDrawing.makeImage();
		}
	};
	
	/**
	 * Clear the QRCode
	 */
	QRCode.prototype.clear = function () {
		this._oDrawing.clear();
	};
	
	/**
	 * @name QRCode.CorrectLevel
	 */
	QRCode.CorrectLevel = QRErrorCorrectLevel;
})();

    </script>

    <script>
        // System variables
        let state = "STANDBY";
        let blessings = 0;
        let brightness = 150;
        let pattern = 0; // Default
        let volume = 15;
        let pirEnabled = true;
        let selectedLang = "en";
        let selectedTheme = "tue";

        // True only when this page is being served BY the physical ESP32
        // itself (as opposed to running standalone, e.g. on GitHub Pages, as
        // a hardware-free simulator). Gates every call that talks to the
        // real device so the standalone simulator never tries to reach a
        // nonexistent local device.
        const isPhysicalESP = (window.location.hostname !== 'localhost' && window.location.hostname !== '127.0.0.1' && window.location.hostname !== 'rags1816.github.io' && window.location.hostname !== '');
        const sendESPControl = (action, extraParams = '') => {
            fetch(`/api/control?action=${action}${extraParams}`).catch(() => {});
        };
        const espGet = (path) => {
            fetch(path).catch(() => {});
        };
        // /api/settings and /api/state use numeric codes for language/theme;
        // the dashboard UI uses string codes. Keep the two mappings in one
        // place. Sanskrit and Marathi share firmware code 1 (Marathi/Sanskrit
        // combined) - polling back from the device can't distinguish them,
        // so it defaults to "sa".
        const LANG_TO_CODE = { en: 0, sa: 1, mr: 1, ta: 2 };
        const CODE_TO_LANG = { 0: "en", 1: "sa", 2: "ta" };
        const THEME_ORDER = ["tue", "mon", "wed", "thu", "fri", "sat", "sun"];

        // How long AMBIENT stays awake before returning to STANDBY. Must be
        // generous enough for the blessings loop to actually roll through
        // multiple entries of the 48-blessing list.
        const AMBIENT_IDLE_MS = 120000;
        // Fallback-only cushion added on top of a track's known duration when
        // arming the MANTRA_ACTIVE/FEET_ACTIVE safety-net timer, so a slow
        // network/buffering start doesn't cause the timer to cut audio off
        // before the real onended event has a chance to fire first.
        const ADVANCE_SAFETY_MARGIN_MS = 8000;
        // Tracks whether the welcome banner has already played for the
        // current "wake" cycle - it should only show once per STANDBY->AMBIENT
        // wake, not on every AMBIENT/MANTRA_ACTIVE/FEET_ACTIVE transition in
        // between, otherwise the blessings rotation never gets a chance to run.
        let introPlayed = false;
        // When true, drawLeds() renders the live LED preview even while
        // STANDBY, so brightness/theme/pattern changes are visible immediately.
        let previewActive = false;
        let previewTimer = null;

        // Aarti Mode: if a devotee hasn't touched anything for this long while
        // idle in AMBIENT, it runs a short scripted ritual (bell -> LED build
        // -> chant -> closing bell), then settles back to AMBIENT.
        const AARTI_IDLE_MS = 60000;
        let aartiTimer = null;
        let aartiBuildActive = false;
        // Once the idle-triggered Aarti has played during a wake cycle, don't
        // re-arm it - the device should settle to sleep afterward (roughly
        // 60s idle + ~4min Aarti + 2min sleep countdown), not loop Aarti after
        // Aarti indefinitely while nobody is around. Reset on every fresh wake.
        let aartiDoneThisWake = false;
        const AARTI_TRACK_FILE = "GaneshAarti.mp3";
        const AARTI_FALLBACK_DURATION_MS = 240000; // GaneshAarti.mp3 is ~4 minutes

        // Temple Closed (Night Mode)
        const CLOSE_HOLD_MS = 15000;
        let mouseDownAt = 0;
        let feetDownAt = 0;
        let closeHoldTimer = null;
        let closeHoldTriggered = false;
        let closeHoldMessageShown = false;

        // Multilingual Database
        const languages = {
            en: {
                welcome: "Welcome!",
                mantra: "   Om Gan Ganapataye Namaha! May the Lord of Wisdom optimize your life's neural networks.   ",
                adultBlessing: "   Blessings: Wishing you deep intellect, peace, and spiritual growth.   ",
                childBlessing: "   Blessings: Happy coding! Study hard and keep smiling.   ",
                standby: "Ready for blessings"
            },
            sa: {
                welcome: "स्वागतम्!",
                mantra: "   ॐ गं गणपतये नमः। वक्रतुण्ड महाकाय सूर्यकोटि समप्रभ।   ",
                adultBlessing: "   आशीर्वाद: बुद्धिं यशो वीर्यं बलमस्तु सदा। सुख शान्ति समृद्धि च।   ",
                childBlessing: "   आशीर्वाद: विद्यां ददाति विनयम्। नित्यं प्रसन्नो भव।   ",
                standby: "आशीर्वादाय सिद्धः"
            },
            mr: {
                welcome: "सुस्वागतम!",
                mantra: "   ॐ गं गणपतये नमः। मंगलमूर्ती मोरया! गणपती बाप्पा मोरया!   ",
                adultBlessing: "   आशीर्वाद: तुमच्या आयुष्यातील सर्व अडथळे दूर होवोत. सुख-समृद्धी लाभो!   ",
                childBlessing: "   आशीर्वाद: खूप अभ्यास कर, मोठा हो आणि नेहमी हसत राहा!   ",
                standby: "बाप्पा मोरया"
            },
            ta: {
                welcome: "வரவேற்பு!",
                mantra: "   ஓம் கம் கணபதயே நமஹ! மங்கள மூர்த்தி மோரையா!   ",
                adultBlessing: "   ஆசீர்வாதம்: உங்கள் வாழ்வில் உள்ள அனைத்து தடைகளும் நீங்கி வெற்றி பெறட்டும்.   ",
                childBlessing: "   ஆசீர்வாதம்: நன்முறையில் கல்வி கற்று, வாழ்வில் சிறந்து விளங்குவாயாக!   ",
                standby: "ஆசீர்வாதம் தயார்"
            }
        };

        // Theme colors configurations
        const themes = {
            tue: { primary: "#800020", secondary: "#ffd700", name: "Tuesday Ganesha" }, // Maroon & Gold
            mon: { primary: "#00f2fe", secondary: "#4facfe", name: "Monday Shiva" }, // Cyan & Blue
            wed: { primary: "#00b4db", secondary: "#00ff87", name: "Wednesday Wisdom" }, // Teal & Green
            thu: { primary: "#ffd700", secondary: "#ffa500", name: "Thursday Guru" }, // Gold & Orange
            fri: { primary: "#ec008c", secondary: "#b92b27", name: "Friday Shakti" }, // Pink & Magenta
            sat: { primary: "#4b0082", secondary: "#4facfe", name: "Saturday Discipline" }, // Indigo & Blue
            sun: { primary: "#ff4b4b", secondary: "#ffd700", name: "Sunday Sun" } // Red & Gold
        };

        // OLED animation variables
        let scrollTimer = null;
        let scrollX = 180;
        let textWidth = 0;
        let currentText = "Welcome!";

        // LED Animation variables
        const numLeds = 24;
        const ledDots = [];
        let animationFrameId = null;
        let hueOffset = 0;
        
        const bellAudio = new Audio('Ganapathibell.mp3');
        
        let isRealBellPlaying = false;
        let isRealMantraPlaying = false;
        let isRealFeetMantraPlaying = false;
        let alternateMantra = false; // false = Ganeshmantra1, true = Ganeshmantra2

        // Sound Synthesis (Web Audio API)
        let audioCtx = null;

        // Generate simulated LED Dots
        const ledRing = document.getElementById('led-ring');
        for (let i = 0; i < numLeds; i++) {
            const dot = document.createElement('div');
            dot.className = 'led-dot';
            const angle = (i * 360 / numLeds) * (Math.PI / 180);
            const radius = 38; // px
            const x = Math.round(45 + radius * Math.cos(angle) - 3);
            const y = Math.round(45 + radius * Math.sin(angle) - 3);
            dot.style.left = `${x}px`;
            dot.style.top = `${y}px`;
            ledRing.appendChild(dot);
            ledDots.push(dot);
        }

        // Initialize Audio context on first click and resume if suspended
        function initAudio() {
            if (!audioCtx) {
                audioCtx = new (window.AudioContext || window.webkitAudioContext)();
            }
            if (audioCtx.state === 'suspended') {
                audioCtx.resume();
            }
        }

        // True Temple Bell Sound Synthesis with 5-Second Beating Oscillation
        function playBellTone(freq = 250, duration = 5.0) {
            // The physical device's real speaker (via its DFPlayer) already
            // plays the actual bell chime - its web server can't serve the
            // real Ganapathibell.mp3 (or any mp3) at all, so this would
            // always 404 and fall back to a synthesized approximation,
            // producing an unwanted second/wrong-sounding chime. Skip
            // entirely on a physical device; only used in the simulator.
            if (isPhysicalESP) return;

            initAudio();
            if (volume === 0) return;

            // Set volume based on dashboard slider
            bellAudio.volume = volume / 30;

            // Rewind before playing: calling play() on an element that's
            // already playing is a silent no-op, which made multi-ring
            // sequences (like the 3-bell wake ritual) sound as a single bell
            // when the real mp3 is present. Rewinding restarts the ring.
            try { bellAudio.currentTime = 0; } catch (e) {}
            
            isRealBellPlaying = true;
            bellAudio.play().then(() => {
                console.log("Playing custom Ganapathibell.mp3");
            }).catch((err) => {
                isRealBellPlaying = false;
                console.log("Ganapathibell.mp3 not found or blocked, falling back to synthesizer:", err);
                synthesizeBell(freq, duration);
            });
        }

        function synthesizeBell(freq, duration) {
            if (!audioCtx) return;
            const volRatio = volume / 30;
            const now = audioCtx.currentTime;
            const baseFreq = freq > 400 ? freq / 3.2 : freq;

            const osc1 = audioCtx.createOscillator();
            const gain1 = audioCtx.createGain();
            osc1.type = "sine";
            osc1.frequency.setValueAtTime(baseFreq, now);
            gain1.gain.setValueAtTime(0.25 * volRatio, now);
            gain1.gain.exponentialRampToValueAtTime(0.001, now + duration);

            const osc2 = audioCtx.createOscillator();
            const gain2 = audioCtx.createGain();
            osc2.type = "sine";
            osc2.frequency.setValueAtTime(baseFreq + 2.5, now);
            gain2.gain.setValueAtTime(0.25 * volRatio, now);
            gain2.gain.exponentialRampToValueAtTime(0.001, now + duration);

            const harmonics = [
                { ratio: 1.2, gain: 0.15, decay: 3.5 }, // Minor third
                { ratio: 1.5, gain: 0.12, decay: 3.0 }, // Perfect fifth
                { ratio: 2.0, gain: 0.10, decay: 2.5 }, // Octave
                { ratio: 2.6, gain: 0.08, decay: 2.0 }, // Upper minor third
                { ratio: 3.0, gain: 0.05, decay: 1.5 }  // Upper fifth
            ];

            harmonics.forEach(h => {
                const oscH = audioCtx.createOscillator();
                const gainH = audioCtx.createGain();
                oscH.type = "sine";
                oscH.frequency.setValueAtTime(baseFreq * h.ratio, now);
                gainH.gain.setValueAtTime(h.gain * volRatio, now);
                gainH.gain.exponentialRampToValueAtTime(0.001, now + h.decay);

                oscH.connect(gainH);
                gainH.connect(audioCtx.destination);
                oscH.start(now);
                oscH.stop(now + h.decay);
            });

            osc1.connect(gain1);
            osc2.connect(gain2);
            gain1.connect(audioCtx.destination);
            gain2.connect(audioCtx.destination);

            osc1.start(now);
            osc2.start(now);
            osc1.stop(now + duration);
            osc2.stop(now + duration);
        }

        // Shloka Synth Hum
        let soundSource = null;
        let soundSourceAmp = null;
        
        function playHum(freq = 220, duration = 15) {
            initAudio();
            if (volume === 0) return;
            stopHum();

            mantraAudio.volume = volume / 30;

            isRealMantraPlaying = true;
            mantraAudio.play().then(() => {
                console.log("Playing custom Ganapathimantrai.mp3");
            }).catch((err) => {
                isRealMantraPlaying = false;
                console.log("Ganapathimantrai.mp3 not found or blocked, falling back to synthesizer:", err);
                synthesizeHum(freq, duration);
            });
        }

        // Blessings database from Blessings1.docx
        const adultBlessings = [
            "I bless you with the removal of professional and personal roadblocks to ensure success.",
            "I grant you the grace of financial stability, wealth, and career growth.",
            "I bless you with the emotional capacity to handle high-stress situations with calm.",
            "I bless you with the wisdom to weigh choices objectively and make sound judgments.",
            "I bless you with a peaceful, loving, and supportive family environment.",
            "I grant you the ability to be thankful for life's blessings and maintain self-improvement.",
            "I bless you with the discipline to avoid greed, anger, or unhealthy attachments.",
            "I bless you to forgive easily and help those in need.",
            "I grant you the wisdom to release things that are beyond your control.",
            "I wish you a deeper connection to your inner self and finding peace amidst a busy life.",
            "I bless you with profound clarity of mind, patient wisdom, and inner strength.",
            "May all professional obstacles dissolve, opening wide doors to prosperity and success.",
            "I grant you emotional resilience and a peaceful heart, shielding you from anxiety.",
            "May My divine energy rejuvenate your physical body, infusing you with robust health.",
            "I bless your home with harmony, lasting unity, and a deep sense of security.",
            "When the weights of responsibility feel too heavy, surrender your burdens into My hands.",
            "May you always possess the integrity, humility, and patience to handle hard situations.",
            "I bless your hard work so that it bears rich fruit, ensuring you never lack resources.",
            "May My presence be a constant anchor in your life, grounding you in spiritual peace.",
            "I bless your journey with continuous growth, purposeful action, and vibrant well-being.",
            "May your heart beat with steady strength and your body remain flexible and resilient.",
            "I bless you with the wisdom to balance hard work with mindful rest, protecting your vitality."
        ];

        const childBlessings = [
            "I bless you to explore, learn, and absorb new knowledge eagerly.",
            "I bless you with the boon of concentration to stay grounded during studies.",
            "I grant you the courage to bounce back quickly when faced with difficult subjects.",
            "I bless you with the grace to remain polite and grounded, much like Ganesha's nature.",
            "I grant you the inspiration to think outside the box and express yourself freely.",
            "I grant you the wisdom to make healthy, positive lifestyle choices from a young age.",
            "I bless you with inner strength to overcome fears in making friends or public speaking.",
            "I grant you the ability to find simple, childlike happiness in everyday moments.",
            "I grant you the blessing of attracting honest, supportive, and kind friends.",
            "I bless you with the shield of grace to keep you safe and guide your journey.",
            "I bless you to be guided safely past financial, mental, and physical hurdles.",
            "I bless you with clarity, sharp focus, and success in studies or new ventures.",
            "I bless you with attracting material success, abundance, and good fortune.",
            "I grant your wish of creating a balanced, calm, and positive environment.",
            "I bless your young mind with sharp focus, memory, and joyful curiosity to learn.",
            "May your heart always be fearless and filled with kind thoughts for everyone.",
            "Whenever a school lesson feels too difficult, remember that I am right beside you.",
            "I grant you the wisdom of My large ears to listen carefully and grow wise.",
            "May My blessings protect you from harm and guide your steps safely.",
            "I fill your spirit with boundless energy to play and explore the world with joy.",
            "May you always speak words as sweet as the modaks I love, spreading happiness.",
            "I bless your body with robust health, deep immunity, and strong glowing energy.",
            "When you feel sad or alone, close your eyes and call My name for instant comfort.",
            "I bless your entire childhood with endless wonder, creative ideas, and a bright smile.",
            "I grant you deep, peaceful sleep at night so your body can rest and wake up full of energy.",
            "I bless every meal you eat to nourish your bones, sharpen your mind, and make you strong."
        ];
        const combinedBlessings = [
            ...adultBlessings,
            ...childBlessings
        ];
        let currentBlessingIndex = 0;

        const mantraTracks = [
            { file: "Ganapathimantrai.mp3", duration: 19121, dfTrack: 1 },
            { file: "Ganpathimantra1.mp3", duration: 28390, dfTrack: 2 },
            { file: "Ganapathimantra2.mp3", duration: 27360, dfTrack: 4 },
            { file: "Ganeshmantra3.mp3", duration: 55350, dfTrack: 5 },
            { file: "Ganeshmantra4.mp3", duration: 155530, dfTrack: 6 },
            { file: "Ganeshmantra5.mp3", duration: 99600, dfTrack: 7 },
            { file: "Ganeshmantra6.mp3", duration: 60160, dfTrack: 8 },
            { file: "Ganeshmantra7.mp3", duration: 136700, dfTrack: 9 },
            { file: "Ganeshmantra8.mp3", duration: 125520, dfTrack: 10 },
            { file: "Ganeshmantra9.mp3", duration: 48800, dfTrack: 11 },
            { file: "Ganeshmantra10.mp3", duration: 27380, dfTrack: 12 },
            { file: "Ganeshmantra11.mp3", duration: 79280, dfTrack: 13 },
            { file: "Ganeshmantra12.mp3", duration: 175730, dfTrack: 14 },
            { file: "Ganeshmantra13.mp3", duration: 28400, dfTrack: 15 }
        ];

        let mouseStep = 0;
        let feetStep = 0;
        const globalMantraPlayer = new Audio();

        // --- Now Playing / Song Library (item 3) ---
        let nowPlayingFile = null;
        let nowPlayingDurationMs = 0;
        let nowPlayingStartTs = 0;

        function formatTime(ms) {
            const totalSec = Math.max(0, Math.round(ms / 1000));
            const m = Math.floor(totalSec / 60);
            const s = totalSec % 60;
            return m + ":" + String(s).padStart(2, '0');
        }

        function startNowPlaying(file, durationMs) {
            nowPlayingFile = file;
            nowPlayingDurationMs = durationMs;
            nowPlayingStartTs = Date.now();
        }

        function stopNowPlaying() {
            nowPlayingFile = null;
        }

        function updateNowPlayingDisplay() {
            const trackEl = document.getElementById('now-playing-track');
            const barEl = document.getElementById('now-playing-bar');
            const elapsedEl = document.getElementById('now-playing-elapsed');
            const totalEl = document.getElementById('now-playing-total');
            if (!trackEl) return;

            const isActive = nowPlayingFile && (state === "MANTRA_ACTIVE" || state === "FEET_ACTIVE" || state === "AARTI_MODE");
            if (!isActive) {
                trackEl.innerText = "Nothing playing";
                barEl.style.width = "0%";
                elapsedEl.innerText = "0:00";
                totalEl.innerText = "0:00";
                highlightSongLibraryRow(null);
                return;
            }

            const elapsedMs = Date.now() - nowPlayingStartTs;
            const pct = nowPlayingDurationMs > 0 ? Math.min(100, (elapsedMs / nowPlayingDurationMs) * 100) : 0;
            trackEl.innerText = nowPlayingFile.replace(/\.mp3$/i, '');
            barEl.style.width = pct + "%";
            elapsedEl.innerText = formatTime(elapsedMs);
            totalEl.innerText = formatTime(nowPlayingDurationMs);
            highlightSongLibraryRow(nowPlayingFile);
        }
        setInterval(updateNowPlayingDisplay, 500);

        function highlightSongLibraryRow(file) {
            const rows = document.querySelectorAll('#song-library-list .song-row');
            rows.forEach(row => {
                row.style.color = (file && row.dataset.file === file) ? '#ffd700' : '';
                row.style.fontWeight = (file && row.dataset.file === file) ? 'bold' : 'normal';
            });
        }

        function renderSongLibrary() {
            const container = document.getElementById('song-library-list');
            if (!container) return;
            const allTracks = mantraTracks.concat([{ file: AARTI_TRACK_FILE, duration: AARTI_FALLBACK_DURATION_MS }]);
            container.innerHTML = allTracks.map(t => `
                <div class="song-row" data-file="${t.file}" style="display: flex; justify-content: space-between; padding: 4px 6px; font-size: 10px; border-bottom: 1px solid rgba(255,255,255,0.05);">
                    <span>${t.file.replace(/\.mp3$/i, '')}</span>
                    <span style="color: #8892b0;">${formatTime(t.duration)}</span>
                </div>
            `).join('');
        }

        // Audio-reactive breathing setup: both the real mp3 (globalMantraPlayer)
        // and the synthesized fallback hum route through this shared bus, so
        // whichever one is actually producing sound drives the same analyser -
        // LED "breathing" then follows real audio energy instead of a fixed
        // sine wave that merely happens to run at the same time.
        let mantraAnalyser = null;
        let mantraFreqData = null;
        let mantraBusGain = null;
        let mantraSourceNode = null;
        function ensureMantraAudioGraph() {
            if (!audioCtx) return;
            if (!mantraBusGain) {
                mantraBusGain = audioCtx.createGain();
                mantraAnalyser = audioCtx.createAnalyser();
                mantraAnalyser.fftSize = 256;
                mantraFreqData = new Uint8Array(mantraAnalyser.frequencyBinCount);
                mantraBusGain.connect(mantraAnalyser);
                mantraAnalyser.connect(audioCtx.destination);
            }
            // CRITICAL: when the page is opened directly from disk (file://),
            // Chrome treats every local file as a separate opaque origin, so
            // MediaElementAudioSource "outputs zeroes due to CORS access
            // restrictions" - connecting the mp3 element to the audio graph
            // silently MUTES every mantra while the element still reports
            // "playing". So on file:// we leave the element's audio going
            // straight to the speakers and simply skip the analyser hookup
            // (LED breathing falls back to its sine wave; the synthesized
            // fallback hum still routes through the analyser fine).
            // Over http/https (e.g. GitHub Pages) the hookup is same-origin
            // and safe, so full audio-reactivity works there.
            if (window.location.protocol === 'file:') return;
            if (!mantraSourceNode) {
                try {
                    mantraSourceNode = audioCtx.createMediaElementSource(globalMantraPlayer);
                    mantraSourceNode.connect(mantraBusGain);
                } catch (e) {
                    console.log("Mantra analyser hookup skipped:", e);
                }
            }
        }
        function getMantraEnergy() {
            if (!mantraAnalyser || !(isRealMantraPlaying || isRealFeetMantraPlaying || soundSource)) return null;
            mantraAnalyser.getByteFrequencyData(mantraFreqData);
            let sum = 0;
            for (let k = 0; k < mantraFreqData.length; k++) sum += mantraFreqData[k];
            const avg = sum / mantraFreqData.length / 255;
            if (avg <= 0.001) return null;
            return avg;
        }

        // --- One-Directional Mic: Wake-on-voice/clap + Crowd-reactive Aarti ---
        // Deliberately never connected to audioCtx.destination - this is for
        // listening only, never routed to the speaker (no feedback risk).
        let micStream = null;
        let micAnalyser = null;
        let micTimeData = null;
        let micMonitorInterval = null;
        let micRecentLevels = [];
        let lastMicWakeTime = 0;
        const MIC_WAKE_COOLDOWN_MS = 4000;

        async function toggleMic() {
            const enabled = document.getElementById('mic-toggle').checked;
            const statusEl = document.getElementById('mic-status');
            if (enabled) {
                try {
                    initAudio();
                    micStream = await navigator.mediaDevices.getUserMedia({ audio: true });
                    const micSource = audioCtx.createMediaStreamSource(micStream);
                    micAnalyser = audioCtx.createAnalyser();
                    micAnalyser.fftSize = 512;
                    micTimeData = new Uint8Array(micAnalyser.fftSize);
                    micSource.connect(micAnalyser);

                    statusEl.innerText = "Listening (wake + crowd-reactive)";
                    statusEl.style.color = "#4ade80";

                    micRecentLevels = [];
                    if (micMonitorInterval) clearInterval(micMonitorInterval);
                    micMonitorInterval = setInterval(monitorMic, 120);
                } catch (err) {
                    console.log("Mic access denied or unavailable:", err);
                    statusEl.innerText = "Mic access denied";
                    statusEl.style.color = "#f87171";
                    document.getElementById('mic-toggle').checked = false;
                }
            } else {
                if (micMonitorInterval) { clearInterval(micMonitorInterval); micMonitorInterval = null; }
                if (micStream) { micStream.getTracks().forEach(t => t.stop()); micStream = null; }
                micAnalyser = null;
                statusEl.innerText = "Off";
                statusEl.style.color = "var(--accent-teal)";
            }
        }

        // Root-mean-square level from the raw waveform, roughly 0..1. Used for
        // loudness/spike detection - not pitch, just "how loud right now".
        function getMicLevel() {
            if (!micAnalyser) return null;
            micAnalyser.getByteTimeDomainData(micTimeData);
            let sumSquares = 0;
            for (let i = 0; i < micTimeData.length; i++) {
                const v = (micTimeData[i] - 128) / 128;
                sumSquares += v * v;
            }
            return Math.sqrt(sumSquares / micTimeData.length);
        }

        // Wake-on-voice/clap: watches for a sudden spike above the recent
        // ambient level while asleep - a clap or a loud greeting wakes the
        // device just like PIR would.
        function monitorMic() {
            const level = getMicLevel();
            if (level === null) return;

            const avg = micRecentLevels.length > 0
                ? micRecentLevels.reduce((a, b) => a + b, 0) / micRecentLevels.length
                : level;

            if (state === "STANDBY") {
                const now = Date.now();
                const isSpike = level > 0.15 && level > avg * 2.5;
                if (isSpike && now - lastMicWakeTime > MIC_WAKE_COOLDOWN_MS) {
                    lastMicWakeTime = now;
                    changeState("AMBIENT", AMBIENT_IDLE_MS);
                }
            }

            micRecentLevels.push(level);
            if (micRecentLevels.length > 20) micRecentLevels.shift(); // ~2.4s window
        }

        function stopHum() {
            if (globalMantraPlayer) {
                try {
                    globalMantraPlayer.onended = null; // don't let an interrupted Aarti chant finish late
                    globalMantraPlayer.pause();
                    globalMantraPlayer.currentTime = 0;
                } catch(e){}
            }

            if (isRealBellPlaying) {
                bellAudio.pause();
                bellAudio.currentTime = 0;
                isRealBellPlaying = false;
            }
            
            isRealMantraPlaying = false;
            isRealFeetMantraPlaying = false;

            if (soundSource) {
                try {
                    soundSourceAmp.gain.setValueAtTime(0, audioCtx.currentTime);
                    soundSource.stop();
                } catch(e) {}
                soundSource = null;
            }
        }



        function synthesizeHum(freq, duration) {
            if (!audioCtx) return;
            ensureMantraAudioGraph();
            const volRatio = volume / 30;
            const now = audioCtx.currentTime;

            const osc = audioCtx.createOscillator();
            soundSourceAmp = audioCtx.createGain();
            osc.type = "triangle";
            osc.frequency.setValueAtTime(freq, now);
            
            const vibrato = audioCtx.createOscillator();
            const vibratoGain = audioCtx.createGain();
            vibrato.frequency.setValueAtTime(5.5, now);
            vibratoGain.gain.setValueAtTime(2.5, now);
            vibrato.connect(vibratoGain);
            vibratoGain.connect(osc.frequency);
            
            soundSourceAmp.gain.setValueAtTime(0.12 * volRatio, now);
            soundSourceAmp.gain.exponentialRampToValueAtTime(0.001, now + duration);

            osc.connect(soundSourceAmp);
            // Route through the shared analyser bus (falls back to direct
            // destination if unavailable) so the LED ring can breathe in time
            // with the actual sound.
            soundSourceAmp.connect(mantraBusGain || audioCtx.destination);

            vibrato.start();
            osc.start(now);
            soundSource = osc;
            
            setTimeout(() => {
                vibrato.stop();
                try { osc.stop(); } catch(e) {}
            }, duration * 1000);
        }

        // Update UI Text & Labels
        function updateUI() {
            document.getElementById('state-val').innerText = state;
            const stateLbl = document.getElementById('oled-state-lbl');
            const stateVal = document.getElementById('state-val');
            
            if (state === "STANDBY") {
                stateLbl.innerText = "[STANDBY]";
                stateVal.className = "status-val";
                document.getElementById('oled-scroller').style.display = 'none';
            } else if (state === "TEMPLE_CLOSED") {
                stateLbl.innerText = "[CLOSED \ud83c\udf19]";
                stateVal.className = "status-val";
                document.getElementById('oled-scroller').style.display = 'none';
            } else {
                stateLbl.innerText = `[${state}]`;
                stateVal.className = "status-val status-active";
                document.getElementById('oled-scroller').style.display = 'block';
            }

            document.getElementById('count-val').innerText = blessings;
            document.getElementById('oled-hits-lbl').innerText = `Devotional Hits: ${blessings}`;
            document.getElementById('bright-lbl').innerText = Math.round((brightness/255)*100) + "%";
            document.getElementById('vol-lbl').innerText = volume;
        }

        // Text Scroll Simulator
        function setOledText(text) {
            currentText = text;
            const scroller = document.getElementById('oled-scroller');
            scroller.innerText = text;
            
            if (scrollTimer) clearInterval(scrollTimer);

            if (state === "FEET_ACTIVE") {
                // Blessing mode: static, wrapped, centered, and GOLD!
                scroller.style.whiteSpace = 'normal';
                scroller.style.textAlign = 'center';
                scroller.style.color = '#ffd700'; // Gold
                scroller.style.textShadow = '0 0 6px rgba(255, 215, 0, 0.4)';
                scroller.style.transform = 'none';
                scroller.style.position = 'relative';
                scroller.style.width = '100%';
                scroller.style.fontSize = '8px'; // Slightly smaller to fit personalized content
                scroller.style.lineHeight = '1.1';
            } else {
                // Normal rolling mode: cyan, scrolling, single line
                scroller.style.whiteSpace = 'nowrap';
                scroller.style.color = '#00f2fe'; // Cyan
                scroller.style.textShadow = 'none';
                scroller.style.position = 'absolute';
                scroller.style.width = 'auto';
                scroller.style.fontSize = '10px';
                scroller.style.transform = `translateX(180px)`;
                scrollX = 180;
                textWidth = scroller.scrollWidth; // measured, not estimated

                const isWelcome = (text === languages.en.welcome || text === languages.sa.welcome || text === languages.mr.welcome || text === languages.ta.welcome);
                if (state !== "STANDBY" && state !== "TEMPLE_CLOSED" && !isWelcome && text !== "") {
                    scrollTimer = setInterval(() => {
                        scrollX -= 2;
                        if (scrollX < -textWidth) {
                            scrollX = 180;
                            // When a message finishes scrolling, load the next blessing from
                            // the 48-blessings loop - but only in the standalone simulator.
                            // On a physical device, pollDeviceState() is the one that updates
                            // scroller text (from the firmware's own scrollText), so picking
                            // independently here would show different text than the display.
                            if (!isPhysicalESP && (state === "AMBIENT" || state === "MANTRA_ACTIVE")) {
                                const nextBlessing = combinedBlessings[currentBlessingIndex];
                                currentBlessingIndex = (currentBlessingIndex + 1) % combinedBlessings.length;
                                const nextText = `   [BLESSING] ${nextBlessing}   `;
                                scroller.innerText = nextText;
                                textWidth = scroller.scrollWidth;
                                onNewBlessingShown(nextBlessing);
                            }
                        }
                        scroller.style.transform = `translateX(${scrollX}px)`;
                    }, 80);
                } else {
                    scroller.style.transform = `translateX(35px)`; // center
                }
            }
        }

        // LED Animations
        // Blends color `b` into color `a` by amount `t` (0 = all a, 1 = all b).
        function blendColor(a, b, t) {
            return {
                r: Math.round(a.r * (1 - t) + b.r * t),
                g: Math.round(a.g * (1 - t) + b.g * t),
                b: Math.round(a.b * (1 - t) + b.b * t)
            };
        }

        function peacockWaveColor(i, hueOffset, c1, c2) {
            const wave = (Math.sin((hueOffset * 0.04) + (i * (Math.PI * 2 / numLeds))) + 1) / 2;
            return {
                r: Math.round(c1.r * (1 - wave) + c2.r * wave),
                g: Math.round(c1.g * (1 - wave) + c2.g * wave),
                b: Math.round(c1.b * (1 - wave) + c2.b * wave)
            };
        }

        function circuitPulseColor(i, hueOffset, c1, c2, breathOverride) {
            // breathOverride, when provided (0..1), comes from live mantra audio
            // energy instead of a fixed sine wave - the ring visibly "breathes"
            // with the chant rather than on its own separate clock.
            const breath = (breathOverride != null) ? breathOverride : (Math.sin(hueOffset * 0.05) * 0.5 + 0.5);
            let color = {
                r: Math.round(c1.r * breath),
                g: Math.round(c1.g * breath),
                b: Math.round(c1.b * breath)
            };
            if (Math.random() < 0.05 && i === Math.floor(Math.random() * numLeds)) {
                color = { r: c2.r, g: c2.g, b: c2.b };
            }
            return color;
        }

        function goldenAuraColor(hueOffset, c1, c2) {
            const breath = Math.sin(hueOffset * 0.03) * 0.4 + 0.6;
            return {
                r: Math.round((c1.r * 0.6 + c2.r * 0.4) * breath),
                g: Math.round((c1.g * 0.6 + c2.g * 0.4) * breath),
                b: Math.round((c1.b * 0.6 + c2.b * 0.4) * breath)
            };
        }

        function rainbowDreamColor(i, hueOffset) {
            const indexHue = (hueOffset + (i * 360 / numLeds)) % 360;
            return hslToRgb(indexHue / 360, 1.0, 0.5);
        }

        function diyaFlickerColor(i, hueOffset) {
            // Warm oil-lamp flicker - intentionally ignores the theme palette.
            const base = 0.55 + Math.sin(hueOffset * 0.07 + i * 1.3) * 0.15;
            const flicker = Math.random() * 0.25;
            const intensity = Math.max(0.15, Math.min(1, base + flicker));
            return {
                r: Math.round(255 * intensity),
                g: Math.round(140 * intensity * 0.75),
                b: Math.round(20 * intensity * 0.3)
            };
        }

        // Blessing mood tagging - lightweight keyword classification so all 48
        // blessings get a fitting color nudge without hand-tagging each one.
        const BLESSING_MOODS = {
            prosperity: { r: 255, g: 200, b: 40 },
            wisdom:     { r: 80, g: 170, b: 255 },
            peace:      { r: 140, g: 220, b: 170 },
            strength:   { r: 255, g: 90, b: 60 }
        };
        const BLESSING_MOOD_KEYWORDS = {
            prosperity: ['wealth', 'financial', 'success', 'prosper', 'career', 'abundan', 'growth', 'opportunit', 'resources'],
            wisdom:     ['wisdom', 'wise', 'clarity', 'mind', 'judgment', 'judgeme', 'knowledge', 'intellect', 'learn'],
            peace:      ['peace', 'calm', 'patien', 'forgive', 'harmony', 'loving', 'family', 'gentle', 'kind'],
            strength:   ['strength', 'discipline', 'courage', 'resilien', 'protect', 'obstacle', 'roadblock', 'hurdle', 'stress']
        };
        function classifyBlessingMood(text) {
            const lower = text.toLowerCase();
            for (const mood in BLESSING_MOOD_KEYWORDS) {
                if (BLESSING_MOOD_KEYWORDS[mood].some(kw => lower.includes(kw))) {
                    return BLESSING_MOODS[mood];
                }
            }
            return null;
        }
        let currentMoodColor = null;

        // --- New-blessing moment: sparkle sweep + soft chime ---------------
        let sparkleStartTime = 0;
        let sparkleEndTime = 0;
        const SPARKLE_DURATION_MS = 700;
        function onNewBlessingShown(blessingText) {
            currentMoodColor = classifyBlessingMood(blessingText);
            sparkleStartTime = Date.now();
            sparkleEndTime = sparkleStartTime + SPARKLE_DURATION_MS;
            // No chime here: visual sparkle only. The chime was audible right
            // before bell rituals and during offerings, which wasn't wanted.
        }
        // Short, isolated bell-like ping - deliberately does NOT touch the
        // shared soundSource/soundSourceAmp globals, so it never collides with
        // an active mantra hum.
        function playChime() {
            return; // DISABLED per request: no chimes anywhere - bells only.
            initAudio();
            if (!audioCtx || volume <= 0) return;
            const volRatio = volume / 30;
            const t = audioCtx.currentTime;
            [880, 1320].forEach((freq, idx) => {
                const osc = audioCtx.createOscillator();
                const gain = audioCtx.createGain();
                osc.type = "sine";
                osc.frequency.setValueAtTime(freq, t);
                gain.gain.setValueAtTime(0, t);
                gain.gain.linearRampToValueAtTime((idx === 0 ? 0.12 : 0.05) * volRatio, t + 0.02);
                gain.gain.exponentialRampToValueAtTime(0.001, t + 0.9);
                osc.connect(gain);
                gain.connect(audioCtx.destination);
                osc.start(t);
                osc.stop(t + 0.95);
            });
        }

        // --- Feet-touch "entering the temple" moment: quick flash -----------
        let feetFlashEndTime = 0;
        const FEET_FLASH_DURATION_MS = 400;
        function triggerFeetFlash() {
            feetFlashEndTime = Date.now() + FEET_FLASH_DURATION_MS;
        }

        // Briefly lets the LED ring render live (even in STANDBY) so changing
        // brightness/pattern/theme gives instant feedback without needing to
        // first wake the device.
        function previewLeds(ms = 2500) {
            previewActive = true;
            if (previewTimer) clearTimeout(previewTimer);
            previewTimer = setTimeout(() => { previewActive = false; }, ms);
        }

        // LED Animations
        function drawLeds() {
            if ((state === "STANDBY" || state === "TEMPLE_CLOSED") && !previewActive) {
                ledDots.forEach(dot => {
                    dot.style.backgroundColor = '#111';
                    dot.style.boxShadow = 'none';
                });
                hueOffset = 0;
            } else if (state === "RECORDING") {
                // Flash yellow to indicate active voice capturing
                ledDots.forEach(dot => {
                    dot.style.backgroundColor = 'rgba(255, 215, 0, 0.8)';
                    dot.style.boxShadow = '0 0 10px rgba(255, 215, 0, 0.8)';
                });
            } else {
                hueOffset += 1.5;
                let brightRatio = brightness / 255;

                // Crowd-reactive Aarti: while the chant is playing, a louder
                // room (singing, clapping) brightens the ring - a call and
                // response with the actual crowd, not just the recorded track.
                if (state === "AARTI_MODE" && !aartiBuildActive && micAnalyser) {
                    const crowdLevel = getMicLevel();
                    if (crowdLevel !== null) {
                        const crowdBoost = Math.min(0.4, crowdLevel * 1.5);
                        brightRatio = Math.min(1, brightRatio + crowdBoost);
                    }
                }
                
                let activePattern = pattern;
                let childRainbow = false;
                const activeThemeColor = themes[selectedTheme];
                let c1 = hexToRgb(activeThemeColor.primary);
                let c2 = hexToRgb(activeThemeColor.secondary);

                // Blessing-mood tint: nudge the palette toward the current
                // blessing's mood wherever blessings are actually displayed.
                // Left out of MANTRA_ACTIVE on purpose - that state stays a
                // "pure" flowing chant color.
                if (currentMoodColor && (state === "AMBIENT" || state === "FEET_ACTIVE")) {
                    c1 = blendColor(c1, currentMoodColor, 0.25);
                    c2 = blendColor(c2, currentMoodColor, 0.25);
                }

                // Audio-reactive breathing while a mantra is actually sounding.
                let breathOverride = null;
                if (state === "MANTRA_ACTIVE" || state === "FEET_ACTIVE" || (state === "AARTI_MODE" && !aartiBuildActive)) {
                    const energy = getMantraEnergy();
                    if (energy !== null) breathOverride = 0.25 + energy * 0.85;
                }

                if (state === "FEET_ACTIVE") {
                    const pitch = document.getElementById('voice-pitch-select').value;
                    if (pitch === "child") {
                        childRainbow = true; // Child trigger: Rainbow!
                    } else {
                        activePattern = 0; // Adult trigger: Peacock wave!
                    }
                } else if (state === "MANTRA_ACTIVE") {
                    activePattern = 1; // Circuit Pulse
                } else if (state === "AARTI_MODE") {
                    activePattern = aartiBuildActive ? 2 : 1; // Golden build -> Circuit Pulse chant
                }
                // AMBIENT uses whichever "Default LED Pattern" the user picked,
                // rendered in today's theme colors - that's the actual link
                // between the two controls: pattern decides motion, theme
                // decides the palette it's drawn in.

                const sparkling = Date.now() < sparkleEndTime;
                const sparkleProgress = sparkling ? (Date.now() - sparkleStartTime) / SPARKLE_DURATION_MS : 0;
                const sparklePos = sparkleProgress * numLeds;

                const flashing = Date.now() < feetFlashEndTime;
                const flashAmount = flashing ? (feetFlashEndTime - Date.now()) / FEET_FLASH_DURATION_MS : 0;

                ledDots.forEach((dot, i) => {
                    let color = {r: 0, g: 0, b: 0};
                    
                    if (childRainbow) {
                        color = rainbowDreamColor(i, hueOffset);
                    } 
                    else if (activePattern == 0) { // Peacock Wave
                        color = peacockWaveColor(i, hueOffset, c1, c2);
                    } 
                    else if (activePattern == 1) { // Circuit Pulse
                        color = circuitPulseColor(i, hueOffset, c1, c2, breathOverride);
                    } 
                    else if (activePattern == 2) { // Golden Aura
                        color = goldenAuraColor(hueOffset, c1, c2);
                    } 
                    else if (activePattern == 3) { // Rainbow Dream
                        color = rainbowDreamColor(i, hueOffset);
                    }
                    else if (activePattern == 4) { // Diya Flicker
                        color = diyaFlickerColor(i, hueOffset);
                    }

                    // "New blessing" sparkle sweep
                    if (sparkling) {
                        const dist = Math.min(Math.abs(i - sparklePos), numLeds - Math.abs(i - sparklePos));
                        if (dist < 2.2) {
                            const sparkleAmt = (1 - dist / 2.2) * (1 - sparkleProgress);
                            color = blendColor(color, { r: 255, g: 255, b: 255 }, sparkleAmt * 0.5);
                            color = blendColor(color, c2, sparkleAmt * 0.3);
                        }
                    }

                    // Feet-touch "entering the temple" flash
                    if (flashing) {
                        color = blendColor(color, { r: 255, g: 245, b: 220 }, flashAmount * 0.7);
                    }

                    // Apply brightness and display
                    const r = Math.round(color.r * brightRatio);
                    const g = Math.round(color.g * brightRatio);
                    const b = Math.round(color.b * brightRatio);
                    const rgbStr = `rgb(${r},${g},${b})`;
                    dot.style.backgroundColor = rgbStr;
                    dot.style.boxShadow = brightRatio > 0.1 ? `0 0 10px ${rgbStr}` : 'none';
                });
            }
            animationFrameId = requestAnimationFrame(drawLeds);
        }

        // HSL Helper
        function hslToRgb(h, s, l) {
            let r, g, b;
            if (s == 0) {
                r = g = b = l;
            } else {
                const hue2rgb = (p, q, t) => {
                    if (t < 0) t += 1;
                    if (t > 1) t -= 1;
                    if (t < 1/6) return p + (q - p) * 6 * t;
                    if (t < 1/2) return q;
                    if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
                    return p;
                };
                const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
                const p = 2 * l - q;
                r = hue2rgb(p, q, h + 1/3);
                g = hue2rgb(p, q, h);
                b = hue2rgb(p, q, h - 1/3);
            }
            return { r: Math.round(r * 255), g: Math.round(g * 255), b: Math.round(b * 255) };
        }

        // Hex to RGB
        function hexToRgb(hex) {
            var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
            return result ? {
                r: parseInt(result[1], 16),
                g: parseInt(result[2], 16),
                b: parseInt(result[3], 16)
            } : { r: 0, g: 242, b: 254 };
        }

        // Helpers for Base64URL encoding/decoding (bypasses IIS special character path restrictions)
        function base64UrlEncode(str) {
            return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g, (match, p1) => String.fromCharCode(parseInt(p1, 16)))).replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
        }
        function base64UrlDecode(str) {
            let b64 = str.replace(/-/g, '+').replace(/_/g, '/');
            while (b64.length % 4) b64 += '=';
            return decodeURIComponent(atob(b64).split('').map(c => '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2)).join(''));
        }

        // Personalized Blessing Formatter
        function formatPersonalizedBlessing(name, offeringName, wishText) {
            const offeringNames = {
                hibiscus: "Hibiscus Flower",
                garland: "Marigold Garland",
                modak: "Sweet Modak",
                coconut: "Fresh Coconut"
            };
            const offeringLabel = offeringNames[offeringName] || offeringName;
            const devoteeName = name && name !== "Anonymous Devotee" ? name.trim() : "";

            let blessingPart = "";
            if (wishText && wishText.trim() !== "") {
                blessingPart = `I bless you with ${wishText.trim()}.`;
            } else {
                // Select a random standard blessing
                const pitch = document.getElementById('voice-pitch-select') ? document.getElementById('voice-pitch-select').value : 'adult';
                const list = pitch === "child" ? childBlessings : adultBlessings;
                const rawBlessing = list[Math.floor(Math.random() * list.length)];
                
                blessingPart = rawBlessing.trim();
                if (!blessingPart.endsWith('.') && !blessingPart.endsWith('!')) {
                    blessingPart += ".";
                }
            }
            
            if (devoteeName !== "") {
                return `✨ ${devoteeName}: ${blessingPart} Thanks for the ${offeringLabel}. ✨`;
            } else {
                return `✨ Ganesha blesses you: ${blessingPart} Thanks for the ${offeringLabel}. ✨`;
            }
        }

        // State Machine Triggers
        let autoReturnTimer = null;
        let feetDisplayTimer = null;

        const WELCOME_BANNER = "   [WELCOME] sukh-samriddhi labho! Wishing you deep intellect, peace, and spiritual growth. khoop abhyas kar, motha ho ani nehami hasat raha! Happy coding! vidyam dadati vinayam. ungal vazhvil anaithu thadaigalum neengi vetri perattum.   ";

        // Shows the welcome banner once per wake cycle, then on every later
        // call resumes the 48-blessing loop from wherever it left off. Used by
        // AMBIENT/MANTRA_ACTIVE entry and to resume rolling after a FEET_ACTIVE
        // lock ends.
        function startAmbientRoll() {
            if (!introPlayed) {
                introPlayed = true;
                setOledText(WELCOME_BANNER);
            } else if (isPhysicalESP) {
                // On a physical device, pollDeviceState() drives what's shown
                // (the firmware's own scrollText, via /api/state's "blessing"
                // field) - don't also pick independently here, or the two
                // would show different text at the same moment.
            } else {
                const nextBlessing = combinedBlessings[currentBlessingIndex];
                currentBlessingIndex = (currentBlessingIndex + 1) % combinedBlessings.length;
                setOledText(`   [BLESSING] ${nextBlessing}   `);
                onNewBlessingShown(nextBlessing);
            }
        }

        function changeState(newState, duration = 0, personalizedText = null) {
            state = newState;
            updateUI();
            
            if (autoReturnTimer) clearTimeout(autoReturnTimer);
            if (feetDisplayTimer) clearTimeout(feetDisplayTimer);
            if (aartiTimer) clearTimeout(aartiTimer);
 
            if (newState === "STANDBY") {
                setOledText("");
                stopHum();
                stopNowPlaying();
                // Reset so the welcome banner plays again next time the device wakes.
                introPlayed = false;
                aartiDoneThisWake = false;
            } else if (newState === "TEMPLE_CLOSED") {
                // Night mode: fully silent and dark. Nothing wakes this except
                // an explicit touch on Mouse Back or Feet Touch - not PIR, not
                // the Aarti idle timer.
                setOledText("");
                stopHum();
                stopNowPlaying();
                introPlayed = false;
                aartiDoneThisWake = false;
            } else if (newState === "AMBIENT") {
                // In Ambient, roll the entire combined blessings loop
                startAmbientRoll();
                stopHum();
                stopNowPlaying();
                if (duration > 0) {
                    autoReturnTimer = setTimeout(() => changeState("STANDBY"), duration);
                }
                // If nobody touches anything for a while, offer a short Aarti -
                // but only once per wake cycle, so the device actually settles
                // to sleep afterward instead of looping Aarti indefinitely.
                // On a physical device this stays firmware-owned (its own
                // AMBIENT_TIMEOUT drives the same idle-close autonomously,
                // so the temple still closes even with no browser open) -
                // only arm this local timer for the standalone simulator.
                if (aartiTimer) clearTimeout(aartiTimer);
                if (!aartiDoneThisWake && !isPhysicalESP) {
                    aartiTimer = setTimeout(() => {
                        if (state === "AMBIENT") triggerAartiMode();
                    }, AARTI_IDLE_MS);
                }
            } else if (newState === "AARTI_MODE") {
                blessings++;
                aartiBuildActive = true;
                aartiDoneThisWake = true;
                setOledText("   \u2728 A moment of Aarti \u2728   ");
                initAudio();
                ensureMantraAudioGraph();
                // No bell here - the 3-bell prelude (ringWakeBells) already
                // rang immediately before this state was entered.

                const BUILD_MS = 2500; // brief pause for LEDs to build before the chant starts

                autoReturnTimer = setTimeout(() => {
                    if (state !== "AARTI_MODE") return;
                    aartiBuildActive = false;

                    const finishAarti = () => {
                        if (state !== "AARTI_MODE") return;
                        isRealMantraPlaying = false;
                        playBellTone(250, 2.0); // same temple bell, signalling the ritual has closed
                        if (isPhysicalESP) sendESPControl('stop');
                        const onComplete = aartiOnComplete;
                        aartiOnComplete = null;
                        if (onComplete) {
                            onComplete();
                        } else {
                            changeState("AMBIENT", AMBIENT_IDLE_MS);
                        }
                    };

                    startNowPlaying(AARTI_TRACK_FILE, AARTI_FALLBACK_DURATION_MS);
                    if (isPhysicalESP) {
                        // aartiOnComplete is set by closeTemple()/triggerAartiMode()
                        // (both intend to close afterward) and cleared by
                        // openTemple() (wakes into Ambient instead) - mirror
                        // that same intent to the firmware's own state machine.
                        sendESPControl(aartiOnComplete ? 'close' : 'aarti');
                        autoReturnTimer = setTimeout(finishAarti, 239000); // 3.59 mins (239s)
                    } else {
                        globalMantraPlayer.src = AARTI_TRACK_FILE;
                        globalMantraPlayer.volume = volume / 30;
                        isRealMantraPlaying = true;
                        // We don't need to know GaneshAarti.mp3's length up front -
                        // just wait for the browser to tell us it actually finished.
                        globalMantraPlayer.onended = () => {
                            globalMantraPlayer.onended = null;
                            finishAarti();
                        };
                        globalMantraPlayer.play().catch((err) => {
                            globalMantraPlayer.onended = null;
                            isRealMantraPlaying = false;
                            console.log(AARTI_TRACK_FILE + " failed to play, using synth:", err);
                            synthesizeHum(220, AARTI_FALLBACK_DURATION_MS / 1000);
                            autoReturnTimer = setTimeout(finishAarti, AARTI_FALLBACK_DURATION_MS);
                        });
                    }
                }, BUILD_MS);
            } else if (newState === "MANTRA_ACTIVE") {
                blessings++;
                // Keep blessings rolling during Mouse Back playback!
                startAmbientRoll();

                // Increment immediately at start of play (matches ESP32)
                mouseStep = (mouseStep + 1) % mantraTracks.length;

                // This local duration-based timer is a fallback for the
                // standalone simulator only. On a physical device, polling
                // is the sole authority on real state (pollDeviceState()'s
                // wasActive/nowIdle check already stops local audio/timers
                // once the device moves on) - this timer doesn't know the
                // real firmware's actual timing, so letting it fire
                // independently caused it to race the poll and briefly
                // show stale/wrong blessing text.
                if (!isPhysicalESP) {
                    autoReturnTimer = setTimeout(() => {
                        changeState("AMBIENT", AMBIENT_IDLE_MS);
                    }, duration + ADVANCE_SAFETY_MARGIN_MS);
                }
            } else if (newState === "FEET_ACTIVE") {
                blessings++;
                feetStep = (feetStep + 1) % mantraTracks.length;

                // On a physical device, the firmware picks and locks its own
                // personalized blessing - polling (pollDeviceState()'s
                // "blessing" sync) is what shows it here, same as Ambient/
                // Mantra. Picking one locally too would show different text
                // than the real device for as long as it takes the next
                // poll to correct it.
                if (personalizedText) {
                    // An offering was approved / admin-simulated - show the
                    // personalized thank-you built from that specific submission.
                    setOledText(personalizedText);
                } else if (!isPhysicalESP) {
                    // A real physical touch - show the next blessing in the
                    // 48-message rotation, same mood-tint/sparkle/chime as Ambient.
                    const nextBlessing = combinedBlessings[currentBlessingIndex];
                    currentBlessingIndex = (currentBlessingIndex + 1) % combinedBlessings.length;
                    setOledText(`   [BLESSING] ${nextBlessing}   `);
                    onNewBlessingShown(nextBlessing);
                }

                // Lock screen on this blessing for 12 seconds, then resume rolling loop
                feetDisplayTimer = setTimeout(() => {
                    if (state === "FEET_ACTIVE") {
                        startAmbientRoll();
                    }
                }, 12000);

                // Same reasoning as MANTRA_ACTIVE above - fallback for the
                // standalone simulator only, polling is the sole authority
                // on a physical device.
                if (duration > 0 && !isPhysicalESP) {
                    autoReturnTimer = setTimeout(advanceFromFeetActive, duration + ADVANCE_SAFETY_MARGIN_MS);
                }
            }
        }

        // Shared by both the FEET_ACTIVE safety-net timer and the audio
        // element's real onended event - whichever fires first wins, and
        // calling changeState() from either path cancels the other's timer.
        function advanceFromFeetActive() {
            if (state !== "FEET_ACTIVE") return;
            const prevState = window.ganeshaPrevState || "AMBIENT";
            window.ganeshaPrevState = null;

            // If a mantra was interrupted mid-play by the offering,
            // resume the SAME track from the SAME position - not a
            // fresh track from the beginning.
            if (resumePausedTrack()) return;

            if (prevState === "STANDBY") {
                changeState("STANDBY");
            } else {
                // AMBIENT, or anything with nothing to resume -
                // settle into a normal Ambient idle.
                changeState("AMBIENT", AMBIENT_IDLE_MS);
            }
        }
 
        // Dropdown Events
        function updateLanguage() {
            selectedLang = document.getElementById('lang-select').value;
            if (isPhysicalESP) espGet(`/api/settings?lang=${LANG_TO_CODE[selectedLang]}`);
            if (state === "AMBIENT") {
                setOledText(languages[selectedLang].welcome);
            }
        }

        function updateTheme() {
            selectedTheme = document.getElementById('theme-select').value;
            if (isPhysicalESP) espGet(`/api/settings?theme=${THEME_ORDER.indexOf(selectedTheme)}`);
            if (state === "AMBIENT" || state === "MANTRA_ACTIVE" || state === "FEET_ACTIVE") {
                updateUI();
            }
            previewLeds();
        }
 
        // Triggers
        function triggerMantra() {
            if (isPhysicalESP) sendESPControl('mantra');

            initAudio();
            stopHum(); // Stop any currently playing audio / synth hum
            ensureMantraAudioGraph();

            const track = mantraTracks[mouseStep];
            isRealMantraPlaying = true;
            startNowPlaying(track.file, track.duration);

            // The real speaker (via the ESP32's DFPlayer) already plays the
            // correct audio on a physical device - its web server only
            // serves the HTML page and API routes, never the actual mp3
            // files, so a local playback attempt here would just 404 and
            // fall back to a synthesized approximation, creating an
            // unwanted second sound. Only mirror audio locally in the
            // standalone simulator, where the real files are reachable.
            if (!isPhysicalESP) {
                globalMantraPlayer.src = track.file;
                globalMantraPlayer.volume = volume / 30;
                // Advance the instant the track actually finishes, rather than
                // waiting for the fixed-duration fallback timer - see item 5,
                // large files were getting clipped by a few seconds on slow
                // buffering starts.
                globalMantraPlayer.onended = () => {
                    globalMantraPlayer.onended = null;
                    if (state === "MANTRA_ACTIVE") changeState("AMBIENT", AMBIENT_IDLE_MS);
                };
                globalMantraPlayer.play().then(() => {
                    console.log("Playing Mouse Loop: " + track.file);
                }).catch((err) => {
                    isRealMantraPlaying = false;
                    globalMantraPlayer.onended = null;
                    console.log(track.file + " failed to play, using synth:", err);
                    synthesizeHum(220, track.duration / 1000);
                });
            }

            changeState("MANTRA_ACTIVE", track.duration);
        }
 
        function triggerFeetMantra() {
            if (isPhysicalESP) sendESPControl('feet');

            initAudio();
            stopHum(); // Stop any currently playing audio / synth hum
            ensureMantraAudioGraph();

            triggerFeetFlash();

            const track = mantraTracks[feetStep];
            isRealFeetMantraPlaying = true;
            const BELL_LEAD_MS = 900; // let the bell ring out before the chant begins
            startNowPlaying(track.file, track.duration);

            // Same reasoning as triggerMantra(): the physical device's real
            // speaker already handles actual audio (bell + chant); its web
            // server can't serve the real mp3/bell files, so mirroring
            // locally here would just fall back to a synthesized
            // approximation - only do this in the standalone simulator.
            if (!isPhysicalESP) {
                // "Entering the temple" moment: ring the bell first, just
                // like a devotee would on stepping in.
                playBellTone(250, 1.8);

                setTimeout(() => {
                    globalMantraPlayer.src = track.file;
                    globalMantraPlayer.volume = volume / 30;
                    // Advance the instant the track actually finishes, rather
                    // than waiting for the fixed-duration fallback timer.
                    globalMantraPlayer.onended = () => {
                        globalMantraPlayer.onended = null;
                        advanceFromFeetActive();
                    };
                    globalMantraPlayer.play().then(() => {
                        console.log("Playing Feet Loop: " + track.file);
                    }).catch((err) => {
                        isRealFeetMantraPlaying = false;
                        globalMantraPlayer.onended = null;
                        console.log(track.file + " failed to play, using synth:", err);
                        synthesizeHum(220, track.duration / 1000);
                    });
                }, BELL_LEAD_MS);
            }

            changeState("FEET_ACTIVE", track.duration + BELL_LEAD_MS);
        }

        // Virtual Puja & Prayer Triggers
        let selectedOffering = "hibiscus";

        function selectOffering(type) {
            selectedOffering = type;
            const options = ['hibiscus', 'garland', 'modak', 'coconut'];
            options.forEach(opt => {
                const btn = document.getElementById(`opt-${opt}`);
                if (opt === type) {
                    btn.classList.add('active-offering');
                } else {
                    btn.classList.remove('active-offering');
                }
            });
        }

        // --- Resume-after-offering support (issue: same song must continue) --
        // Captures what was playing the moment an offering interrupts, BEFORE
        // stopHum() wipes it (stopHum resets currentTime to 0), so the exact
        // track can resume from the exact position after the 12s display.
        let pausedTrackInfo = null;
        function capturePausedTrack() {
            if ((state === "MANTRA_ACTIVE" || state === "FEET_ACTIVE") &&
                globalMantraPlayer.src && !globalMantraPlayer.paused) {
                pausedTrackInfo = {
                    src: globalMantraPlayer.src,
                    time: globalMantraPlayer.currentTime,
                    state: state
                };
            } else {
                pausedTrackInfo = null;
            }
        }
        function resumePausedTrack() {
            const info = pausedTrackInfo;
            pausedTrackInfo = null;
            if (!info) return false;
            if (autoReturnTimer) clearTimeout(autoReturnTimer);
            if (feetDisplayTimer) clearTimeout(feetDisplayTimer);
            state = info.state;
            updateUI();
            startAmbientRoll(); // blessing display resumes rolling
            if (globalMantraPlayer.src !== info.src) globalMantraPlayer.src = info.src;
            globalMantraPlayer.volume = volume / 30;
            if (info.state === "MANTRA_ACTIVE") isRealMantraPlaying = true;
            else isRealFeetMantraPlaying = true;
            globalMantraPlayer.onended = () => {
                globalMantraPlayer.onended = null;
                isRealMantraPlaying = false;
                isRealFeetMantraPlaying = false;
                changeState("AMBIENT", AMBIENT_IDLE_MS);
            };
            globalMantraPlayer.play().then(() => {
                // Seek AFTER playback starts - seeking a paused element before
                // play() can silently reset to 0, which made the track restart
                // from the beginning instead of continuing where it left off.
                try { globalMantraPlayer.currentTime = info.time; } catch (e) {}
            }).catch(() => {
                changeState("AMBIENT", AMBIENT_IDLE_MS);
            });
            return true;
        }

        function submitPuja() {
            const nameInput = document.getElementById('puja-name-input');
            const textInput = document.getElementById('puja-input');

            const name = nameInput.value.trim() !== "" ? nameInput.value.trim() : "Anonymous Devotee";
            const text = textInput.value.trim();

            initAudio();
            capturePausedTrack();
            stopHum();

            // Ring the temple bell
            playBellTone();

            const displayMsg = formatPersonalizedBlessing(name, selectedOffering, text);

            // Save previous state before entering feet active devotee display (only if not already in feet display)
            if (state !== "FEET_ACTIVE") {
                window.ganeshaPrevState = state;
            }

            // Enter FEET_ACTIVE state to show the personalized gold text (12 seconds)
            changeState("FEET_ACTIVE", 12000, displayMsg);

            // Actually add this to the Priest Queue - this step was missing
            // entirely before, so submitting here never created anything
            // for a priest to approve/reject. Same shape/relay pattern as
            // puja.html's (the separate devotee-facing page) submitPuja(),
            // which already had this right.
            const newRequest = {
                id: Date.now().toString() + Math.random().toString(36).substr(2, 5),
                timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
                name: name,
                offering: selectedOffering,
                prayer: text
            };
            let localQueue = JSON.parse(localStorage.getItem('ganesha_puja_queue') || '[]');
            localQueue.push(newRequest);
            localStorage.setItem('ganesha_puja_queue', JSON.stringify(localQueue));
            localStorage.setItem('ganesha_puja_queue_trigger', Date.now().toString());
            renderQueue();

            // Best-effort background sync so other devices/tabs see it too -
            // same relay pattern used by approveQueueItem()/rejectQueueItem().
            const appKey = "sxnoamwe";
            const itemKey = "ganesha_queue";
            const readUrl = `https://keyvalue.immanuel.co/api/KeyVal/GetValue/${appKey}/${itemKey}?_t=${Date.now()}`;
            fetch(readUrl)
                .then(res => res.json())
                .then(b64data => {
                    let onlineQueue = [];
                    if (b64data && b64data.trim() !== "" && b64data !== "test_value" && b64data !== "[]") {
                        try { onlineQueue = JSON.parse(base64UrlDecode(b64data)); } catch (e) {}
                    }
                    if (!Array.isArray(onlineQueue)) onlineQueue = [];
                    onlineQueue.push(newRequest);
                    const encodedVal = base64UrlEncode(JSON.stringify(onlineQueue));
                    const writeUrl = `https://keyvalue.immanuel.co/api/KeyVal/UpdateValue/${appKey}/${itemKey}/${encodedVal}`;
                    return fetch(writeUrl, { method: 'POST' });
                })
                .catch(err => console.warn("Background relay sync failed for new offering (still added locally):", err));

            // Clear input after submission
            textInput.value = "";
            nameInput.value = "";
        }

        // Squeaky Talk-Back Output
        function speakMouseRepeat(text) {
            if (!text || text.trim() === "") {
                if (pirEnabled) changeState("AMBIENT", 20000);
                else changeState("STANDBY");
                return;
            }

            changeState("AMBIENT", 15000);
            setOledText(`   Mouse says: "${text}"   `);

            // Use browser speech synthesis to repeat back with high pitch & rate
            const utterance = new SpeechSynthesisUtterance(text);
            
            // Language selection for voice output
            if (selectedLang === "ta") utterance.lang = 'ta-IN';
            else if (selectedLang === "sa" || selectedLang === "mr") utterance.lang = 'hi-IN';
            else utterance.lang = 'en-US';

            utterance.pitch = 2.0; // SQUEAKY!
            utterance.rate = 1.35;  // FASTER!
            utterance.volume = volume / 30;

            window.speechSynthesis.speak(utterance);
        }

        function triggerStop() {
            window.speechSynthesis.cancel();
            pausedTrackInfo = null; // a manual stop discards any pending resume
            // Always reach the real device first and unconditionally, so this
            // works as a guaranteed recovery button even if this browser's
            // local state tracking has drifted out of sync with the device
            // (e.g. missed/failed polls) - stopAudioAndStandby() on the
            // firmware side resets to STANDBY regardless of its prior state.
            if (isPhysicalESP) sendESPControl('stop');
            if (state === "STANDBY") {
                // Second press while already stopped: close the temple down
                // completely (night mode) - the "exit" gesture.
                closeTemple();
                return;
            }
            changeState("STANDBY");
        }

        function triggerPIR() {
            if (pirEnabled && state === "STANDBY") {
                initAudio();
                playBellTone(250, 1.5);
                if (isPhysicalESP) sendESPControl('pir');
                changeState("AMBIENT", AMBIENT_IDLE_MS);
            }
        }

        // What runs once the Aarti chant finishes - set by whichever path
        // started the ritual (idle timeout vs. manual wake), defaults to
        // settling into a normal ready Ambient state if nothing set it.
        let aartiOnComplete = null;

        // Shared "temple wakes up" ritual: 3 quick bells, then the full Aarti.
        function ringWakeBells(remaining) {
            if (remaining <= 0) {
                changeState("AARTI_MODE");
                return;
            }
            playBellTone(250, 1.0);
            setTimeout(() => ringWakeBells(remaining - 1), 900);
        }

        // Idle timeout (nobody around for a while while Ambient): 3 bells,
        // Aarti, then close the temple down for the night - the Aarti mode
        // itself is what triggers closing afterward.
        function triggerAartiMode() {
            if (state !== "AMBIENT") return;
            aartiOnComplete = () => changeState("TEMPLE_CLOSED");
            ringWakeBells(3);
        }

        // --- Temple Closed (Night Mode) --------------------------------
        function evaluateCloseHold() {
            if (mouseDownAt && feetDownAt && !closeHoldTimer && !closeHoldTriggered && state !== "TEMPLE_CLOSED") {
                closeHoldMessageShown = true;
                setOledText("   Hold both to close the temple for the night...   ");
                closeHoldTimer = setTimeout(() => {
                    closeHoldTriggered = true;
                    closeHoldTimer = null;
                    closeTemple();
                }, CLOSE_HOLD_MS);
            }
        }
        function cancelCloseHold() {
            if (closeHoldTimer) { clearTimeout(closeHoldTimer); closeHoldTimer = null; }
        }
        function finalizePadRelease() {
            if (mouseDownAt || feetDownAt) return;
            if (closeHoldMessageShown && !closeHoldTriggered) {
                closeHoldMessageShown = false;
                if (state === "AMBIENT" || state === "MANTRA_ACTIVE" || state === "FEET_ACTIVE") {
                    startAmbientRoll();
                }
            }
            closeHoldTriggered = false;
        }

        function closeTemple() {
            if (state === "TEMPLE_CLOSED") return;
            initAudio();
            // Closing the temple - manually or automatically - always plays
            // the Aarti chant first (3 bells, then GaneshAarti.mp3), then
            // settles into TEMPLE_CLOSED once it finishes.
            aartiOnComplete = () => changeState("TEMPLE_CLOSED");
            ringWakeBells(3);
        }
        // Manual wake (temple was closed, someone touched a pad): same 3
        // bells + Aarti ritual as the idle trigger, but settles into a
        // normal ready Ambient state afterward instead of closing again.
        function openTemple() {
            // Always reach the real device, even if this browser's local
            // state tracking has drifted out of sync with it (e.g. missed
            // polls) - openTempleFromClosed() on the firmware side only
            // acts if IT thinks it's actually closed, so this is safe to
            // send regardless of what this browser currently believes.
            if (isPhysicalESP) sendESPControl('open');
            if (state !== "TEMPLE_CLOSED") return;

            initAudio();
            blessings++;

            // Quick reopening greeting: bell + one short mantra preview
            // (mantraTracks[0] specifically, picked for testing - not the
            // rotating mouseStep), NOT the full ~4-minute Aarti ritual this
            // used to reuse from closeTemple() via ringWakeBells()+
            // AARTI_MODE - which meant opening could idle its way into
            // ANOTHER auto-close within minutes. Only previewed locally in
            // the standalone simulator; the physical device's real speaker
            // plays its own version via /api/control?action=open above.
            if (!isPhysicalESP) {
                playBellTone(250, 1.0);
                setTimeout(() => {
                    const track = mantraTracks[0];
                    globalMantraPlayer.src = track.file;
                    globalMantraPlayer.volume = volume / 30;
                    startNowPlaying(track.file, track.duration);
                    globalMantraPlayer.play().catch(() => synthesizeHum(220, track.duration / 1000));
                }, 900);
            }

            changeState("AMBIENT", AMBIENT_IDLE_MS);
        }

        function onMousePadDown() {
            mouseDownAt = Date.now();
            evaluateCloseHold();
        }
        function onMousePadUp() {
            const wasDown = mouseDownAt !== 0;
            const held = mouseDownAt ? Date.now() - mouseDownAt : 0;
            mouseDownAt = 0;
            cancelCloseHold();
            if (!closeHoldTriggered) {
                if (state === "TEMPLE_CLOSED") {
                    openTemple();
                } else if (wasDown && held < CLOSE_HOLD_MS) {
                    triggerMantra();
                }
            }
            finalizePadRelease();
        }
        function onMousePadCancel() {
            mouseDownAt = 0;
            cancelCloseHold();
            finalizePadRelease();
        }

        function onFeetPadDown() {
            feetDownAt = Date.now();
            evaluateCloseHold();
        }
        function onFeetPadUp() {
            const wasDown = feetDownAt !== 0;
            const held = feetDownAt ? Date.now() - feetDownAt : 0;
            feetDownAt = 0;
            cancelCloseHold();
            if (!closeHoldTriggered) {
                if (state === "TEMPLE_CLOSED") {
                    openTemple();
                } else if (wasDown && held < CLOSE_HOLD_MS) {
                    triggerFeetMantra();
                }
            }
            finalizePadRelease();
        }
        function onFeetPadCancel() {
            feetDownAt = 0;
            cancelCloseHold();
            finalizePadRelease();
        }

        function updateBrightness() {
            brightness = document.getElementById('bright-slider').value;
            if (isPhysicalESP) espGet(`/api/leds?brightness=${brightness}`);
            updateUI();
            previewLeds();
        }

        function updatePattern() {
            pattern = document.getElementById('pattern-select').value;
            if (isPhysicalESP) espGet(`/api/leds?pattern=${pattern}`);
            previewLeds();
        }

        let lastVolumeBlip = 0;
        function updateVolume() {
            volume = document.getElementById('vol-slider').value;
            if (isPhysicalESP) espGet(`/api/audio?volume=${volume}`);
            updateUI();

            const volRatio = volume / 30;

            // Apply live to whatever's actually playing right now - previously
            // volume was only read once at the moment a mantra started.
            if (isRealMantraPlaying || isRealFeetMantraPlaying) {
                globalMantraPlayer.volume = volRatio;
            }
            if (soundSourceAmp && audioCtx) {
                soundSourceAmp.gain.cancelScheduledValues(audioCtx.currentTime);
                soundSourceAmp.gain.setValueAtTime(0.12 * volRatio, audioCtx.currentTime);
            }

            // Audible preview blip so volume changes are noticeable even when
            // nothing is currently playing. Isolated oscillator/gain so it can
            // never collide with an active mantra hum. Debounced so dragging
            // doesn't stack up overlapping tones.
            const now = Date.now();
            if (now - lastVolumeBlip > 150) {
                lastVolumeBlip = now;
                initAudio();
                if (audioCtx && volume > 0) {
                    const t = audioCtx.currentTime;
                    const blipOsc = audioCtx.createOscillator();
                    const blipGain = audioCtx.createGain();
                    blipOsc.type = "sine";
                    blipOsc.frequency.setValueAtTime(440, t);
                    blipGain.gain.setValueAtTime(0.15 * volRatio, t);
                    blipGain.gain.exponentialRampToValueAtTime(0.001, t + 0.12);
                    blipOsc.connect(blipGain);
                    blipGain.connect(audioCtx.destination);
                    blipOsc.start(t);
                    blipOsc.stop(t + 0.13);
                }
            }
        }

        // Toggle functions
        function togglePIR() {
            pirEnabled = document.getElementById('pir-toggle').checked;
            if (isPhysicalESP) espGet(`/api/settings?pir=${pirEnabled ? 1 : 0}`);
        }

        // Priest Queue Logic
        let lastQueueCount = -1; // Track queue count for audio notifications

        function renderQueue() {
            const queueList = document.getElementById('queue-list');
            const queueStatus = document.getElementById('queue-status');
            
            // Read from local storage (for single-tab offline/local testing)
            let localQueue = JSON.parse(localStorage.getItem('ganesha_puja_queue') || '[]');
            
            // Fetch from online relay to sync mobile phone submissions
            const appKey = "sxnoamwe";
            const itemKey = "ganesha_queue";
            const readUrl = `https://keyvalue.immanuel.co/api/KeyVal/GetValue/${appKey}/${itemKey}?_t=${Date.now()}`;
            
            fetch(readUrl)
                .then(res => res.json())
                .then(b64data => {
                    let onlineQueue = [];
                    if (b64data && b64data.trim() !== "" && b64data !== "test_value" && b64data !== "[]") {
                        try {
                            onlineQueue = JSON.parse(base64UrlDecode(b64data));
                        } catch (e) {
                            console.warn("Error decoding online queue, resetting:", e);
                        }
                    }
                    if (!Array.isArray(onlineQueue)) onlineQueue = [];
                    
                    // Deduplicate items based on ID
                    const mergedQueue = [...onlineQueue];
                    localQueue.forEach(l => {
                        if (!mergedQueue.some(o => o.id === l.id)) {
                            mergedQueue.push(l);
                        }
                    });
                    
                    // 🔔 Audio Notification: Play Ganesha's bell tone if a new item arrives in queue
                    if (mergedQueue.length > lastQueueCount) {
                        if (lastQueueCount !== -1) { // Avoid ringing on initial page load
                            playBellTone();
                        }
                    }
                    lastQueueCount = mergedQueue.length;
                    
                    queueList.innerHTML = '';
                    
                    if (mergedQueue.length === 0) {
                        queueStatus.innerText = 'No pending offerings.';
                        return;
                    }
                    
                    queueStatus.innerText = `${mergedQueue.length} pending offering(s) in queue:`;
                    
                    mergedQueue.forEach(item => {
                        const itemDiv = document.createElement('div');
                        itemDiv.className = 'queue-item';
                        
                        const emojiMap = {
                            hibiscus: '🌺 Flower',
                            garland: '🌼 Garland',
                            modak: '🥟 Modak',
                            coconut: '🥥 Coconut'
                        };
                        const offeringLabel = emojiMap[item.offering] || item.offering;
                        const devoteeName = item.name || "Anonymous Devotee";
                        
                        itemDiv.innerHTML = `
                            <div class="queue-item-header">
                                <span>${offeringLabel}</span>
                                <span class="queue-item-time">${item.timestamp}</span>
                            </div>
                            <div style="font-size: 8px; color: var(--accent-teal); font-weight: bold; text-align: left; margin: 2px 0;">From: ${devoteeName}</div>
                            <div class="queue-item-text">
                                ${item.prayer !== "" ? `"${item.prayer}"` : "(No wish text)"}
                            </div>
                            <div class="queue-item-actions">
                                <button class="btn-approve" onclick="approveQueueItem('${item.id}')">✨ Approve & Offer</button>
                                <button class="btn-reject" onclick="rejectQueueItem('${item.id}')">❌ Reject</button>
                            </div>
                        `;
                        queueList.appendChild(itemDiv);
                    });
                })
                .catch(err => {
                    console.warn("Relay fetch failed, rendering local cache:", err);
                    // Fallback to local storage rendering
                    queueList.innerHTML = '';
                    if (localQueue.length === 0) {
                        queueStatus.innerText = 'No pending offerings.';
                        return;
                    }
                    queueStatus.innerText = `${localQueue.length} pending (offline):`;
                    localQueue.forEach(item => {
                        const itemDiv = document.createElement('div');
                        itemDiv.className = 'queue-item';
                        const emojiMap = { hibiscus: '🌺 Flower', garland: '🌼 Garland', modak: '🥟 Modak', coconut: '🥥 Coconut' };
                        const offeringLabel = emojiMap[item.offering] || item.offering;
                        const devoteeName = item.name || "Anonymous Devotee";
                        itemDiv.innerHTML = `
                            <div class="queue-item-header">
                                <span>${offeringLabel}</span>
                                <span class="queue-item-time">${item.timestamp}</span>
                            </div>
                            <div style="font-size: 8px; color: var(--accent-teal); font-weight: bold; text-align: left; margin: 2px 0;">From: ${devoteeName}</div>
                            <div class="queue-item-text">${item.prayer !== "" ? `"${item.prayer}"` : "(No wish text)"}</div>
                            <div class="queue-item-actions">
                                <button class="btn-approve" onclick="approveQueueItem('${item.id}')">✨ Approve & Offer</button>
                                <button class="btn-reject" onclick="rejectQueueItem('${item.id}')">❌ Reject</button>
                            </div>
                        `;
                        queueList.appendChild(itemDiv);
                    });
                });
        }

        function approveQueueItem(id) {
            // Act locally FIRST and immediately - a priest tapping Approve
            // should always work even if the network relay is slow,
            // unreachable, or down. Network sync happens afterward, best-effort.
            let localQueue = JSON.parse(localStorage.getItem('ganesha_puja_queue') || '[]');
            const indexLocal = localQueue.findIndex(item => item.id === id);
            const item = indexLocal !== -1 ? localQueue[indexLocal] : null;
            if (indexLocal !== -1) localQueue.splice(indexLocal, 1);
            localStorage.setItem('ganesha_puja_queue', JSON.stringify(localQueue));
            localStorage.setItem('ganesha_puja_queue_trigger', Date.now().toString());

            if (item) {
                initAudio();
                capturePausedTrack();
                stopHum();
                playBellTone(); // Ring the temple bell tone
                const displayMsg = formatPersonalizedBlessing(item.name, item.offering, item.prayer);
                if (state !== "FEET_ACTIVE") {
                    window.ganeshaPrevState = state;
                }
                changeState("FEET_ACTIVE", 12000, displayMsg);

                // Show the devotee's actual name/offering/prayer on the
                // physical OLED too, not just this browser's local preview.
                if (isPhysicalESP) {
                    const params = `&name=${encodeURIComponent(item.name)}&offering=${encodeURIComponent(item.offering)}&prayer=${encodeURIComponent(item.prayer)}`;
                    sendESPControl('offering', params);
                }
            }
            renderQueue();

            // Best-effort background sync so other devices see the removal too.
            const appKey = "sxnoamwe";
            const itemKey = "ganesha_queue";
            const readUrl = `https://keyvalue.immanuel.co/api/KeyVal/GetValue/${appKey}/${itemKey}?_t=${Date.now()}`;
            fetch(readUrl)
                .then(res => res.json())
                .then(b64data => {
                    let onlineQueue = [];
                    if (b64data && b64data.trim() !== "" && b64data !== "test_value" && b64data !== "[]") {
                        try { onlineQueue = JSON.parse(base64UrlDecode(b64data)); } catch (e) {}
                    }
                    if (!Array.isArray(onlineQueue)) onlineQueue = [];
                    const indexOnline = onlineQueue.findIndex(oi => oi.id === id);
                    if (indexOnline !== -1) onlineQueue.splice(indexOnline, 1);
                    const encodedVal = base64UrlEncode(JSON.stringify(onlineQueue));
                    const writeUrl = `https://keyvalue.immanuel.co/api/KeyVal/UpdateValue/${appKey}/${itemKey}/${encodedVal}`;
                    return fetch(writeUrl, { method: 'POST' });
                })
                .then(() => renderQueue())
                .catch(err => console.warn("Background relay sync failed for approve (item still removed locally):", err));
        }

        function rejectQueueItem(id) {
            // Same local-first pattern as approve.
            let localQueue = JSON.parse(localStorage.getItem('ganesha_puja_queue') || '[]');
            const indexLocal = localQueue.findIndex(item => item.id === id);
            if (indexLocal !== -1) localQueue.splice(indexLocal, 1);
            localStorage.setItem('ganesha_puja_queue', JSON.stringify(localQueue));
            localStorage.setItem('ganesha_puja_queue_trigger', Date.now().toString());
            renderQueue();

            const appKey = "sxnoamwe";
            const itemKey = "ganesha_queue";
            const readUrl = `https://keyvalue.immanuel.co/api/KeyVal/GetValue/${appKey}/${itemKey}?_t=${Date.now()}`;
            fetch(readUrl)
                .then(res => res.json())
                .then(b64data => {
                    let onlineQueue = [];
                    if (b64data && b64data.trim() !== "" && b64data !== "test_value" && b64data !== "[]") {
                        try { onlineQueue = JSON.parse(base64UrlDecode(b64data)); } catch (e) {}
                    }
                    if (!Array.isArray(onlineQueue)) onlineQueue = [];
                    const indexOnline = onlineQueue.findIndex(oi => oi.id === id);
                    if (indexOnline !== -1) onlineQueue.splice(indexOnline, 1);
                    const encodedVal = base64UrlEncode(JSON.stringify(onlineQueue));
                    const writeUrl = `https://keyvalue.immanuel.co/api/KeyVal/UpdateValue/${appKey}/${itemKey}/${encodedVal}`;
                    return fetch(writeUrl, { method: 'POST' });
                })
                .then(() => renderQueue())
                .catch(err => console.warn("Background relay sync failed for reject (item still removed locally):", err));
        }

        function clearAllQueue() {
            const appKey = "sxnoamwe";
            const itemKey = "ganesha_queue";
            localStorage.setItem('ganesha_puja_queue', JSON.stringify([]));
            localStorage.setItem('ganesha_puja_queue_trigger', Date.now().toString());
            renderQueue();

            const encodedVal = base64UrlEncode(JSON.stringify([]));
            const writeUrl = `https://keyvalue.immanuel.co/api/KeyVal/UpdateValue/${appKey}/${itemKey}/${encodedVal}`;
            fetch(writeUrl, { method: 'POST' })
                .then(() => renderQueue())
                .catch(err => console.warn("Background relay sync failed for clear (queue still cleared locally):", err));
        }

        // Listen to storage events to keep tabs synced in real-time
        window.addEventListener('storage', (e) => {
            if (e.key === 'ganesha_puja_queue' || e.key === 'ganesha_puja_queue_trigger') {
                renderQueue();
            }
        });

        // Start animation loop
        drawLeds();
        // Render initial queue list
        renderQueue();
        // Render the static song library list (item 3)
        renderSongLibrary();
        // Devotee QR Code - only meaningful on the physical device, since
        // /puja only exists on the ESP32's own web server (the standalone
        // simulator has no real device for it to point to).
        if (isPhysicalESP) {
            const pujaUrl = window.location.origin + '/puja';
            new QRCode(document.getElementById('qr-code-container'), {
                text: pujaUrl,
                width: 160,
                height: 160,
                colorDark: '#000000',
                colorLight: '#ffffff',
                correctLevel: QRCode.CorrectLevel.M
            });
            document.getElementById('qr-code-status').innerText = pujaUrl;
        } else {
            document.getElementById('qr-code-container').innerText = '(only available on the physical device)';
            document.getElementById('qr-code-container').style.cssText = 'padding: 20px; color: #888; font-size: 10px;';
        }
        // Poll online database queue every 4 seconds
        setInterval(renderQueue, 4000);
        // Start in Standby
        changeState("STANDBY");

        // When running on the physical idol, mirror its live status here so
        // the dashboard reflects real touch/PIR-triggered activity too, not
        // just what this browser session has done. Deliberately only syncs
        // status display (updateUI + control values) - it does not replay
        // audio/LED side effects locally, since /api/state doesn't say which
        // track is playing.
        function pollDeviceState() {
            fetch('/api/state').then(r => r.json()).then(data => {
                const prevState = state;
                const wasActive = (prevState === "MANTRA_ACTIVE" || prevState === "FEET_ACTIVE" || prevState === "AARTI_MODE");
                const nowIdle = (data.state === "STANDBY" || data.state === "AMBIENT" || data.state === "TEMPLE_CLOSED");

                state = data.state;
                blessings = data.blessings;
                brightness = data.brightness;
                pattern = data.pattern;
                volume = data.volume;
                pirEnabled = data.pirEnabled;
                selectedLang = CODE_TO_LANG[data.lang] || selectedLang;
                selectedTheme = THEME_ORDER[data.theme] || selectedTheme;

                const brightSlider = document.getElementById('bright-slider');
                const patternSelect = document.getElementById('pattern-select');
                const volSlider = document.getElementById('vol-slider');
                const pirToggle = document.getElementById('pir-toggle');
                const langSelect = document.getElementById('lang-select');
                const themeSelect = document.getElementById('theme-select');
                if (brightSlider) brightSlider.value = brightness;
                if (patternSelect) patternSelect.value = pattern;
                if (volSlider) volSlider.value = volume;
                if (pirToggle) pirToggle.checked = pirEnabled;
                if (langSelect) langSelect.value = selectedLang;
                if (themeSelect) themeSelect.value = selectedTheme;

                // The real device already left playback on its own (its
                // onboard AMBIENT timeout is 30s, well ahead of this
                // browser's 120s local timer) - stop any locally-playing
                // audio/timers left over from when this browser initiated
                // that same trigger, so the dashboard doesn't keep humming
                // or showing "now playing" after the device moved on.
                if (wasActive && nowIdle) {
                    if (autoReturnTimer) clearTimeout(autoReturnTimer);
                    if (feetDisplayTimer) clearTimeout(feetDisplayTimer);
                    if (aartiTimer) clearTimeout(aartiTimer);
                    stopHum();
                    stopNowPlaying();
                }

                // The device can also enter AARTI_MODE entirely on its own
                // (the firmware's autonomous idle-timeout close), with no
                // local button click to have called startNowPlaying() - so
                // the panel would otherwise sit on "Nothing playing" even
                // though Aarti is genuinely running. If we don't already
                // know what's playing, assume it's the Aarti track once we
                // see AARTI_MODE reported back.
                if (data.state === "AARTI_MODE" && nowPlayingFile === null) {
                    startNowPlaying(AARTI_TRACK_FILE, AARTI_FALLBACK_DURATION_MS);
                }

                // True content sync: the firmware is now the single source of
                // truth for what text is showing (see /api/state's
                // "blessing" field, straight from its own scrollText) -
                // display exactly that instead of running an independent
                // local rotation, so the physical OLED and the dashboard
                // always show the same line at the same time.
                if (data.blessing && data.blessing !== currentText &&
                    (data.state === "AMBIENT" || data.state === "MANTRA_ACTIVE" ||
                     data.state === "FEET_ACTIVE" || data.state === "AARTI_MODE")) {
                    setOledText(data.blessing);
                }

                updateUI();
            }).catch(() => {});
        }
        if (isPhysicalESP) {
            pollDeviceState();
            setInterval(pollDeviceState, 2000);
        }
    </script>
</body>
</html>

)rawliteral";

#endif
