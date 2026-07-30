#!/usr/bin/env bash
# Compile-check GanapatiAI without an Arduino IDE or a physical board.
#
# Exists because r10-r24 were all shipped verified only by reading code and
# counting braces. The first real compile (r24) came back clean, but that was
# luck as much as care - this makes it repeatable.
#
# Two environment quirks it works around, both from a restricted network:
#   1. downloads.arduino.cc is blocked, so the Arduino package index and the
#      bundled tools (ctags, discoveries) cannot be fetched. Only ONE tool
#      from that package is actually needed - arduino:dfu-util, which is a
#      DFU *upload* tool and irrelevant to compiling - so it is satisfied
#      with a local stub served over 127.0.0.1.
#   2. Arduino's ctags (a patched 5.8) is unavailable, and stock
#      exuberant-ctags emits malformed prototypes. Rather than fight it, the
#      .ino is wrapped: prototypes are generated here and injected after the
#      include block (so they can see types from config.h), and the sketch
#      becomes a one-line #include. Identical translation unit, no ctags.
#
# Core is 3.1.3 deliberately: the device's own boot log shows FastLED's
# rmt_5 driver, which is only selected under ESP-IDF 5.x = arduino-esp32 3.x.
set -euo pipefail

FW="$(cd "$(dirname "$0")/.." && pwd)"
BUILD=/tmp/ganapati_compile_check
FQBN="esp32:esp32:esp32:PartitionScheme=huge_app"

command -v arduino-cli >/dev/null || { echo "arduino-cli not installed"; exit 1; }

rm -rf "$BUILD"; mkdir -p "$BUILD/GanapatiAI"
python3 - "$FW" "$BUILD" <<'PYEOF'
import re, sys
fw, build = sys.argv[1], sys.argv[2]
src = open(f'{fw}/GanapatiAI.ino').read()
lines = src.split('\n')
pat = re.compile(
    r'^((?:const\s+)?(?:unsigned\s+)?(?:void|int|bool|long|char|float|String|SystemState|CRGB)'
    r'(?:\s*\*)?)\s+([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*\{', re.MULTILINE)
protos, seen = [], set()
for m in pat.finditer(src):
    ret, name, args = m.group(1), m.group(2), m.group(3)
    if name in ('setup', 'loop') or name in seen:
        continue
    seen.add(name)
    protos.append(f"{ret} {name}({args.strip()});")
last_inc = max(i for i, l in enumerate(lines) if l.strip().startswith('#include'))
while last_inc + 1 < len(lines) and lines[last_inc + 1].strip() in ('#endif', ''):
    if lines[last_inc + 1].strip() == '#endif':
        last_inc += 1
        break
    last_inc += 1
block = ['', '// --- auto-generated prototypes (stand-in for the ctags pass) ---'] + protos + ['']
out = lines[:last_inc + 1] + block + lines[last_inc + 1:]
open(f'{build}/GanapatiAI/ganapati_impl.h', 'w').write(
    "#pragma once\n#include <Arduino.h>\n" + '\n'.join(out))
open(f'{build}/GanapatiAI/GanapatiAI.ino', 'w').write('#include "ganapati_impl.h"\n')
print(f"injected {len(protos)} prototypes")
PYEOF

cp "$FW"/config.h "$FW"/web_dashboard.h "$FW"/puja_page.h "$BUILD/GanapatiAI/"

arduino-cli compile --warnings all --fqbn "$FQBN" \
  --library ~/Arduino/libraries/FastLED \
  --library ~/Arduino/libraries/U8g2 \
  --library ~/Arduino/libraries/DFRobotDFPlayerMini \
  "$BUILD/GanapatiAI" 2>&1 \
  | grep -viE "Error initializing instance|library_index|Downloading index|Multiple libraries|^  (Used|Not used):"

echo
echo "Any 'warning:' or 'error:' naming ganapati_impl.h / config.h /"
echo "web_dashboard.h / puja_page.h above is real and in our code."
