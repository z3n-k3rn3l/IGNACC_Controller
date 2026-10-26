#!/usr/bin/env python3
"""
Step-by-step relay pin check, for measuring with a multimeter.

Drives IN1/IN2 one step at a time and tells you what each pin should measure
(multimeter black probe on ESP32 GND, red probe on D26 / D27).

    python tools/pin_check.py --port COM15            # press Enter for each step
    python tools/pin_check.py --port COM15 --hold 5   # auto-advance every 5 s

Close PuTTY / Serial Studio first: only one program can open the COM port.
"""
import argparse
import sys
import time

import serial

STEPS = [
    # (command, what to expect on the pins / relays)
    ("IN1_OFF", None),
    ("IN2_OFF", "Both OFF:  D26 = 3.3 V,  D27 = 3.3 V.  Both relays OFF (LEDs off)."),
    ("IN1_ON",  "IN1 ON:    D26 = 0 V,    D27 = 3.3 V.  Relay 1 ON, relay 2 OFF."),
    ("IN1_OFF", "IN1 OFF:   D26 = 3.3 V,  D27 = 3.3 V.  Both relays OFF."),
    ("IN2_ON",  "IN2 ON:    D26 = 3.3 V,  D27 = 0 V.    Relay 1 OFF, relay 2 ON."),
    ("IN2_OFF", "IN2 OFF:   D26 = 3.3 V,  D27 = 3.3 V.  Both relays OFF."),
]


def open_port(port: str) -> serial.Serial:
    s = serial.Serial()
    s.port = port
    s.baudrate = 115200
    s.timeout = 1.0
    s.dtr = False  # don't touch EN / GPIO0 through the auto-reset circuit
    s.rts = False
    s.open()
    return s


def send(s: serial.Serial, line: str) -> str:
    s.reset_input_buffer()
    s.write((line + "\n").encode())
    return s.read_until(b"\r\n").decode(errors="replace").strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default="COM15")
    parser.add_argument("--hold", type=float, default=0, help="seconds per step (0 = wait for Enter)")
    args = parser.parse_args()

    try:
        s = open_port(args.port)
    except serial.SerialException as exc:
        print(f"Cannot open {args.port}: {exc}\nClose PuTTY / Serial Studio (Disconnect) and retry.")
        return 1
    try:
        hello = send(s, "HeyU")
        if not hello.startswith("Hello IGNACC"):
            print(f"No handshake (got {hello!r}). Is the firmware running and the port free?")
            return 1
        print(f"Board: {hello}\n")

        for cmd, expect in STEPS:
            reply = send(s, cmd)
            if not reply.endswith("_OK"):
                print(f"{cmd} -> {reply!r}  (expected {cmd}_OK)")
                return 1
            if expect is None:
                continue
            status = send(s, "STATUS")
            print(f"> {cmd:8} -> {reply:12} {status}")
            print(f"  MEASURE: {expect}")
            if args.hold > 0:
                time.sleep(args.hold)
            else:
                input("  Press Enter for the next step...")
            print()
        print("Done. Both relays commanded OFF.")
        return 0
    finally:
        s.close()


if __name__ == "__main__":
    sys.exit(main())
