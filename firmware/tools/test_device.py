#!/usr/bin/env python3
"""
GanapatiAI hardware diagnostic utility - menu-driven wrapper around the
firmware's own /api/* diagnostic routes, run from a PC on the same Wi-Fi
network. Every endpoint below is a real, existing route in GanapatiAI.ino
as of r39 - none of this needs a firmware change to use.

Saves typing the same handful of URLs into a browser by hand during
bring-up, which is how most of a day of testing actually happened before
this existed.
"""
import sys
import json

try:
    import requests
except ImportError:
    print("Error: The 'requests' library is required. Install it using: pip install requests")
    sys.exit(1)


def print_header(title):
    print("\n" + "=" * 50)
    print(f"  {title}")
    print("=" * 50)


def main():
    print_header("Ganapati AI Hardware Diagnostic Utility")
    # No sensible default IP exists across sessions - this project's board
    # has landed on a different address after nearly every reboot today.
    # Check the Serial Monitor's "IP Address:" line for the current one.
    ip = input("Enter your ESP32 IP address (see Serial Monitor boot log): ").strip()
    if not ip:
        print("An IP address is required.")
        sys.exit(1)

    base_url = f"http://{ip}"
    print(f"\nConnecting to Ganapati AI at {base_url}...")

    try:
        response = requests.get(f"{base_url}/api/pins", timeout=5)
        if response.status_code == 200:
            print("[SUCCESS] Connected to device successfully!")
        else:
            print(f"[ERROR] Got status code {response.status_code} from /api/pins")
            sys.exit(1)
    except requests.exceptions.RequestException as e:
        print("[ERROR] Connection failed. Check the IP and Wi-Fi connection.")
        print(f"Details: {e}")
        sys.exit(1)

    while True:
        print_header("Diagnostics Menu")
        print("1. Check System State (/api/state)")
        print("2. Check Hardware Pin Levels & Touch Counts (/api/pins)")
        print("3. Test OLED Screen (Solid White Card for 10s)")
        print("4. Test DFPlayer Audio (Play Ganapati Bell Track 3)")
        print("5. Test Custom Audio Track")
        print("6. Simulate PIR Motion Trigger")
        print("7. Check Online Devotee Queue (keyvalue.immanuel.co)")
        print("8. Exit")

        choice = input("\nSelect an option (1-8): ").strip()

        if choice == '1':
            try:
                res = requests.get(f"{base_url}/api/state")
                data = res.json()
                print("\n--- System State ---")
                print(f"State     : {data.get('state')}")
                print(f"Blessing  : {data.get('blessing', '').strip()}")
                print(f"Volume    : {data.get('volume')}/30")
                print(f"Brightness: {data.get('brightness')}/255")
                print(f"Active Trk: Track #{data.get('track')} (Elapsed: {data.get('elapsed')}ms)")
            except Exception as e:
                print(f"Error: {e}")

        elif choice == '2':
            try:
                res = requests.get(f"{base_url}/api/pins")
                data = res.json()
                print("\n--- Pin Diagnostic Report ---")
                print(f"Firmware: {data.get('firmware')}")
                print(f"Uptime  : {data.get('uptime_s')} seconds")

                feet = data.get('feet', {})
                print(f"\n[FEET TOUCH SENSOR] GPIO {feet.get('gpio')}")
                print(f"  Level: {feet.get('level')} (0=idle, 1=touched) | Total Touches: {feet.get('touches')}")

                back = data.get('back', {})
                print(f"\n[MOUSE TOUCH SENSOR] GPIO {back.get('gpio')}")
                print(f"  Level: {back.get('level')} (0=idle, 1=touched) | Total Touches: {back.get('touches')}")

                pir = data.get('pir', {})
                print(f"\n[PIR MOTION SENSOR] GPIO {pir.get('gpio')}")
                print(f"  Level: {pir.get('level')} (0=no motion, 1=motion)")
            except Exception as e:
                print(f"Error: {e}")

        elif choice == '3':
            try:
                res = requests.get(f"{base_url}/api/test?oled=1")
                print(res.text)
            except Exception as e:
                print(f"Error: {e}")

        elif choice == '4':
            try:
                res = requests.get(f"{base_url}/api/test?track=3")
                print(res.text)
            except Exception as e:
                print(f"Error: {e}")

        elif choice == '5':
            track_num = input("Enter track number to play (1-16): ").strip()
            if track_num.isdigit():
                try:
                    res = requests.get(f"{base_url}/api/test?track={track_num}")
                    print(res.text)
                except Exception as e:
                    print(f"Error: {e}")

        elif choice == '6':
            try:
                res = requests.get(f"{base_url}/api/control?action=pir")
                print(res.text)
            except Exception as e:
                print(f"Error: {e}")

        elif choice == '7':
            appKey = "sxnoamwe"
            itemKey = "ganesha_queue"
            readUrl = f"https://keyvalue.immanuel.co/api/KeyVal/GetValue/{appKey}/{itemKey}"
            try:
                res = requests.get(readUrl)
                b64data = res.json()
                if b64data and b64data != "test_value" and b64data != "[]":
                    import base64
                    b64 = b64data.replace('-', '+').replace('_', '/')
                    while len(b64) % 4:
                        b64 += '='
                    decoded = base64.b64decode(b64).decode('utf-8')
                    queue = json.loads(decoded)
                    print(f"\nFound {len(queue)} pending offerings in cloud queue:")
                    for idx, item in enumerate(queue):
                        print(f"  [{idx+1}] Devotee: {item.get('name')} | Offering: {item.get('offering')} | Prayer: {item.get('prayer')}")
                else:
                    print("Cloud queue is empty.")
            except Exception as e:
                print(f"Error: {e}")

        elif choice == '8':
            print("Exiting...")
            break

        input("\nPress Enter to return to menu...")


if __name__ == "__main__":
    main()
