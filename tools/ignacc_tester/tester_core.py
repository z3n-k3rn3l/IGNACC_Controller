"""
IGNACC tester core: serial link to the board and the test catalogue.
No GUI code here, so it also runs headless:

    python tester_core.py --port COM15                 # all tests, 1 s per step
    python tester_core.py --port COM15 --tests 3-5 --delay 0.2
"""
import argparse
import re
import sys
import threading
import time
from dataclasses import dataclass
from typing import Callable, List, Optional

import serial
import serial.tools.list_ports

BAUD = 115200
REPLY_TIMEOUT = 1.0
SILENCE_WINDOW = 0.4
SETTLE_MS = 15      # firmware default CONFIG_IGNACC_RELAY_SETTLE_MS
MAX_LINE_LEN = 32   # firmware default CONFIG_IGNACC_MAX_LINE_LEN
HELLO_RE = re.compile(r"^Hello IGNACC v(\S+) READY$")


def list_ports():
    """[(device, description)] sorted by COM number."""
    def key(p):
        m = re.search(r"(\d+)$", p.device)
        return int(m.group(1)) if m else 0
    return [(p.device, p.description) for p in sorted(serial.tools.list_ports.comports(), key=key)]


class Link:
    """Serial connection to the board. Thread-safe: one transaction at a time."""

    def __init__(self):
        self._ser: Optional[serial.Serial] = None
        self.lock = threading.RLock()

    @property
    def is_open(self) -> bool:
        return self._ser is not None and self._ser.is_open

    def open(self, port: str, baud: int = BAUD):
        s = serial.Serial()
        s.port = port
        s.baudrate = baud
        s.timeout = REPLY_TIMEOUT
        # Deassert DTR/RTS before opening so the devkit's auto-reset circuit
        # (EN / GPIO0) is not triggered.
        s.dtr = False
        s.rts = False
        s.open()
        self._ser = s

    def close(self):
        with self.lock:
            if self._ser is not None:
                try:
                    self._ser.close()
                finally:
                    self._ser = None

    def send_raw(self, data: bytes):
        with self.lock:
            self._ser.reset_input_buffer()
            # No flush(): on Windows it maps to FlushFileBuffers, which on the CP210x
            # driver intermittently blocks ~50 ms and would be counted as latency.
            self._ser.write(data)

    def read_line(self) -> Optional[str]:
        """One response line without CRLF, or None on timeout."""
        with self.lock:
            raw = self._ser.read_until(b"\r\n")
        if not raw.endswith(b"\r\n"):
            return None
        return raw[:-2].decode(errors="replace")

    def command(self, line: str, eol: str = "\r\n"):
        """Send a line; return (reply or None, round-trip ms from write to full reply)."""
        with self.lock:
            start = time.perf_counter()
            self.send_raw((line + eol).encode())
            reply = self.read_line()
            return reply, (time.perf_counter() - start) * 1000.0

    def silence(self, data: bytes, window: float = SILENCE_WINDOW) -> bytes:
        """Send data and return whatever arrives within window (expected: nothing)."""
        with self.lock:
            self.send_raw(data)
            time.sleep(window)
            return self._ser.read(self._ser.in_waiting)

    def hard_reset(self, boot_wait: float = 1.0) -> bytes:
        """Pulse EN through RTS; return the boot output."""
        with self.lock:
            self._ser.dtr = False
            self._ser.rts = True   # EN low
            time.sleep(0.1)
            self._ser.rts = False  # EN high -> boot
            time.sleep(boot_wait)
            return self._ser.read(self._ser.in_waiting)


# ---------------------------------------------------------------------------
# Test runner


@dataclass
class StepResult:
    test: str
    step: str
    ok: bool
    detail: str


class Stopped(Exception):
    pass


class TestContext:
    """Passed to each test: runs steps, reports results, paces with the step delay."""

    def __init__(self, link: Link, delay: float, report: Callable[[StepResult], None], stop: threading.Event,
                 latency_limit_ms: Optional[float] = None, latencies: Optional[List[float]] = None,
                 exclude_settle: bool = False):
        self.link = link
        self.delay = delay
        self.latency_limit_ms = latency_limit_ms
        self.exclude_settle = exclude_settle
        self.latencies = latencies if latencies is not None else []
        self.report = report
        self.stop = stop
        self.test_name = ""
        self.ok = True
        self._first_step = True

    def _pace(self):
        if self.stop.is_set():
            raise Stopped()
        if not self._first_step and self.delay > 0:
            if self.stop.wait(self.delay):
                raise Stopped()
        self._first_step = False

    def record(self, step: str, ok: bool, detail: str = ""):
        self.ok &= ok
        self.report(StepResult(self.test_name, step, ok, detail))

    def expect(self, cmd: str, expected: str, eol: str = "\r\n", min_ms: float = 0.0):
        self._pace()
        reply, ms = self.link.command(cmd, eol)
        ok = reply == expected and ms >= min_ms
        detail = f"got {reply!r} in {ms:.1f} ms"
        if reply is not None:
            self.latencies.append(ms)
        if min_ms:
            detail += f", comm {ms - min_ms:.1f} ms + {min_ms:.0f} ms settle"
        if self.latency_limit_ms is not None:
            ok &= self.latency_ok(ms, is_relay=bool(min_ms))
            detail += f" (limit {self.latency_limit_ms:.0f} ms{' excl. settle' if min_ms and self.exclude_settle else ''})"
        self.record(f"{cmd!r}{'' if eol == chr(13) + chr(10) else ' +' + repr(eol)} -> {expected!r}", ok, detail)

    def latency_ok(self, ms: float, is_relay: bool) -> bool:
        if self.latency_limit_ms is None:
            return True
        if is_relay and self.exclude_settle:
            ms -= SETTLE_MS
        return ms <= self.latency_limit_ms

    def expect_silent(self, step: str, data: bytes):
        self._pace()
        extra = self.link.silence(data)
        self.record(step, extra == b"", f"received {extra!r}" if extra else "no reply")

    def handshake(self, retries: int = 5) -> Optional[str]:
        self._pace()
        reply = None
        for _ in range(retries):
            reply, _ = self.link.command("HeyU")
            m = HELLO_RE.match(reply or "")
            if m:
                self.record("HeyU -> Hello IGNACC v<version> READY", True, f"got {reply!r}")
                return m.group(1)
        self.record("HeyU -> Hello IGNACC v<version> READY", False, f"got {reply!r}")
        return None


def _status(in1: bool, in2: bool) -> str:
    return f"STATUS IN1={'ON' if in1 else 'OFF'} IN2={'ON' if in2 else 'OFF'}"


def t_handshake(ctx: TestContext):
    ctx.handshake()


def t_both_off(ctx: TestContext):
    ctx.expect("IN1_OFF", "IN1_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("IN2_OFF", "IN2_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(False, False))


def t_in1_cycle(ctx: TestContext):
    ctx.expect("IN1_ON", "IN1_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(True, False))
    ctx.expect("IN1_OFF", "IN1_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(False, False))


def t_in2_cycle(ctx: TestContext):
    ctx.expect("IN2_ON", "IN2_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(False, True))
    ctx.expect("IN2_OFF", "IN2_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(False, False))


def t_both_on(ctx: TestContext):
    ctx.expect("IN1_ON", "IN1_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("IN2_ON", "IN2_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(True, True))
    ctx.expect("IN1_OFF", "IN1_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(False, True))
    ctx.expect("IN2_OFF", "IN2_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(False, False))


def t_idempotent(ctx: TestContext):
    ctx.expect("IN1_ON", "IN1_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("IN1_ON", "IN1_ON_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(True, False))
    ctx.expect("IN1_OFF", "IN1_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("IN1_OFF", "IN1_OFF_OK", min_ms=SETTLE_MS)
    ctx.expect("STATUS", _status(False, False))


def t_line_endings(ctx: TestContext):
    ctx.expect("STATUS", _status(False, False), eol="\n")
    ctx.expect("STATUS", _status(False, False), eol="\r\n")
    ctx.expect_silent("CR only ('STATUS\\r') gets no reply", b"STATUS\r")
    ctx.expect("", _status(False, False), eol="\n")  # the LF completes the pending line


def t_ignored_input(ctx: TestContext):
    for junk in ["heyu", "HEYU", "in1_on", "IN1_on", "IN1_ON ", " IN1_ON", "IN3_ON", "garbage", "STATUS?"]:
        ctx.expect_silent(f"ignored {junk!r}", (junk + "\r\n").encode())
    ctx.expect_silent("ignored embedded NUL", b"IN1_ON\x00\r\n")
    ctx.expect_silent("ignored ROM-style text", b"rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)\r\n")
    ctx.expect("STATUS", _status(False, False))


def t_overlong(ctx: TestContext):
    ctx.expect_silent(f"{MAX_LINE_LEN + 1}-char line dropped", b"A" * (MAX_LINE_LEN + 1) + b"\r\n")
    ctx.expect_silent("command at tail of 200-char line dropped", b"X" * 200 + b"IN1_ON\r\n")
    ctx.expect("STATUS", _status(False, False))


def t_split_and_pipelined(ctx: TestContext):
    ctx._pace()
    with ctx.link.lock:
        ctx.link.send_raw(b"IN2_")
        time.sleep(0.05)
        ctx.link._ser.write(b"ON\r\n")
        line = ctx.link.read_line()
    ctx.record("line split across two writes", line == "IN2_ON_OK", f"got {line!r}")

    ctx._pace()
    want = ["IN1_ON_OK", _status(True, True), "IN1_OFF_OK", "IN2_OFF_OK"]
    with ctx.link.lock:
        ctx.link.send_raw(b"IN1_ON\r\nSTATUS\r\nIN1_OFF\r\nIN2_OFF\r\n")
        lines = [ctx.link.read_line() for _ in want]
    ctx.record("4 pipelined commands answered in order", lines == want, f"got {lines!r}")


LATENCY_SAMPLES = 10
LATENCY_GAP = 0.05  # s between samples, so the relays are not buzzed


def t_latency(ctx: TestContext):
    """Round trip (write -> complete reply) per command, LATENCY_SAMPLES times each."""
    groups = [
        ("HeyU", [("HeyU", None)]),
        ("STATUS", [("STATUS", None)]),
        ("IN1_ON / IN1_OFF", [("IN1_ON", "IN1_ON_OK"), ("IN1_OFF", "IN1_OFF_OK")]),
        ("IN2_ON / IN2_OFF", [("IN2_ON", "IN2_ON_OK"), ("IN2_OFF", "IN2_OFF_OK")]),
    ]
    for label, cmds in groups:
        ctx._pace()
        samples, bad = [], []
        for n in range(LATENCY_SAMPLES):
            cmd, expected = cmds[n % len(cmds)]
            reply, ms = ctx.link.command(cmd)
            if reply is None or (expected is not None and reply != expected):
                bad.append(f"{cmd}->{reply!r}")
            else:
                samples.append(ms)
                ctx.latencies.append(ms)
            time.sleep(LATENCY_GAP)
        ok = not bad and bool(samples)
        is_relay = cmds[0][1] is not None
        if samples:
            detail = (f"n={len(samples)} min {min(samples):.1f} / avg {sum(samples) / len(samples):.1f} / "
                      f"max {max(samples):.1f} ms")
            if is_relay:
                detail += f" (incl. {SETTLE_MS} ms settle; comm max {max(samples) - SETTLE_MS:.1f} ms)"
            if ctx.latency_limit_ms is not None:
                ok &= ctx.latency_ok(max(samples), is_relay)
                excl = " excl. settle" if is_relay and ctx.exclude_settle else ""
                detail += f" [limit {ctx.latency_limit_ms:.0f} ms{excl}]"
        else:
            detail = "no valid replies"
        if bad:
            detail += f"; bad replies: {bad[:3]}"
        ctx.record(f"latency {label}", ok, detail)


def t_reset_boot(ctx: TestContext):
    ctx._pace()
    boot = ctx.link.hard_reset()
    ctx.record("board reset via RTS; boot output contains no protocol reply",
               not re.search(rb"_OK|_FAIL|STATUS|Hello", boot), f"{len(boot)} bytes of ROM output")
    ctx.handshake()
    ctx.expect("STATUS", _status(False, False))


@dataclass
class TestCase:
    name: str
    description: str
    func: Callable[[TestContext], None]


TESTS: List[TestCase] = [
    TestCase("Handshake", "HeyU -> Hello IGNACC v<version> READY", t_handshake),
    TestCase("Both OFF", "IN1_OFF, IN2_OFF, STATUS shows both OFF", t_both_off),
    TestCase("IN1 ON/OFF", "Toggle relay 1, check STATUS after each step", t_in1_cycle),
    TestCase("IN2 ON/OFF", "Toggle relay 2, check STATUS after each step", t_in2_cycle),
    TestCase("Both ON, then OFF", "Both relays ON together, then OFF one by one", t_both_on),
    TestCase("Idempotent commands", "Repeat the same command: re-driven, still _OK", t_idempotent),
    TestCase("Line endings", "LF and CRLF accepted; CR alone waits for LF", t_line_endings),
    TestCase("Ignored input", "Wrong case, spaces, unknown text, NUL, ROM text: no reply", t_ignored_input),
    TestCase("Overlong lines", "Lines longer than 32 chars are dropped entirely", t_overlong),
    TestCase("Split + pipelined", "Line split across writes; 4 commands sent at once", t_split_and_pipelined),
    TestCase("Latency", f"Round trip of each command x{LATENCY_SAMPLES}: min/avg/max vs limit", t_latency),
    TestCase("Reset and boot", "Reset the board (RTS); boot text ignored; relays OFF after boot", t_reset_boot),
]


@dataclass
class RunSummary:
    passed_tests: int
    failed_tests: int
    passed_steps: int
    failed_steps: int
    stopped: bool
    latencies: List[float]

    def latency_text(self) -> str:
        if not self.latencies:
            return "no replies timed"
        lat = self.latencies
        return (f"round trip over {len(lat)} replies: min {min(lat):.1f} / "
                f"avg {sum(lat) / len(lat):.1f} / max {max(lat):.1f} ms")


def run_tests(link: Link, indices: List[int], delay: float,
              on_step: Callable[[StepResult], None],
              on_test: Callable[[int, TestCase, Optional[bool]], None],
              stop: threading.Event, latency_limit_ms: Optional[float] = None,
              exclude_settle: bool = False) -> RunSummary:
    """Run TESTS[i] for i in indices. on_test(i, test, None) at start, (i, test, ok) at end.
    If latency_limit_ms is set, every timed reply slower than it fails its step; with
    exclude_settle, relay replies are judged on round trip minus the firmware settle delay.
    Both relays are commanded OFF at the end, even if a test failed or the run was stopped."""
    counts = {"steps_ok": 0, "steps_fail": 0}
    latencies: List[float] = []

    def report(r: StepResult):
        counts["steps_ok" if r.ok else "steps_fail"] += 1
        on_step(r)

    passed = failed = 0
    stopped = False
    first = True
    try:
        for i in indices:
            test = TESTS[i]
            on_test(i, test, None)
            ctx = TestContext(link, delay, report, stop, latency_limit_ms, latencies, exclude_settle)
            ctx.test_name = test.name
            ctx._first_step = first
            first = False
            try:
                test.func(ctx)
            except Stopped:
                stopped = True  # no on_test end call: the test neither passed nor failed
                break
            except (serial.SerialException, OSError) as exc:
                ctx.record("serial error", False, str(exc))
                on_test(i, test, False)
                failed += 1
                break
            on_test(i, test, ctx.ok)
            if ctx.ok:
                passed += 1
            else:
                failed += 1
    finally:
        # Leave the hardware safe.
        try:
            if link.is_open:
                link.command("IN1_OFF")
                link.command("IN2_OFF")
        except (serial.SerialException, OSError):
            pass
    return RunSummary(passed, failed, counts["steps_ok"], counts["steps_fail"], stopped, latencies)


def parse_selection(text: str, count: int) -> List[int]:
    """'1-3,5' -> [0, 1, 2, 4] (1-based input, 0-based output)."""
    out: List[int] = []
    for part in filter(None, (p.strip() for p in text.split(","))):
        if "-" in part:
            a, b = (int(x) for x in part.split("-", 1))
            out.extend(range(min(a, b), max(a, b) + 1))
        else:
            out.append(int(part))
    bad = [n for n in out if not 1 <= n <= count]
    if bad:
        raise ValueError(f"test numbers out of range 1..{count}: {bad}")
    return [n - 1 for n in dict.fromkeys(out)]


def _cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", required=True)
    parser.add_argument("--delay", type=float, default=1.0, help="seconds between steps (default 1)")
    parser.add_argument("--tests", default=f"1-{len(TESTS)}", help="e.g. 1-3,5 (default: all)")
    parser.add_argument("--latency-limit", type=float, default=20.0,
                        help="max round trip per reply in ms (default 20, 0 = no limit)")
    parser.add_argument("--exclude-settle", action="store_true",
                        help=f"judge relay replies on round trip minus the {SETTLE_MS} ms settle delay")
    parser.add_argument("--list", action="store_true", help="list tests and exit")
    args = parser.parse_args()

    if args.list:
        for n, t in enumerate(TESTS, 1):
            print(f"{n:2}. {t.name:22} {t.description}")
        return 0

    link = Link()
    link.open(args.port)
    try:
        summary = run_tests(
            link, parse_selection(args.tests, len(TESTS)), args.delay,
            on_step=lambda r: print(f"   [{'PASS' if r.ok else 'FAIL'}] {r.step}  ({r.detail})"),
            on_test=lambda i, t, ok: print(f"{i + 1:2}. {t.name}" if ok is None
                                           else f"    => {'PASS' if ok else 'FAIL'}"),
            stop=threading.Event(),
            latency_limit_ms=args.latency_limit or None,
            exclude_settle=args.exclude_settle)
    finally:
        link.close()
    print(f"\nTests: {summary.passed_tests} passed, {summary.failed_tests} failed; "
          f"steps: {summary.passed_steps} passed, {summary.failed_steps} failed")
    print(f"Latency: {summary.latency_text()}")
    return 0 if summary.failed_tests == 0 and not summary.stopped else 1


if __name__ == "__main__":
    sys.exit(_cli())
