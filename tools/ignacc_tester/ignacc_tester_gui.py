"""
IGNACC Tester - GUI for testing the IGNACC_Controller relay firmware.

Pick a COM port, connect, then run all tests, a ticked selection or a range,
with a pause between steps so you can watch the relays switch. Every reply is
timed and can be checked against a latency limit.
"""
import ctypes
import queue
import sys
import threading
import time
import tkinter as tk
from datetime import datetime
from tkinter import filedialog, messagebox, ttk
from tkinter.scrolledtext import ScrolledText

import serial

import tester_core as core

APP_VERSION = "1.1.0"
APP_TITLE = "IGNACC Tester"
BAUD_RATES = ["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"]
POLL_MS = 50

COLORS = {"pass": "#1a7f37", "fail": "#cf222e", "info": "#57606a", "test": "#0550ae", "manual": "#8250df"}


class TesterApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(f"{APP_TITLE} v{APP_VERSION}")
        self.minsize(980, 680)

        self.link = core.Link()
        self.events: "queue.Queue[tuple]" = queue.Queue()
        self.stop_event = threading.Event()
        self.running = False
        self.worker: "threading.Thread | None" = None
        self.ports: list = []

        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value=str(core.BAUD))
        self.conn_var = tk.StringVar(value="Disconnected")
        self.mode_var = tk.StringVar(value="all")
        self.range_from = tk.IntVar(value=1)
        self.range_to = tk.IntVar(value=len(core.TESTS))
        self.delay_var = tk.DoubleVar(value=1.0)
        self.check_latency = tk.BooleanVar(value=True)
        self.latency_var = tk.DoubleVar(value=20.0)
        self.exclude_settle = tk.BooleanVar(value=True)
        self.custom_var = tk.StringVar()
        self.current_var = tk.StringVar(value="Idle")
        self.summary_var = tk.StringVar(value="")
        self.test_vars = [tk.BooleanVar(value=True) for _ in core.TESTS]
        self.result_labels: list = []

        self._build_ui()
        self.refresh_ports()
        self._set_connected(False)
        self.protocol("WM_DELETE_WINDOW", self.on_close)
        self.after(POLL_MS, self._poll_events)

    # ------------------------------------------------------------------ UI

    def _build_ui(self):
        style = ttk.Style(self)
        if "vista" in style.theme_names():
            style.theme_use("vista")
        style.configure("Pass.TLabel", foreground=COLORS["pass"])
        style.configure("Fail.TLabel", foreground=COLORS["fail"])
        style.configure("Run.TLabel", foreground=COLORS["test"])
        style.configure("On.TLabel", background="#2da44e", foreground="white", padding=(10, 4))
        style.configure("Off.TLabel", background="#d0d7de", foreground="black", padding=(10, 4))

        self.columnconfigure(0, weight=1)
        self.rowconfigure(2, weight=1)

        # --- Connection --------------------------------------------------
        conn = ttk.LabelFrame(self, text="Connection", padding=8)
        conn.grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 4))
        conn.columnconfigure(1, weight=1)
        ttk.Label(conn, text="COM port:").grid(row=0, column=0, sticky="w")
        self.port_combo = ttk.Combobox(conn, textvariable=self.port_var, state="readonly", width=50)
        self.port_combo.grid(row=0, column=1, sticky="ew", padx=4)
        self.refresh_btn = ttk.Button(conn, text="Refresh", command=self.refresh_ports)
        self.refresh_btn.grid(row=0, column=2, padx=4)
        ttk.Label(conn, text="Baud:").grid(row=0, column=3, padx=(12, 0))
        self.baud_combo = ttk.Combobox(conn, textvariable=self.baud_var, values=BAUD_RATES, width=9)
        self.baud_combo.grid(row=0, column=4, padx=4)
        self.connect_btn = ttk.Button(conn, text="Connect", command=self.toggle_connection, width=12)
        self.connect_btn.grid(row=0, column=5, padx=4)
        self.conn_label = ttk.Label(conn, textvariable=self.conn_var, width=34)
        self.conn_label.grid(row=0, column=6, padx=(8, 0), sticky="w")

        # --- Tests + manual ----------------------------------------------
        mid = ttk.Frame(self)
        mid.grid(row=1, column=0, sticky="ew", padx=8, pady=4)
        mid.columnconfigure(0, weight=1)

        tests = ttk.LabelFrame(mid, text="Tests", padding=8)
        tests.grid(row=0, column=0, sticky="nsew")
        tests.columnconfigure(1, weight=1)

        modes = ttk.Frame(tests)
        modes.grid(row=0, column=0, columnspan=3, sticky="w", pady=(0, 6))
        ttk.Label(modes, text="Run:").pack(side="left")
        for text, value in (("All tests", "all"), ("Selected (ticked)", "selected"), ("Range", "range")):
            ttk.Radiobutton(modes, text=text, value=value, variable=self.mode_var,
                            command=self._mode_changed).pack(side="left", padx=6)
        ttk.Label(modes, text="from").pack(side="left", padx=(4, 2))
        self.from_spin = ttk.Spinbox(modes, from_=1, to=len(core.TESTS), textvariable=self.range_from, width=4)
        self.from_spin.pack(side="left")
        ttk.Label(modes, text="to").pack(side="left", padx=2)
        self.to_spin = ttk.Spinbox(modes, from_=1, to=len(core.TESTS), textvariable=self.range_to, width=4)
        self.to_spin.pack(side="left")
        ttk.Button(modes, text="Tick all", command=lambda: self._tick_all(True)).pack(side="left", padx=(16, 2))
        ttk.Button(modes, text="Tick none", command=lambda: self._tick_all(False)).pack(side="left", padx=2)

        self.test_checks = []
        for i, test in enumerate(core.TESTS):
            cb = ttk.Checkbutton(tests, text=f"{i + 1:2}. {test.name}", variable=self.test_vars[i])
            cb.grid(row=i + 1, column=0, sticky="w")
            self.test_checks.append(cb)
            ttk.Label(tests, text=test.description, foreground=COLORS["info"]).grid(row=i + 1, column=1, sticky="w",
                                                                                     padx=8)
            res = ttk.Label(tests, text="", width=8)
            res.grid(row=i + 1, column=2, sticky="e")
            self.result_labels.append(res)

        opts = ttk.Frame(tests)
        opts.grid(row=len(core.TESTS) + 1, column=0, columnspan=3, sticky="w", pady=(8, 0))
        ttk.Label(opts, text="Delay between steps (s):").pack(side="left")
        ttk.Spinbox(opts, from_=0.0, to=10.0, increment=0.1, textvariable=self.delay_var, width=5).pack(
            side="left", padx=(2, 16))
        ttk.Checkbutton(opts, text="Latency limit (ms):", variable=self.check_latency).pack(side="left")
        ttk.Spinbox(opts, from_=1, to=1000, increment=1, textvariable=self.latency_var, width=5).pack(
            side="left", padx=(2, 16))
        ttk.Checkbutton(opts, text=f"Relay replies: exclude {core.SETTLE_MS} ms settle delay",
                        variable=self.exclude_settle).pack(side="left")

        runbar = ttk.Frame(tests)
        runbar.grid(row=len(core.TESTS) + 2, column=0, columnspan=3, sticky="ew", pady=(8, 0))
        runbar.columnconfigure(2, weight=1)
        self.run_btn = ttk.Button(runbar, text="Run tests", command=self.run_tests, width=14)
        self.run_btn.grid(row=0, column=0)
        self.stop_btn = ttk.Button(runbar, text="Stop", command=self.stop_tests, width=10)
        self.stop_btn.grid(row=0, column=1, padx=6)
        self.progress = ttk.Progressbar(runbar, mode="determinate")
        self.progress.grid(row=0, column=2, sticky="ew", padx=6)
        ttk.Label(runbar, textvariable=self.current_var, width=30).grid(row=0, column=3, sticky="w")

        manual = ttk.LabelFrame(mid, text="Manual control", padding=8)
        manual.grid(row=0, column=1, sticky="nsew", padx=(8, 0))
        self.manual_buttons = []
        cmds = [("HeyU", "HeyU"), ("STATUS", "STATUS"),
                ("IN1 ON", "IN1_ON"), ("IN1 OFF", "IN1_OFF"),
                ("IN2 ON", "IN2_ON"), ("IN2 OFF", "IN2_OFF"),
                ("ALL ON", "ALL_ON"), ("ALL OFF", "ALL_OFF")]
        for n, (label, cmd) in enumerate(cmds):
            b = ttk.Button(manual, text=label, width=10, command=lambda c=cmd: self.send_manual(c))
            b.grid(row=n // 2, column=n % 2, padx=3, pady=3, sticky="ew")
            self.manual_buttons.append(b)
        ind = ttk.Frame(manual)
        ind.grid(row=4, column=0, columnspan=2, pady=(8, 4))
        ttk.Label(ind, text="Relays:").pack(side="left", padx=(0, 6))
        self.ind1 = ttk.Label(ind, text="IN1 ?", style="Off.TLabel")
        self.ind1.pack(side="left", padx=2)
        self.ind2 = ttk.Label(ind, text="IN2 ?", style="Off.TLabel")
        self.ind2.pack(side="left", padx=2)
        ttk.Label(manual, text="Custom line:").grid(row=5, column=0, columnspan=2, sticky="w", pady=(8, 0))
        self.custom_entry = ttk.Entry(manual, textvariable=self.custom_var, width=24)
        self.custom_entry.grid(row=6, column=0, columnspan=2, sticky="ew")
        self.custom_entry.bind("<Return>", lambda _e: self.send_manual(self.custom_var.get()))
        b = ttk.Button(manual, text="Send", command=lambda: self.send_manual(self.custom_var.get()))
        b.grid(row=7, column=0, columnspan=2, sticky="ew", pady=3)
        self.manual_buttons.append(b)

        # --- Log ---------------------------------------------------------
        logf = ttk.LabelFrame(self, text="Log", padding=8)
        logf.grid(row=2, column=0, sticky="nsew", padx=8, pady=(4, 8))
        logf.columnconfigure(0, weight=1)
        logf.rowconfigure(0, weight=1)
        self.log = ScrolledText(logf, height=14, font=("Consolas", 10), state="disabled", wrap="none")
        self.log.grid(row=0, column=0, columnspan=4, sticky="nsew")
        for tag, color in COLORS.items():
            self.log.tag_configure(tag, foreground=color)
        self.log.tag_configure("bold", font=("Consolas", 10, "bold"))
        ttk.Label(logf, textvariable=self.summary_var, font=("Segoe UI", 10, "bold")).grid(
            row=1, column=0, sticky="w", pady=(6, 0))
        ttk.Button(logf, text="Save log...", command=self.save_log).grid(row=1, column=2, pady=(6, 0), padx=4)
        ttk.Button(logf, text="Clear log", command=self.clear_log).grid(row=1, column=3, pady=(6, 0))

        self._mode_changed()

    def _mode_changed(self):
        rng = self.mode_var.get() == "range"
        self.from_spin.configure(state="normal" if rng else "disabled")
        self.to_spin.configure(state="normal" if rng else "disabled")

    def _tick_all(self, value: bool):
        for v in self.test_vars:
            v.set(value)

    # ----------------------------------------------------------- logging

    def write_log(self, text: str, tag: str = "info"):
        stamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.log.configure(state="normal")
        self.log.insert("end", f"{stamp}  ", "info")
        self.log.insert("end", text + "\n", tag)
        self.log.see("end")
        self.log.configure(state="disabled")

    def clear_log(self):
        self.log.configure(state="normal")
        self.log.delete("1.0", "end")
        self.log.configure(state="disabled")
        self.summary_var.set("")

    def save_log(self):
        path = filedialog.asksaveasfilename(
            title="Save log", defaultextension=".txt",
            initialfile=f"ignacc_test_{datetime.now():%Y%m%d_%H%M%S}.txt",
            filetypes=[("Text", "*.txt"), ("All files", "*.*")])
        if path:
            with open(path, "w", encoding="utf-8") as f:
                f.write(self.log.get("1.0", "end"))
            self.write_log(f"Log saved to {path}")

    # -------------------------------------------------------- connection

    def refresh_ports(self):
        self.ports = core.list_ports()
        values = [f"{dev}  -  {desc}" for dev, desc in self.ports]
        self.port_combo["values"] = values
        current = self.port_var.get().split(" ")[0]
        devices = [dev for dev, _ in self.ports]
        if current in devices:
            self.port_combo.current(devices.index(current))
        elif values:
            self.port_combo.current(len(values) - 1)
        else:
            self.port_var.set("")

    def _selected_port(self) -> str:
        return self.port_var.get().split(" ")[0]

    def toggle_connection(self):
        if self.link.is_open:
            self.link.close()
            self._set_connected(False)
            self.write_log("Disconnected")
            return
        port = self._selected_port()
        if not port:
            messagebox.showwarning(APP_TITLE, "No COM port selected.")
            return
        try:
            baud = int(self.baud_var.get())
            self.link.open(port, baud)
        except (ValueError, serial.SerialException) as exc:
            messagebox.showerror(APP_TITLE, f"Cannot open {port}:\n{exc}\n\n"
                                            "Close other programs using the port (PuTTY, Serial Studio, idf.py monitor).")
            return
        self._set_connected(True, f"Connected to {port}")
        self.write_log(f"Connected to {port} at {baud} baud (DTR/RTS off)")
        self.send_manual("HeyU")

    def _set_connected(self, connected: bool, text: str = "Disconnected"):
        self.conn_var.set(text)
        self.conn_label.configure(foreground=COLORS["pass"] if connected else COLORS["fail"])
        self.connect_btn.configure(text="Disconnect" if connected else "Connect")
        self.port_combo.configure(state="disabled" if connected else "readonly")
        self.baud_combo.configure(state="disabled" if connected else "normal")
        self.refresh_btn.configure(state="disabled" if connected else "normal")
        self._update_controls()

    def _update_controls(self):
        connected = self.link.is_open
        idle = not self.running
        self.run_btn.configure(state="normal" if connected and idle else "disabled")
        self.stop_btn.configure(state="normal" if self.running else "disabled")
        self.connect_btn.configure(state="normal" if idle else "disabled")
        for b in self.manual_buttons:
            b.configure(state="normal" if connected and idle else "disabled")
        self.custom_entry.configure(state="normal" if connected and idle else "disabled")

    def _lost_connection(self, exc: Exception):
        self.link.close()
        self._set_connected(False)
        self.write_log(f"Connection lost: {exc}", "fail")

    # ---------------------------------------------------- relay indicators

    def _update_indicators(self, reply: str):
        if reply.startswith("STATUS "):
            fields = dict(part.split("=", 1) for part in reply.split()[1:] if "=" in part)
            for key, lbl in (("IN1", self.ind1), ("IN2", self.ind2)):
                if key in fields:
                    self._set_indicator(lbl, key, fields[key] == "ON")
        elif reply in ("ALL_ON_OK", "ALL_OFF_OK"):
            for key, lbl in (("IN1", self.ind1), ("IN2", self.ind2)):
                self._set_indicator(lbl, key, reply == "ALL_ON_OK")
        else:
            for key, lbl in (("IN1", self.ind1), ("IN2", self.ind2)):
                if reply == f"{key}_ON_OK":
                    self._set_indicator(lbl, key, True)
                elif reply == f"{key}_OFF_OK":
                    self._set_indicator(lbl, key, False)

    @staticmethod
    def _set_indicator(lbl: ttk.Label, key: str, on: bool):
        lbl.configure(text=f"{key} {'ON' if on else 'OFF'}", style="On.TLabel" if on else "Off.TLabel")

    # ------------------------------------------------------- manual sends

    def send_manual(self, line: str):
        line = line.strip("\r\n")
        if not self.link.is_open or self.running:
            return

        def work():
            try:
                reply, ms = self.link.command(line)
                self.events.put(("manual", line, reply, ms))
            except (serial.SerialException, OSError) as exc:
                self.events.put(("lost", exc))

        threading.Thread(target=work, daemon=True).start()

    # ------------------------------------------------------------ test run

    def _selected_indices(self):
        n = len(core.TESTS)
        mode = self.mode_var.get()
        if mode == "all":
            return list(range(n))
        if mode == "selected":
            return [i for i, v in enumerate(self.test_vars) if v.get()]
        try:
            a, b = int(self.range_from.get()), int(self.range_to.get())
        except (tk.TclError, ValueError):
            raise ValueError("Range must be two numbers.")
        if not (1 <= a <= n and 1 <= b <= n):
            raise ValueError(f"Range must be within 1..{n}.")
        lo, hi = min(a, b), max(a, b)
        return list(range(lo - 1, hi))

    def run_tests(self):
        if self.running:
            return
        if not self.link.is_open:
            messagebox.showwarning(APP_TITLE, "Connect to the board first.")
            return
        try:
            indices = self._selected_indices()
            delay = max(0.0, float(self.delay_var.get()))
            limit = float(self.latency_var.get()) if self.check_latency.get() else None
        except (ValueError, tk.TclError) as exc:
            messagebox.showwarning(APP_TITLE, str(exc) or "Invalid number.")
            return
        if not indices:
            messagebox.showwarning(APP_TITLE, "No tests selected.")
            return

        for lbl in self.result_labels:
            lbl.configure(text="", style="TLabel")
        for i in indices:
            self.result_labels[i].configure(text="queued", style="TLabel")
        self.progress.configure(maximum=len(indices), value=0)
        self.summary_var.set("")
        self.stop_event.clear()
        self.running = True
        self._update_controls()

        limit_text = f"{limit:.0f} ms" if limit is not None else "off"
        if limit is not None and self.exclude_settle.get():
            limit_text += f" (relay replies excl. {core.SETTLE_MS} ms settle)"
        self.write_log(f"=== Running {len(indices)} test(s): {', '.join(str(i + 1) for i in indices)} | "
                       f"step delay {delay:.1f} s | latency limit {limit_text}", "bold")
        exclude = self.exclude_settle.get()
        self.run_started = time.perf_counter()

        def work():
            try:
                summary = core.run_tests(
                    self.link, indices, delay,
                    on_step=lambda r: self.events.put(("step", r)),
                    on_test=lambda i, t, ok: self.events.put(("test", i, t, ok)),
                    stop=self.stop_event, latency_limit_ms=limit, exclude_settle=exclude)
                self.events.put(("done", summary))
            except Exception as exc:  # keep the GUI alive whatever happens
                self.events.put(("crash", exc))

        self.worker = threading.Thread(target=work, daemon=True)
        self.worker.start()

    def stop_tests(self):
        self.stop_event.set()
        self.current_var.set("Stopping...")

    # --------------------------------------------------------- event pump

    def _poll_events(self):
        try:
            while True:
                self._handle_event(self.events.get_nowait())
        except queue.Empty:
            pass
        self.after(POLL_MS, self._poll_events)

    def _handle_event(self, ev):
        kind = ev[0]
        if kind == "manual":
            _, line, reply, ms = ev
            if reply is None:
                self.write_log(f"> {line!r}  ->  (no reply within {core.REPLY_TIMEOUT:.0f} s)", "manual")
            else:
                self.write_log(f"> {line!r}  ->  {reply!r}  [{ms:.1f} ms]", "manual")
                self._update_indicators(reply)
                m = core.HELLO_RE.match(reply)
                if m:
                    self.conn_var.set(f"Connected to {self._selected_port()} - FW v{m.group(1)}")
        elif kind == "lost":
            self._lost_connection(ev[1])
        elif kind == "test":
            _, i, test, ok = ev
            lbl = self.result_labels[i]
            if ok is None:
                self.current_var.set(f"Running {i + 1}. {test.name}")
                lbl.configure(text="running", style="Run.TLabel")
                self.write_log(f"--- {i + 1}. {test.name}: {test.description}", "test")
            else:
                lbl.configure(text="PASS" if ok else "FAIL", style="Pass.TLabel" if ok else "Fail.TLabel")
                self.write_log(f"    => {i + 1}. {test.name}: {'PASS' if ok else 'FAIL'}", "pass" if ok else "fail")
                self.progress.step(1)
        elif kind == "step":
            r: core.StepResult = ev[1]
            self.write_log(f"    [{'PASS' if r.ok else 'FAIL'}] {r.step}   {r.detail}", "pass" if r.ok else "fail")
            if "got '" in r.detail:
                reply = r.detail.split("got '", 1)[1].split("'", 1)[0]
                self._update_indicators(reply)
        elif kind == "done":
            s: core.RunSummary = ev[1]
            self.running = False
            elapsed = time.perf_counter() - self.run_started
            state = "STOPPED" if s.stopped else ("ALL PASSED" if s.failed_tests == 0 else "FAILURES")
            text = (f"{state}: {s.passed_tests} test(s) passed, {s.failed_tests} failed "
                    f"({s.passed_steps}/{s.passed_steps + s.failed_steps} steps) in {elapsed:.1f} s  |  "
                    f"Latency {s.latency_text()}")
            self.summary_var.set(text)
            self.write_log("=== " + text, "pass" if state == "ALL PASSED" else "fail")
            self.write_log("Relays commanded OFF after the run.")
            self.ind1.configure(text="IN1 OFF", style="Off.TLabel")
            self.ind2.configure(text="IN2 OFF", style="Off.TLabel")
            self._finish_run()
        elif kind == "crash":
            self.running = False
            self.write_log(f"Test run aborted: {ev[1]!r}", "fail")
            self.summary_var.set(f"ABORTED: {ev[1]}")
            self._finish_run()

    def _finish_run(self):
        self.current_var.set("Idle")
        for lbl in self.result_labels:
            if lbl.cget("text") == "running":
                lbl.configure(text="stopped", style="Fail.TLabel")
            elif lbl.cget("text") == "queued":
                lbl.configure(text="skipped", style="TLabel")
        if not self.link.is_open:
            self._set_connected(False)
        self._update_controls()

    def on_close(self):
        if self.running:
            if not messagebox.askyesno(APP_TITLE, "Tests are running. Stop them and quit?"):
                return
            self.stop_event.set()
            if self.worker is not None:
                self.worker.join(timeout=5)  # its cleanup switches both relays OFF
        try:
            if self.link.is_open:
                core.all_off(self.link)
        except (serial.SerialException, OSError):
            pass
        self.link.close()
        self.destroy()


def main():
    if sys.platform == "win32":
        try:
            ctypes.windll.shcore.SetProcessDpiAwareness(1)  # crisp text on high-DPI screens
        except (AttributeError, OSError):
            pass
    TesterApp().mainloop()


if __name__ == "__main__":
    main()
