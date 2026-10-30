#!/usr/bin/env python3
"""
Hardware-in-the-loop test for IGNACC_Controller.

Talks to the flashed board over its USB-UART bridge and checks every protocol
response. The relays WILL click during the test.

    python tools/hil_test.py --port COM15
    python tools/hil_test.py --port COM15 --no-reset   # don't pulse EN first

Requires pyserial (already in the ESP-IDF Python environment).
"""
import argparse
import pathlib
import re
import sys
import time

import serial

ROOT = pathlib.Path(__file__).resolve().parent.parent
BAUD = 115200
SETTLE_MS = 15          # CONFIG_IGNACC_RELAY_SETTLE_MS default
MAX_LINE_LEN = 32       # CONFIG_IGNACC_MAX_LINE_LEN default
REPLY_TIMEOUT = 1.0
SILENCE_WINDOW = 0.4


def project_version() -> str:
    text = (ROOT / "CMakeLists.txt").read_text()
    match = re.search(r'set\(PROJECT_VER\s+"([^"]+)"\)', text)
    if not match:
        sys.exit("PROJECT_VER not found in CMakeLists.txt")
    return match.group(1)


class Board:
    def __init__(self, port: str):
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.baudrate = BAUD
        self.ser.timeout = REPLY_TIMEOUT
        # Deassert DTR/RTS before opening so the auto-reset circuit
        # (EN / GPIO0) is not triggered by opening the port.
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.open()

    def close(self):
        self.ser.close()

    def hard_reset(self) -> bytes:
        """Pulse EN via RTS and return whatever the board prints while booting."""
        self.ser.dtr = False
        self.ser.rts = True     # EN low
        time.sleep(0.1)
        self.ser.rts = False    # EN high -> boot
        time.sleep(1.0)
        return self.ser.read(self.ser.in_waiting or 1)

    def send_raw(self, data: bytes):
        self.ser.reset_input_buffer()
        # No flush(): on Windows/CP210x it can block ~50 ms and skew the timing.
        self.ser.write(data)

    def command(self, line: str, eol: str = "\r\n"):
        """Send one line, return (reply_without_crlf | None, elapsed_ms, raw)."""
        self.send_raw((line + eol).encode())
        start = time.perf_counter()
        raw = self.ser.read_until(b"\r\n")
        elapsed = (time.perf_counter() - start) * 1000.0
        if not raw.endswith(b"\r\n"):
            return None, elapsed, raw
        return raw[:-2].decode(errors="replace"), elapsed, raw

    def read_lines(self, count: int):
        lines = []
        for _ in range(count):
            raw = self.ser.read_until(b"\r\n")
            if not raw.endswith(b"\r\n"):
                break
            lines.append(raw[:-2].decode(errors="replace"))
        return lines

    def silence(self, window: float = SILENCE_WINDOW) -> bytes:
        """Return bytes received within window (expected: nothing)."""
        time.sleep(window)
        return self.ser.read(self.ser.in_waiting)


class Results:
    def __init__(self):
        self.passed = 0
        self.failed = []

    def check(self, name: str, ok: bool, detail: str = ""):
        status = "PASS" if ok else "FAIL"
        print(f"[{status}] {name}" + (f"  ({detail})" if detail else ""))
        if ok:
            self.passed += 1
        else:
            self.failed.append(name)


def expect(res: Results, board: Board, cmd: str, expected: str, eol: str = "\r\n", min_ms: float = 0.0):
    reply, ms, raw = board.command(cmd, eol)
    ok = reply == expected and ms >= min_ms
    detail = f"got {raw!r} in {ms:.1f} ms"
    if min_ms:
        detail += f", need >= {min_ms:.0f} ms"
    res.check(f"{cmd!r} -> {expected!r}", ok, detail)


def expect_nack(res: Results, board: Board, name: str, data: bytes):
    board.send_raw(data)
    got = board.silence()
    res.check(name, got == b"UNK_NACK\r\n", f"received {got!r}")


def expect_silence(res: Results, board: Board, name: str, data: bytes):
    board.send_raw(data)
    extra = board.silence()
    res.check(name, extra == b"", f"received {extra!r}" if extra else "no reply")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default="COM15")
    parser.add_argument("--no-reset", action="store_true", help="do not pulse EN before testing")
    args = parser.parse_args()

    version = project_version()
    hello = f"Hello IGNACC v{version} READY"
    res = Results()
    board = Board(args.port)
    try:
        # --- Boot ---------------------------------------------------------
        if not args.no_reset:
            boot = board.hard_reset()
            print(f"boot output after reset ({len(boot)} bytes): {boot[:120]!r}")
            res.check("no protocol response emitted during boot",
                      not re.search(rb"_OK|_FAIL|STATUS|Hello", boot))

        # Handshake: the host sends HeyU and waits for Hello before anything else.
        reply = None
        for _ in range(5):
            reply, _, _ = board.command("HeyU")
            if reply == hello:
                break
        res.check("handshake HeyU -> Hello", reply == hello, f"got {reply!r}")

        expect(res, board, "STATUS", "STATUS IN1=OFF IN2=OFF")  # safe default after boot

        # --- Relay commands ------------------------------------------------
        expect(res, board, "IN1_ON", "IN1_ON_OK", min_ms=SETTLE_MS)
        expect(res, board, "STATUS", "STATUS IN1=ON IN2=OFF")
        expect(res, board, "IN2_ON", "IN2_ON_OK", min_ms=SETTLE_MS)
        expect(res, board, "STATUS", "STATUS IN1=ON IN2=ON")
        expect(res, board, "IN1_ON", "IN1_ON_OK", min_ms=SETTLE_MS)  # idempotent
        expect(res, board, "IN1_OFF", "IN1_OFF_OK", min_ms=SETTLE_MS)
        expect(res, board, "STATUS", "STATUS IN1=OFF IN2=ON")
        expect(res, board, "IN2_OFF", "IN2_OFF_OK", min_ms=SETTLE_MS)
        expect(res, board, "IN2_OFF", "IN2_OFF_OK", min_ms=SETTLE_MS)  # idempotent
        expect(res, board, "STATUS", "STATUS IN1=OFF IN2=OFF")

        # --- Line endings --------------------------------------------------
        expect(res, board, "HeyU", hello, eol="\n")
        expect(res, board, "STATUS", "STATUS IN1=OFF IN2=OFF", eol="\n")

        # --- Ignored input -------------------------------------------------
        for junk in ["heyu", "HEYU", "in1_on", "IN1_on", "IN1_ON ", " IN1_ON", "IN3_ON", "garbage", "STATUS?"]:
            expect_nack(res, board, f"{junk!r} -> UNK_NACK", (junk + "\r\n").encode())
        expect_nack(res, board, "embedded NUL -> UNK_NACK", b"IN1_ON\x00\r\n")
        expect_nack(res, board, "short ROM-style line -> UNK_NACK", b"entry 0x400805e4\r\n")
        expect_silence(res, board, "56-char ROM-style line: overlong, dropped silently",
                       b"rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)\r\n")
        expect_silence(res, board, f"overlong line ({MAX_LINE_LEN + 1} chars) dropped",
                       b"A" * (MAX_LINE_LEN + 1) + b"\r\n")
        expect_silence(res, board, "command at tail of overlong line dropped",
                       b"X" * 200 + b"IN1_ON\r\n")
        expect(res, board, "STATUS", "STATUS IN1=OFF IN2=OFF")  # nothing switched

        # --- Partial line split across writes ------------------------------
        board.send_raw(b"IN2_")
        time.sleep(0.05)
        board.ser.write(b"ON\r\n")
        lines = board.read_lines(1)
        res.check("line split across writes", lines == ["IN2_ON_OK"], f"got {lines!r}")

        # --- Pipelined commands (within queue depth) -----------------------
        board.send_raw(b"IN1_ON\r\nSTATUS\r\nIN1_OFF\r\nIN2_OFF\r\n")
        lines = board.read_lines(4)
        want = ["IN1_ON_OK", "STATUS IN1=ON IN2=ON", "IN1_OFF_OK", "IN2_OFF_OK"]
        res.check("pipelined commands answered in order", lines == want, f"got {lines!r}")

        # --- Leave the board safe ------------------------------------------
        expect(res, board, "STATUS", "STATUS IN1=OFF IN2=OFF")
    finally:
        board.close()

    total = res.passed + len(res.failed)
    print(f"\n{res.passed}/{total} checks passed")
    if res.failed:
        print("Failed:\n  " + "\n  ".join(res.failed))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
