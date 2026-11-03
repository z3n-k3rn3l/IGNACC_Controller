#!/usr/bin/env python3
"""
Generate the IGNACC_Controller architecture and UML diagrams as SVG.

    python docs/diagrams/generate_diagrams.py

Writes every diagram next to this script (docs/diagrams/*.svg).
Standard library only. Each diagram is one function below; edit the
coordinates or text there and re-run.
"""
import html
import pathlib

OUT = pathlib.Path(__file__).resolve().parent

FONT = "'Segoe UI', 'Helvetica Neue', Arial, sans-serif"
MONO = "Consolas, 'Cascadia Mono', Menlo, monospace"

INK = "#1f2937"
MUTED = "#4b5563"
LINE = "#374151"

# (fill, stroke) per element kind
KIND = {
    "app": ("#dbeafe", "#2563eb"),      # application / state machine
    "svc": ("#dcfce7", "#16a34a"),      # services, pure logic
    "ifc": ("#fef9c3", "#a16207"),      # interfaces, structs
    "drv": ("#ffedd5", "#c2410c"),      # drivers
    "hal": ("#f3e8ff", "#7e22ce"),      # platform: ESP-IDF, FreeRTOS, HAL
    "hw": ("#e5e7eb", "#4b5563"),       # hardware
    "pc": ("#e0f2fe", "#0369a1"),       # PC side
    "note": ("#fffbeb", "#b45309"),     # notes
    "ok": ("#dcfce7", "#15803d"),       # success / tests
    "bad": ("#fee2e2", "#b91c1c"),      # failure / safe state
    "neutral": ("#f9fafb", "#6b7280"),  # neutral / unused
    "cfg": ("#ecfeff", "#0e7490"),      # configuration / composition
}

ARROW_COLORS = {"ink": LINE, "red": "#b91c1c", "green": "#15803d", "blue": "#2563eb"}


def esc(s):
    return html.escape(str(s), quote=True)


def text_width(s, size, mono=False):
    return len(str(s)) * size * (0.6 if mono else 0.53)


class Svg:
    def __init__(self, name, w, h, title, subtitle=""):
        self.name, self.w, self.h = name, w, h
        self.title, self.subtitle = title, subtitle
        self.p = []

    # ---- primitives -------------------------------------------------------
    def raw(self, s):
        self.p.append(s)

    def rect(self, x, y, w, h, fill="#fff", stroke=LINE, rx=8, sw=1.5, dash=None, opacity=None):
        d = f' stroke-dasharray="{dash}"' if dash else ""
        o = f' fill-opacity="{opacity}"' if opacity is not None else ""
        self.raw(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}"{o} '
                 f'stroke="{stroke}" stroke-width="{sw}"{d}/>')

    def text(self, x, y, s, size=13, weight="normal", anchor="start", color=INK, mono=False, italic=False):
        fam = MONO if mono else FONT
        st = ' font-style="italic"' if italic else ""
        self.raw(f'<text x="{x}" y="{y}" font-family="{fam}" font-size="{size}" font-weight="{weight}" '
                 f'text-anchor="{anchor}" fill="{color}"{st} xml:space="preserve">{esc(s)}</text>')

    def lines(self, x, y, items, size=13, lh=None, **kw):
        lh = lh or size * 1.4
        for i, s in enumerate(items):
            self.text(x, y + i * lh, s, size=size, **kw)

    def label(self, x, y, s, size=11.5, color=MUTED, anchor="middle", mono=False, weight="normal", bg="#ffffff"):
        """Text with a background patch so it stays readable on top of lines."""
        w = text_width(s, size, mono) + 8
        if anchor == "middle":
            bx = x - w / 2
        elif anchor == "start":
            bx = x - 4
        else:
            bx = x - w + 4
        if bg:
            self.raw(f'<rect x="{bx:.1f}" y="{y - size:.1f}" width="{w:.1f}" height="{size + 5:.1f}" rx="3" '
                     f'fill="{bg}" fill-opacity="0.92"/>')
        self.text(x, y, s, size=size, anchor=anchor, color=color, mono=mono, weight=weight)

    def line(self, pts, color="ink", sw=1.6, dash=None, head="filled", tail=None):
        c = ARROW_COLORS.get(color, color)
        cname = color if color in ARROW_COLORS else "ink"
        d = f' stroke-dasharray="{dash}"' if dash else ""
        me = f' marker-end="url(#{head}-{cname})"' if head else ""
        ms = f' marker-start="url(#{tail}-{cname})"' if tail else ""
        pts_s = " ".join(f"{x},{y}" for x, y in pts)
        self.raw(f'<polyline points="{pts_s}" fill="none" stroke="{c}" stroke-width="{sw}" '
                 f'stroke-linejoin="round"{d}{me}{ms}/>')

    def path(self, d, color="ink", sw=1.6, dash=None, head="filled"):
        c = ARROW_COLORS.get(color, color)
        cname = color if color in ARROW_COLORS else "ink"
        da = f' stroke-dasharray="{dash}"' if dash else ""
        me = f' marker-end="url(#{head}-{cname})"' if head else ""
        self.raw(f'<path d="{d}" fill="none" stroke="{c}" stroke-width="{sw}"{da}{me}/>')

    # ---- shapes -------------------------------------------------------------
    def box(self, x, y, w, h, title, body=(), kind="app", stereo=None, title_size=14, body_size=12,
            mono_body=False, rx=8, dash=None, align="left", badge=None):
        fill, stroke = KIND[kind]
        self.rect(x, y, w, h, fill=fill, stroke=stroke, rx=rx, dash=dash)
        cy = y + 20
        if stereo:
            self.text(x + w / 2, cy, stereo, size=11, anchor="middle", color=MUTED, italic=True)
            cy += 17
        for t in str(title).split("\n"):
            self.text(x + w / 2, cy, t, size=title_size, weight="bold", anchor="middle")
            cy += title_size + 4
        if body:
            sep = cy - title_size * 0.55 + 2
            self.raw(f'<line x1="{x}" y1="{sep}" x2="{x + w}" y2="{sep}" stroke="{stroke}" stroke-width="1"/>')
            by = sep + body_size + 7
            lh = body_size * 1.45
            for i, s in enumerate(body):
                if align == "center":
                    self.text(x + w / 2, by + i * lh, s, size=body_size, anchor="middle", mono=mono_body)
                else:
                    self.text(x + 10, by + i * lh, s, size=body_size, mono=mono_body)
        if badge:
            bw = text_width(badge, 10) + 12
            self.rect(x + w - bw - 6, y - 9, bw, 18, fill="#15803d", stroke="#15803d", rx=9, sw=1)
            self.text(x + w - bw / 2 - 6, y + 4, badge, size=10, anchor="middle", color="#ffffff", weight="bold")
        return {"x": x, "y": y, "w": w, "h": h, "cx": x + w / 2, "cy": y + h / 2, "r": x + w, "b": y + h}

    def action(self, cx, cy, w, h, lines_, kind="app", size=12.5, rx=18):
        fill, stroke = KIND[kind]
        self.rect(cx - w / 2, cy - h / 2, w, h, fill=fill, stroke=stroke, rx=rx)
        n = len(lines_)
        lh = size * 1.35
        y0 = cy - (n - 1) * lh / 2 + size * 0.35
        for i, s in enumerate(lines_):
            self.text(cx, y0 + i * lh, s, size=size, anchor="middle")

    def diamond(self, cx, cy, w, h, lines_, size=12):
        fill, stroke = KIND["ifc"]
        pts = f"{cx},{cy - h / 2} {cx + w / 2},{cy} {cx},{cy + h / 2} {cx - w / 2},{cy}"
        self.raw(f'<polygon points="{pts}" fill="{fill}" stroke="{stroke}" stroke-width="1.5"/>')
        n = len(lines_)
        lh = size * 1.3
        y0 = cy - (n - 1) * lh / 2 + size * 0.35
        for i, s in enumerate(lines_):
            self.text(cx, y0 + i * lh, s, size=size, anchor="middle")

    def start(self, cx, cy, r=10):
        self.raw(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{INK}"/>')

    def final(self, cx, cy, r=12):
        self.raw(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="#fff" stroke="{INK}" stroke-width="2"/>')
        self.raw(f'<circle cx="{cx}" cy="{cy}" r="{r - 5}" fill="{INK}"/>')

    def flow_final(self, cx, cy, r=11):
        self.raw(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="#fff" stroke="{INK}" stroke-width="2"/>')
        k = r * 0.62
        self.raw(f'<path d="M{cx - k},{cy - k} L{cx + k},{cy + k} M{cx - k},{cy + k} L{cx + k},{cy - k}" '
                 f'stroke="{INK}" stroke-width="2"/>')

    def connector(self, cx, cy, s, r=13):
        self.raw(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="#fff" stroke="{INK}" stroke-width="1.6"/>')
        self.text(cx, cy + 4.5, s, size=12, weight="bold", anchor="middle")

    def ellipse(self, cx, cy, rx, ry, s, kind="app", size=12.5):
        fill, stroke = KIND[kind]
        self.raw(f'<ellipse cx="{cx}" cy="{cy}" rx="{rx}" ry="{ry}" fill="{fill}" stroke="{stroke}" stroke-width="1.5"/>')
        parts = s.split("\n")
        lh = size * 1.3
        y0 = cy - (len(parts) - 1) * lh / 2 + size * 0.35
        for i, t in enumerate(parts):
            self.text(cx, y0 + i * lh, t, size=size, anchor="middle")

    def actor(self, x, y, name):
        c = INK
        self.raw(f'<circle cx="{x}" cy="{y}" r="11" fill="#fff" stroke="{c}" stroke-width="1.8"/>')
        self.raw(f'<path d="M{x},{y + 11} L{x},{y + 44} M{x - 20},{y + 22} L{x + 20},{y + 22} '
                 f'M{x},{y + 44} L{x - 16},{y + 68} M{x},{y + 44} L{x + 16},{y + 68}" '
                 f'stroke="{c}" stroke-width="1.8" fill="none"/>')
        for i, t in enumerate(name.split("\n")):
            self.text(x, y + 88 + i * 16, t, size=13, weight="bold", anchor="middle")

    def caption(self, y, items, size=12.5):
        for i, s in enumerate(items):
            self.text(30, y + i * size * 1.5, s, size=size, color=MUTED)

    def legend(self, x, y, items):
        """items: list of (sample, text) where sample is 'call', 'reply', 'uses', 'realize', 'agg' or a kind."""
        for i, (sample, t) in enumerate(items):
            yy = y + i * 22
            if sample in KIND:
                f, s = KIND[sample]
                self.rect(x, yy - 11, 28, 15, fill=f, stroke=s, rx=3)
            elif sample == "call":
                self.line([(x, yy - 4), (x + 28, yy - 4)])
            elif sample == "reply":
                self.line([(x, yy - 4), (x + 28, yy - 4)], dash="5 4", head="open")
            elif sample == "uses":
                self.line([(x, yy - 4), (x + 28, yy - 4)], dash="5 4", head="open")
            elif sample == "realize":
                self.line([(x, yy - 4), (x + 28, yy - 4)], dash="5 4", head="hollow")
            elif sample == "agg":
                self.line([(x, yy - 4), (x + 28, yy - 4)], head="open", tail="diamond")
            self.text(x + 36, yy, t, size=12, color=MUTED)

    # ---- output -------------------------------------------------------------
    def save(self):
        markers = []
        for cname, c in ARROW_COLORS.items():
            markers.append(
                f'<marker id="filled-{cname}" viewBox="0 0 10 10" refX="9.6" refY="5" markerWidth="10" '
                f'markerHeight="10" markerUnits="userSpaceOnUse" orient="auto-start-reverse">'
                f'<path d="M0,0 L10,5 L0,10 z" fill="{c}"/></marker>')
            markers.append(
                f'<marker id="open-{cname}" viewBox="0 0 10 10" refX="9.6" refY="5" markerWidth="11" '
                f'markerHeight="11" markerUnits="userSpaceOnUse" orient="auto-start-reverse">'
                f'<path d="M0,0.5 L10,5 L0,9.5" fill="none" stroke="{c}" stroke-width="1.6"/></marker>')
            markers.append(
                f'<marker id="hollow-{cname}" viewBox="0 0 12 12" refX="11.4" refY="6" markerWidth="15" '
                f'markerHeight="15" markerUnits="userSpaceOnUse" orient="auto-start-reverse">'
                f'<path d="M0.5,0.5 L11.5,6 L0.5,11.5 z" fill="#fff" stroke="{c}" stroke-width="1.3"/></marker>')
            markers.append(
                f'<marker id="diamond-{cname}" viewBox="0 0 16 10" refX="0.6" refY="5" markerWidth="18" '
                f'markerHeight="11" markerUnits="userSpaceOnUse" orient="auto-start-reverse">'
                f'<path d="M0.5,5 L8,0.6 L15.5,5 L8,9.4 z" fill="#fff" stroke="{c}" stroke-width="1.3"/></marker>')
        head = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
                f'viewBox="0 0 {self.w} {self.h}" role="img" aria-labelledby="t d">\n'
                f'<title id="t">{esc(self.title)}</title>\n<desc id="d">{esc(self.subtitle)}</desc>\n'
                f'<defs>{"".join(markers)}</defs>\n'
                f'<rect x="0" y="0" width="{self.w}" height="{self.h}" fill="#ffffff"/>\n')
        title = (f'<text x="30" y="36" font-family="{FONT}" font-size="21" font-weight="bold" fill="{INK}">'
                 f'{esc(self.title)}</text>\n')
        if self.subtitle:
            title += (f'<text x="30" y="58" font-family="{FONT}" font-size="13" fill="{MUTED}">'
                      f'{esc(self.subtitle)}</text>\n')
        body = "\n".join(self.p)
        (OUT / f"{self.name}.svg").write_text(head + title + body + "\n</svg>\n", encoding="utf-8")
        print(f"  {self.name}.svg  ({self.w}x{self.h})")


# ===========================================================================
# Sequence diagram helper
# ===========================================================================

def sequence(svg, parts, items, x0=100, gap=140, top=84, row=32, head_h=48, bottom_pad=26):
    """parts: [(key, 'label\\nline2', kind)]; items: see diagrams below.

    item forms:
      ('call'|'reply'|'async', a, b, text)      message a -> b
      ('self', a, text)                         self call
      ('note', [a] or [a, b], ['line', ...])    note over participants
      ('frame', kind, label, [a, b])            open loop/alt/opt frame spanning a..b
      ('else', label)                           alt divider
      ('end',)                                  close frame
      ('gap', px)
    """
    xs = {k: x0 + i * gap for i, (k, _, _) in enumerate(parts)}
    bw = gap - 18
    start_index = len(svg.p)
    y = top + head_h + 26
    frames = []
    for it in items:
        kind = it[0]
        if kind in ("call", "reply", "async"):
            _, a, b, t = it
            y += row
            xa, xb = xs[a], xs[b]
            if kind == "call":
                svg.line([(xa, y), (xb, y)], head="filled")
            elif kind == "async":
                svg.line([(xa, y), (xb, y)], head="open")
            else:
                svg.line([(xa, y), (xb, y)], dash="6 4", head="open")
            svg.label((xa + xb) / 2, y - 6, t, size=11.5, color=INK, mono=True)
        elif kind == "self":
            _, a, t = it
            y += row
            x = xs[a]
            svg.line([(x, y - 8), (x + 34, y - 8), (x + 34, y + 8), (x + 3, y + 8)], head="filled")
            svg.label(x + 40, y + 4, t, size=11.5, color=INK, anchor="start", mono=True)
            y += 10
        elif kind == "note":
            _, who, lines_ = it
            y += 14
            xl = xs[who[0]] - bw / 2
            xr = xs[who[-1]] + bw / 2
            h = 12 + 16 * len(lines_)
            f, s = KIND["note"]
            svg.rect(xl, y, xr - xl, h, fill=f, stroke=s, rx=4)
            svg.lines(xl + 8, y + 17, lines_, size=11.5, lh=16, color=INK)
            y += h
        elif kind == "frame":
            _, fk, lab, span = it
            y += 22
            frames.append({"kind": fk, "label": lab, "span": span, "y0": y, "elses": [], "depth": len(frames)})
            y += 18
        elif kind == "else":
            y += 20
            frames[-1]["elses"].append((y, it[1]))
            y += 10
        elif kind == "end":
            y += 16
            fr = frames.pop()
            d = fr["depth"] * 9
            xl = xs[fr["span"][0]] - bw / 2 - 12 + d
            xr = xs[fr["span"][-1]] + bw / 2 + 12 - d
            svg.rect(xl, fr["y0"], xr - xl, y - fr["y0"], fill="none", stroke="#6b7280", rx=2, sw=1.3)
            tag = f'{fr["kind"]}'
            tw = text_width(tag, 12) + 18
            svg.raw(f'<path d="M{xl},{fr["y0"]} h{tw} v12 l-7,8 h{-(tw - 7)} z" fill="#f3f4f6" '
                    f'stroke="#6b7280" stroke-width="1.2"/>')
            svg.text(xl + 7, fr["y0"] + 15, tag, size=12, weight="bold")
            svg.text(xl + tw + 8, fr["y0"] + 15, f'[{fr["label"]}]', size=11.5, color=MUTED, italic=True)
            for ey, el in fr["elses"]:
                svg.raw(f'<line x1="{xl}" y1="{ey}" x2="{xr}" y2="{ey}" stroke="#6b7280" stroke-dasharray="6 4"/>')
                svg.text(xl + 8, ey + 15, f"[{el}]", size=11.5, color=MUTED, italic=True)
        elif kind == "gap":
            y += it[1]
    end_y = y + bottom_pad
    # Participants and lifelines go underneath everything else.
    under = []
    for k, label, kd in parts:
        x = xs[k]
        f, s = KIND[kd]
        under.append(f'<line x1="{x}" y1="{top + head_h}" x2="{x}" y2="{end_y}" stroke="#9ca3af" '
                     f'stroke-width="1.3" stroke-dasharray="5 5"/>')
        under.append(f'<rect x="{x - bw / 2}" y="{top}" width="{bw}" height="{head_h}" rx="6" fill="{f}" '
                     f'stroke="{s}" stroke-width="1.5"/>')
        lines_ = label.split("\n")
        y0 = top + head_h / 2 - (len(lines_) - 1) * 8 + 4.5
        for i, t in enumerate(lines_):
            under.append(f'<text x="{x}" y="{y0 + i * 16}" font-family="{FONT}" font-size="12.5" '
                         f'font-weight="bold" text-anchor="middle" fill="{INK}">{esc(t)}</text>')
    svg.p[start_index:start_index] = under
    return end_y


# ===========================================================================
# 1. System context
# ===========================================================================

def d_system_context():
    s = Svg("system-context", 1300, 660, "System context: what is connected to what",
            "The PC tells the ESP32 which relay to switch; the relays switch the ECU's ACC and IGN lines.")
    f, st = KIND["pc"]
    s.rect(30, 80, 330, 430, fill="#f0f9ff", stroke=st, rx=12, dash="8 5")
    s.text(48, 104, "Test PC (Windows 11)", size=14, weight="bold", color=st)
    canoe = s.box(60, 122, 270, 66, "CANoe + CAPL script", ["automated test steps"], kind="pc", align="center")
    tester = s.box(60, 210, 270, 66, "IGNACC Tester (.exe)", ["manual control and self-test"], kind="pc", align="center")
    s.text(195, 304, "only one program can use the COM port at a time", size=10.5, anchor="middle",
           color=MUTED, italic=True)
    drv = s.box(60, 322, 270, 66, "USB-serial driver (CP210x)", ["COM15 · 115200 baud · 8N1"], kind="pc", align="center")
    vn = s.box(60, 420, 270, 66, "Vector VN interface", ["CAN hardware for CANoe"], kind="pc", align="center")
    s.line([(195, 276), (195, 322)])
    s.line([(60, 150), (48, 150), (48, 355), (60, 355)])
    s.line([(60, 172), (38, 172), (38, 452), (60, 452)])

    esp = s.box(440, 230, 230, 160, "ESP32 board", ["IGNACC Controller firmware", "• reads text commands",
                                                    "• switches the relays", "• replies OK / FAIL / UNK_NACK"],
                kind="app")
    s.line([(330, 355), (440, 355)], head="filled", tail="filled")
    s.label(386, 412, "USB cable", size=11.5, color=INK, weight="bold")
    s.label(386, 428, "IN1_ON ↔ IN1_ON_OK", size=11, mono=True)

    sh = s.box(730, 260, 150, 100, "Level shifter", ["3.3 V → 5 V logic"], kind="drv", align="center")
    s.line([(670, 300), (730, 300)])
    s.label(700, 290, "GPIO26/27", size=10.5)
    rel = s.box(940, 235, 150, 150, "Relay module", ["IN1 → ACC relay", "IN2 → IGN relay", "5 V coils"],
                kind="hw", align="center")
    s.line([(880, 300), (940, 300)])
    s.label(910, 290, "IN1/IN2", size=10.5)
    ecu = s.box(1150, 205, 120, 210, "ECU under test", ["ACC input", "IGN input", "", "CAN"], kind="neutral",
                align="center")
    s.line([(1090, 300), (1150, 300)])
    s.label(1120, 290, "12 V", size=10.5)
    s.label(1120, 324, "ACC / IGN", size=10.5)
    sup = s.box(940, 450, 150, 56, "12 V bench supply", [], kind="hw")
    s.line([(1015, 450), (1015, 385)])

    s.line([(195, 486), (195, 568), (1210, 568), (1210, 415)], color="blue", sw=2.4)
    s.label(700, 572, "CAN bus · CAPL sends the ACC / IGN status signals", size=12, color="#2563eb", weight="bold")

    s.caption(608, ["In one sentence: the PC sends a short text such as IN1_ON over USB; the ESP32 switches relay 1, which",
                    "turns the ECU's ACC line on, and answers IN1_ON_OK. CAPL then sends the matching ACC signal on CAN."])
    s.save()


# ===========================================================================
# 2. Use cases
# ===========================================================================

def d_use_cases():
    s = Svg("use-cases", 1120, 690, "Use cases: who uses the system, and for what",
            "Each figure is a user; each oval is one thing they can do with the system.")
    s.rect(300, 80, 530, 560, fill="#f8fafc", stroke="#64748b", rx=14)
    s.text(320, 106, "IGNACC Controller (firmware + tester)", size=14, weight="bold", color="#334155")
    ucs = [
        ("Check the board is alive\n(HeyU)", 150),
        ("Switch one relay\n(IN<n>_ON / IN<n>_OFF)", 225),
        ("Switch all relays at once\n(ALL_ON / ALL_OFF)", 300),
        ("Read every relay's state\n(STATUS)", 375),
        ("Be told a command is invalid\n(UNK_NACK)", 450),
        ("Run the automated self-test\n(IGNACC Tester)", 525),
        ("Configure channels and pins,\nbuild and flash the firmware", 600),
    ]
    cx = 565
    for t, y in ucs:
        s.ellipse(cx, y, 175, 31, t, kind="cfg" if "Configure" in t else ("ok" if "self-test" in t else "app"))
    s.actor(150, 150, "Test engineer")
    f, st = KIND["pc"]
    s.rect(55, 360, 190, 76, fill=f, stroke=st, rx=8)
    s.text(150, 382, "«system actor»", size=11, anchor="middle", color=MUTED, italic=True)
    s.text(150, 402, "CANoe / CAPL", size=14, weight="bold", anchor="middle")
    s.text(150, 420, "test script", size=12, anchor="middle", color=MUTED)
    s.actor(990, 470, "Firmware\ndeveloper")

    for y in (150, 225, 300, 375, 525):
        s.line([(172, 172), (390, y)], head=None, sw=1.3)
    for y in (150, 225, 300, 375):
        s.line([(245, 398), (390, y)], head=None, sw=1.3)
    for y in (525, 600):
        s.line([(968, 492), (740, y)], head=None, sw=1.3)
    # UNK_NACK extends the command use cases
    s.line([(740, 450), (790, 450), (790, 228), (742, 228)], dash="6 4", head="open")
    s.label(790, 340, "«extend»", size=11.5, color=MUTED)
    s.caption(660, ["UNK_NACK «extends» switching: it is what happens when the command text is wrong, "
                    "or names a relay the board doesn't have."])
    s.save()


# ===========================================================================
# 3. Layered architecture (AUTOSAR-style)
# ===========================================================================

def d_layers():
    s = Svg("layered-architecture", 1270, 860, "Layered architecture (AUTOSAR-style view)",
            "Each layer only uses the layers below it, through interfaces. Green tag = pure C, tested on the PC.")
    rows = [
        ("Application", "AUTOSAR: SWC / application", "app", 80),
        ("Services", "AUTOSAR: services", "svc", 198),
        ("Interfaces", "AUTOSAR: RTE / ECU abstraction", "ifc", 316),
        ("Drivers", "AUTOSAR: complex / IO drivers", "drv", 434),
        ("Platform", "AUTOSAR: MCAL + OS (ESP-IDF)", "hal", 552),
        ("Hardware", "ESP32-WROOM-32 devkit", "hw", 670),
    ]
    for name, sub, kind, y in rows:
        f, st = KIND[kind]
        s.rect(30, y, 880, 104, fill=f, stroke=st, rx=10, opacity=0.45)
        s.text(46, y + 46, name, size=16, weight="bold", color=st)
        s.text(46, y + 66, sub, size=11, color=MUTED)
    tag = "host-tested"
    s.box(240, 96, 330, 72, "app_fsm", ["state machine · sole owner of the relays"], kind="app", align="center", badge=tag)
    s.box(590, 96, 300, 72, "app_fsm_task", ["FreeRTOS task: queue → state machine"], kind="app", align="center")
    s.box(240, 214, 210, 72, "protocol", ["parse / format text"], kind="svc", align="center", badge=tag)
    s.box(465, 214, 210, 72, "relay_bank", ["N relays · ALL_ON / ALL_OFF"], kind="svc", align="center", badge=tag)
    s.box(690, 214, 200, 72, "line_assembler", ["bytes → lines"], kind="svc", align="center", badge=tag)
    s.box(240, 332, 210, 72, "relay.h", ["relay_t"], kind="ifc", stereo="«interface»", align="center")
    s.box(465, 332, 210, 72, "link_if.h", ["link_tx_t · link_line_t"], kind="ifc", stereo="«interface»", align="center")
    s.box(690, 332, 200, 72, "pin_hal.h", ["pin_hal_t"], kind="ifc", stereo="«interface»", align="center")
    s.box(240, 450, 330, 72, "relay_gpio", ["one relay per GPIO · verified by read-back"], kind="drv", align="center",
          badge=tag)
    s.box(590, 450, 300, 72, "uart_link", ["UART receive task + transmit"], kind="drv", align="center")
    s.box(240, 568, 150, 72, "pin_hal_esp", ["GPIO adapter"], kind="hal", align="center")
    s.box(400, 568, 160, 72, "ESP-IDF GPIO", ["driver"], kind="hal", align="center")
    s.box(570, 568, 160, 72, "ESP-IDF UART", ["driver (ISR + queue)"], kind="hal", align="center")
    s.box(740, 568, 150, 72, "FreeRTOS", ["tasks · queue · delay"], kind="hal", align="center")
    s.box(240, 686, 320, 72, "GPIO26 / GPIO27 …", ["one pin per relay channel"], kind="hw", align="center")
    s.box(570, 686, 320, 72, "UART0 → CP210x USB bridge", ["the protocol link to the PC"], kind="hw", align="center")

    s.box(940, 80, 300, 346, "main", ["composition root:", "", "• reads Kconfig", "• checks pins at compile time",
                                      "• creates every object", "• injects the dependencies",
                                      "  (relays, link, delay)", "• starts the tasks", "",
                                      "no application logic of its own"],
          kind="cfg", stereo="«composition root»", body_size=12.5)
    s.box(940, 446, 300, 312, "Kconfig / sdkconfig", ["build-time settings:", "", "• relay channels: 1–14",
                                                      "• one GPIO per channel", "• active level, settle ms",
                                                      "• UART port, pins, baud", "• queue depth, task sizes", "",
                                                      "only main reads them"],
          kind="cfg", stereo="«configuration»", body_size=12.5)
    s.caption(790, ["Rule: upper layers never call ESP-IDF directly. Hardware is reached through relay_t, pin_hal_t "
                    "and link_tx_t, so", "the green-tagged modules compile and run on a PC with mocks (75 unit tests), "
                    "and new hardware plugs in below an interface."])
    s.save()


# ===========================================================================
# 4. Component dependencies
# ===========================================================================

def d_components():
    s = Svg("component-dependencies", 1200, 880, "Component dependencies",
            "Who uses whom. Arrows point from the user to what it depends on.")
    s.box(490, 80, 200, 62, "main", ["composition root"], kind="cfg", align="center")
    s.box(110, 200, 200, 62, "app_fsm_task", ["FreeRTOS runner"], kind="app", align="center")
    s.box(490, 200, 200, 62, "app_fsm", ["state machine"], kind="app", align="center")
    s.box(870, 200, 200, 62, "uart_link", ["UART line transport"], kind="drv", align="center")
    s.box(110, 340, 200, 62, "protocol", ["parse / format"], kind="svc", align="center")
    s.box(380, 340, 200, 62, "relay_bank", ["group of relays"], kind="svc", align="center")
    s.box(650, 340, 200, 70, "link_if", ["link_tx_t, link_line_t"], kind="ifc", stereo="«interface»", align="center")
    s.box(920, 340, 200, 62, "line_assembler", ["bytes → lines"], kind="svc", align="center")
    s.box(380, 470, 200, 70, "relay", ["relay_t"], kind="ifc", stereo="«interface»", align="center")
    s.box(380, 610, 200, 62, "relay_gpio", ["GPIO relay"], kind="drv", align="center")
    s.box(650, 606, 200, 70, "pin_hal", ["pin_hal_t"], kind="ifc", stereo="«interface»", align="center")
    s.box(650, 740, 200, 62, "pin_hal_esp", ["ESP-IDF GPIO adapter"], kind="hal", align="center")
    s.box(920, 740, 230, 62, "ESP-IDF", ["UART · GPIO · FreeRTOS"], kind="hal", align="center")

    u = dict(dash="6 4", head="open")
    s.line([(490, 111), (210, 111), (210, 200)], **u)
    s.line([(590, 142), (590, 200)], **u)
    s.line([(690, 111), (970, 111), (970, 200)], **u)
    s.label(350, 106, "creates", size=11)
    s.label(830, 106, "creates", size=11)
    s.label(590, 172, "creates", size=11)
    s.text(700, 166, "main also creates the relay_gpio", size=11, color=MUTED, italic=True)
    s.text(700, 181, "and pin_hal_esp instances", size=11, color=MUTED, italic=True)
    s.line([(310, 231), (490, 231)], **u)
    s.label(400, 226, "calls", size=11)
    s.line([(500, 262), (500, 300), (210, 300), (210, 340)], **u)
    s.line([(520, 262), (520, 340)], **u)
    s.line([(660, 262), (660, 300), (720, 300), (720, 340)], **u)
    s.label(720, 292, "replies via", size=11)
    s.line([(900, 262), (900, 318), (790, 318), (790, 340)], dash="6 4", head="hollow")
    s.label(860, 312, "implements", size=11)
    s.line([(1020, 262), (1020, 340)], **u)
    s.line([(1070, 231), (1170, 231), (1170, 771), (1150, 771)], **u)
    s.line([(545, 402), (545, 470)], head="open", tail="diamond")
    s.label(560, 440, "1..16", size=11, anchor="start")
    s.line([(480, 610), (480, 540)], dash="6 4", head="hollow")
    s.label(480, 580, "implements", size=11)
    s.line([(580, 641), (650, 641)], **u)
    s.line([(750, 740), (750, 676)], dash="6 4", head="hollow")
    s.label(750, 712, "implements", size=11)
    s.line([(850, 771), (920, 771)], **u)
    s.legend(60, 740, [("uses", "uses / depends on"), ("realize", "implements an interface"),
                       ("agg", "holds 1..16 of (aggregation)")])
    s.save()


# ===========================================================================
# 5-7. State machines
# ===========================================================================

def d_state_fsm():
    s = Svg("state-app-fsm", 1100, 640, "State machine: app_fsm",
            "Two states. All the work happens in READY, one received line at a time.")
    s.start(70, 155)
    s.box(130, 110, 230, 92, "INIT", ["created by main", "every line ignored (no reply)"], kind="neutral", rx=16)
    s.line([(80, 155), (130, 155)])
    s.label(105, 140, "create", size=11)
    s.box(470, 100, 600, 300, "READY", [
        "entry / drive ALL relays OFF (safe state)",
        "",
        "IN<n>_ON | IN<n>_OFF, n ≤ count",
        "      / drive · wait 15 ms · read back  →  IN<n>_…_OK",
        "ALL_ON | ALL_OFF",
        "      / drive all · wait 15 ms once · read back all  →  ALL_…_OK",
        "HeyU   / reply  Hello IGNACC v<version> READY",
        "STATUS / reply  STATUS IN1=… IN<count>=…",
        "anything else, or n > count  / reply  UNK_NACK",
        "",
        "any drive or read-back failure",
        "      / reply …_FAIL, drive ALL relays OFF",
    ], kind="app", rx=16, body_size=12.5, mono_body=True)
    s.line([(360, 156), (470, 156)])
    s.label(415, 146, "app_fsm_start()", size=11, mono=True, color=INK)
    s.label(415, 174, "/ all relays OFF", size=11)
    s.path("M 760 100 C 760 48, 880 48, 880 100")
    s.label(820, 66, "line received / handle it", size=11.5, color=INK)
    s.box(110, 270, 300, 190, "Notes", ["No final state: runs until power-off.",
                                        "A reset starts again in INIT, with",
                                        "every relay OFF.",
                                        "",
                                        "One line is handled completely",
                                        "(including the 15 ms wait) before the",
                                        "next one, so commands never overlap."], kind="note", rx=6)
    s.caption(530, ["Plain words: after start-up the controller waits for a line of text. If it is a valid command it",
                    "carries it out and answers; if anything goes wrong it switches every relay off and says _FAIL;",
                    "if the text isn't a command it answers UNK_NACK and changes nothing."])
    s.save()


def d_state_relay():
    s = Svg("state-relay-channel", 980, 520, "State machine: one relay channel",
            "Every channel (IN1 … INn) behaves the same way. OFF is the safe state.")
    s.start(70, 255)
    off = s.box(150, 205, 210, 100, "OFF", ["pin at OFF level", "(active-low: HIGH)"], kind="neutral", rx=16,
                align="center")
    on = s.box(620, 205, 210, 100, "ON", ["pin at ON level", "(active-low: LOW)"], kind="ok", rx=16, align="center")
    s.line([(80, 255), (150, 255)])
    s.label(112, 240, "boot", size=11)
    s.path("M 330 205 C 400 120, 580 120, 650 205")
    s.label(490, 128, "IN<n>_ON or ALL_ON  [drive + read-back OK]", size=11.5, color=INK)
    s.label(490, 146, "/ reply …_ON_OK", size=11.5)
    s.path("M 650 305 C 580 390, 400 390, 330 305")
    s.label(490, 368, "IN<n>_OFF or ALL_OFF  / reply …_OFF_OK", size=11.5, color=INK)
    s.label(490, 386, "any failure on any channel  / …_FAIL (safe state)", size=11.5, color="#b91c1c")
    s.label(490, 404, "reset or power-off", size=11.5)
    s.path("M 190 305 C 160 380, 260 380, 240 306")
    s.label(210, 400, "IN<n>_OFF, ALL_OFF", size=11)
    s.label(210, 416, "/ drive again, _OK", size=11)
    s.path("M 790 205 C 820 130, 720 130, 740 204")
    s.label(770, 112, "IN<n>_ON, ALL_ON / drive again, _OK", size=11)
    s.caption(460, ["Sending the same command twice is safe (idempotent): the pin is driven again and the answer is _OK.",
                    "If any relay fails, every relay goes to OFF — never to an unknown state."])
    s.save()


def d_state_line():
    s = Svg("state-line-assembler", 1040, 540, "State machine: line_assembler",
            "Turns the stream of received bytes into complete command lines.")
    s.start(70, 255)
    s.box(150, 200, 270, 110, "COLLECTING", ["store each byte", "(up to max_len + 1, room for one '\\r')"],
          kind="svc", rx=16, align="center")
    s.box(640, 200, 270, 110, "DISCARDING", ["drop every byte", "until the next '\\n'"], kind="bad", rx=16,
          align="center")
    s.line([(80, 255), (150, 255)])
    s.path("M 360 200 C 430 120, 610 120, 680 200")
    s.label(520, 122, "byte would make the line longer than max_len + 1", size=11.5, color=INK)
    s.label(520, 140, "or UART overflow / frame / parity error", size=11.5, color=INK)
    s.path("M 680 310 C 610 390, 430 390, 360 310")
    s.label(520, 384, "'\\n'  / nothing emitted (whole line dropped)", size=11.5, color=INK)
    s.path("M 200 200 C 170 120, 290 120, 270 199")
    s.label(232, 104, "'\\n' / strip one trailing '\\r';", size=11, color=INK)
    s.label(232, 120, "emit the line if it isn't empty", size=11, color=INK)
    s.path("M 860 310 C 900 390, 780 390, 800 311")
    s.label(830, 404, "UART error / stay", size=11)
    s.caption(450, ["max_len is 32 by default (menuconfig). A command hidden at the end of a too-long line is never",
                    "executed, and a line corrupted by a UART error is never passed on: both are dropped, silently."])
    s.save()


# ===========================================================================
# 8-9. Activity diagrams
# ===========================================================================

def d_activity_command():
    s = Svg("activity-command-handling", 1200, 1110, "Activity: handling one command",
            "What the state machine does with every received line.")
    cx = 560
    s.start(cx, 92)
    s.action(cx, 146, 300, 44, ["Take the next line from the queue"])
    s.connector(360, 146, "1")
    s.line([(373, 146), (410, 146)])
    s.line([(cx, 102), (cx, 124)])
    s.diamond(cx, 226, 170, 70, ["FSM READY?"])
    s.line([(cx, 168), (cx, 191)])
    s.action(890, 226, 200, 44, ["Ignore (no reply)"], kind="neutral")
    s.line([(645, 226), (790, 226)])
    s.label(700, 218, "[no: INIT]", size=11)
    s.line([(990, 226), (1040, 226)])
    s.connector(1053, 226, "1")
    s.action(cx, 306, 300, 44, ["Parse the line  (proto_parse)"])
    s.line([(cx, 261), (cx, 284)])
    s.label(cx + 26, 276, "[yes]", size=11, anchor="start")
    s.diamond(cx, 390, 200, 76, ["Valid command", "for this board?"])
    s.line([(cx, 328), (cx, 352)])
    s.action(200, 390, 220, 44, ["Reply UNK_NACK"], kind="bad")
    s.line([(460, 390), (310, 390)])
    s.label(395, 381, "[no]", size=11)
    s.line([(90, 390), (62, 390)])
    s.connector(49, 390, "1")
    s.diamond(cx, 486, 200, 70, ["Which command?"])
    s.line([(cx, 428), (cx, 451)])
    s.label(cx + 26, 445, "[yes]", size=11, anchor="start")
    s.action(200, 486, 240, 44, ["Reply Hello IGNACC v… READY"], kind="ok")
    s.line([(460, 486), (320, 486)])
    s.label(395, 478, "[HeyU]", size=11)
    s.line([(200, 508), (200, 540)])
    s.connector(200, 553, "1")
    s.action(925, 486, 270, 52, ["Read every relay back,", "reply STATUS IN1=… IN<n>=…"], kind="ok")
    s.line([(660, 486), (790, 486)])
    s.label(725, 478, "[STATUS]", size=11)
    s.line([(925, 512), (925, 540)])
    s.connector(925, 553, "1")
    s.action(cx, 590, 400, 52, ["Drive relay n — or every relay for ALL,", "back to back"])
    s.line([(cx, 521), (cx, 564)])
    s.label(cx + 26, 548, "[IN<n>_ON/OFF or ALL_ON/OFF]", size=11, anchor="start")
    s.diamond(cx, 680, 190, 66, ["Every drive OK?"])
    s.line([(cx, 616), (cx, 647)])
    s.action(cx, 770, 320, 44, ["Wait the settle delay (15 ms, once)"])
    s.line([(cx, 713), (cx, 748)])
    s.label(cx + 26, 736, "[yes]", size=11, anchor="start")
    s.action(cx, 845, 320, 44, ["Read back relay n — or every relay"])
    s.line([(cx, 792), (cx, 823)])
    s.diamond(cx, 930, 210, 70, ["Read-back matches", "the command?"])
    s.line([(cx, 867), (cx, 895)])
    s.action(cx, 1015, 260, 44, ["Reply …_OK"], kind="ok")
    s.line([(cx, 965), (cx, 993)])
    s.label(cx + 26, 984, "[yes]", size=11, anchor="start")
    s.line([(cx + 130, 1015), (cx + 200, 1015)])
    s.connector(cx + 213, 1015, "1")
    s.action(940, 770, 270, 52, ["Drive ALL relays OFF", "(safe state)"], kind="bad")
    s.line([(655, 680), (940, 680), (940, 744)], color="red")
    s.label(800, 672, "[no]", size=11, color="#b91c1c")
    s.line([(665, 930), (1110, 930), (1110, 770), (1075, 770)], color="red")
    s.label(880, 922, "[no]", size=11, color="#b91c1c")
    s.action(940, 860, 220, 44, ["Reply …_FAIL"], kind="bad")
    s.line([(940, 796), (940, 838)], color="red")
    s.line([(1050, 860), (1080, 860)], color="red")
    s.connector(1093, 860, "1")
    s.legend(60, 1000, [("ok", "normal answer"), ("bad", "UNK_NACK / failure path")])
    s.text(60, 1060, "1 = back to the top: take the next line.", size=12, color=MUTED)
    s.save()


def d_activity_boot():
    s = Svg("activity-boot", 980, 1000, "Activity: power-on to READY",
            "What happens between switching the board on and the first command being accepted.")
    cx = 420
    s.start(cx, 92)
    s.label(cx + 20, 96, "power on / reset", size=11.5, anchor="start")
    s.action(cx, 150, 520, 50, ["ROM bootloader runs (prints its boot text on UART0;", "the application can't disable this)"],
             kind="hw")
    s.line([(cx, 102), (cx, 125)])
    s.action(cx, 222, 520, 40, ["2nd-stage bootloader (silent: logs off)"], kind="hw")
    s.line([(cx, 175), (cx, 202)])
    s.action(cx, 286, 300, 40, ["app_main() starts"], kind="cfg")
    s.line([(cx, 242), (cx, 266)])
    s.rect(150, 320, 540, 250, fill="none", stroke="#6b7280", rx=14, dash="7 5")
    s.text(166, 342, "for each channel 1 … count", size=12, weight="bold", color=MUTED, italic=True)
    s.action(cx, 380, 420, 44, ["Write the OFF level to the pin"])
    s.line([(cx, 306), (cx, 358)])
    s.action(cx, 446, 420, 44, ["Then switch the pin to output (no ON glitch)"])
    s.line([(cx, 402), (cx, 424)])
    s.diamond(cx, 522, 210, 60, ["Pin reads back OFF?"])
    s.line([(cx, 468), (cx, 492)])
    s.action(800, 650, 280, 76, ["halt(): stop here.", "Relays stay OFF; HeyU gets", "no reply (host times out)."],
             kind="bad")
    s.line([(525, 522), (800, 522), (800, 612)], color="red")
    s.label(650, 514, "[no]", size=11, color="#b91c1c")
    s.action(cx, 630, 360, 40, ["Create the command queue"], kind="cfg")
    s.line([(cx, 552), (cx, 610)])
    s.label(cx + 22, 590, "[yes, all channels]", size=11, anchor="start")
    s.action(cx, 690, 360, 40, ["Start the UART link (driver + RX task)"], kind="cfg")
    s.line([(cx, 650), (cx, 670)])
    s.action(cx, 750, 360, 40, ["Create the state machine; drive ALL OFF"], kind="cfg")
    s.line([(cx, 710), (cx, 730)])
    s.action(cx, 810, 360, 40, ["Start the state-machine task"], kind="cfg")
    s.line([(cx, 770), (cx, 790)])
    s.line([(600, 750), (660, 750), (660, 688)], color="red", dash="5 4")
    s.label(706, 760, "any failure", size=11, color="#b91c1c")
    s.action(cx, 890, 360, 46, ["READY: waiting for commands"], kind="ok")
    s.line([(cx, 830), (cx, 867)])
    s.caption(950, ["From power-on to READY takes well under a second. Until then, relay pins are inputs or OFF, "
                    "so a relay never switches ON during boot."])
    s.save()


# ===========================================================================
# 10-11. Sequence diagrams
# ===========================================================================

def d_seq_command():
    s = Svg("sequence-relay-command", 1340, 900, "Sequence: one command through every layer (IN1_ON)",
            "Time runs downward. Solid arrow = call or message, dashed arrow = return or reply.")
    parts = [("host", "Host\n(CAPL / Tester)", "pc"), ("uart", "ESP-IDF\nUART driver", "hal"),
             ("link", "uart_link\nRX task", "drv"), ("q", "command\nqueue", "cfg"),
             ("task", "app_fsm\ntask", "app"), ("fsm", "app_fsm", "app"), ("bank", "relay_bank", "svc"),
             ("relay", "relay_gpio\n(IN1)", "drv"), ("hal", "pin_hal_esp", "hal")]
    items = [
        ("async", "host", "uart", '"IN1_ON\\n"'),
        ("async", "uart", "link", "UART_DATA event"),
        ("self", "link", "line_asm_push() × 7"),
        ("async", "link", "q", "xQueueSend(line)"),
        ("call", "q", "task", "xQueueReceive"),
        ("call", "task", "fsm", "handle_line()"),
        ("self", "fsm", "proto_parse() → IN1, ON"),
        ("call", "fsm", "bank", "set(0, ON)"),
        ("call", "bank", "relay", "relay_set(ON)"),
        ("call", "relay", "hal", "set_level(26, 0)"),
        ("call", "relay", "hal", "get_level(26)"),
        ("reply", "hal", "relay", "0  (LOW)"),
        ("reply", "relay", "bank", "RELAY_OK"),
        ("reply", "bank", "fsm", "RELAY_OK"),
        ("note", ["fsm"], ["pin is ON now"]),
        ("self", "fsm", "delay(15 ms)  settle"),
        ("call", "fsm", "bank", "is_on(0)"),
        ("reply", "bank", "fsm", "true"),
        ("self", "fsm", "format → IN1_ON_OK"),
        ("call", "fsm", "link", "link_tx_send()"),
        ("call", "link", "uart", "uart_write_bytes"),
        ("reply", "uart", "host", '"IN1_ON_OK\\r\\n"'),
        ("note", ["host", "uart"], ["≈ 19 ms after sending: 15 ms settle", "+ USB transfer both ways"]),
    ]
    s.h = sequence(s, parts, items, x0=95, gap=143) + 24
    s.save()


def d_seq_capl():
    s = Svg("sequence-capl-session", 1140, 1080, "Sequence: a CAPL test session (ACC timing sweep)",
            "How CANoe drives the board and the ECU together. CAN is sent only after the board confirms with _OK.")
    parts = [("capl", "CANoe /\nCAPL script", "pc"), ("board", "IGNACC board\n(ESP32)", "app"),
             ("acc", "ACC relay\n(IN1)", "hw"), ("ecu", "ECU under test", "neutral")]
    items = [
        ("note", ["capl"], ["measurement start"]),
        ("call", "capl", "board", "HeyU"),
        ("reply", "board", "capl", "Hello IGNACC v1.2.0 READY"),
        ("call", "capl", "board", "ALL_OFF"),
        ("async", "board", "acc", "OFF (all relays)"),
        ("reply", "board", "capl", "ALL_OFF_OK"),
        ("frame", "loop", "for OFF time t = 1.5 s, 1.6 s … 2.1 s", ["capl", "ecu"]),
        ("self", "capl", "wait t (msTimer)"),
        ("call", "capl", "board", "IN1_ON"),
        ("async", "board", "acc", "switch ON (GPIO26 LOW)"),
        ("async", "acc", "ecu", "ACC line = 12 V"),
        ("self", "board", "settle 15 ms, verify"),
        ("reply", "board", "capl", "IN1_ON_OK  (≈ 19 ms)"),
        ("async", "capl", "ecu", "CAN: ACC = ON"),
        ("self", "capl", "wait 1 s"),
        ("call", "capl", "board", "IN1_OFF"),
        ("async", "board", "acc", "switch OFF"),
        ("async", "acc", "ecu", "ACC line = 0 V"),
        ("reply", "board", "capl", "IN1_OFF_OK"),
        ("async", "capl", "ecu", "CAN: ACC = OFF"),
        ("frame", "alt", "reply is _FAIL, UNK_NACK, or none within 100 ms", ["capl", "board"]),
        ("self", "capl", "stop the test, log it"),
        ("note", ["board"], ["on _FAIL every relay", "is already OFF"]),
        ("end",),
        ("end",),
        ("call", "capl", "board", "ALL_OFF"),
        ("reply", "board", "capl", "ALL_OFF_OK"),
        ("note", ["capl"], ["measurement end:", "bench left safe"]),
    ]
    s.h = sequence(s, parts, items, x0=170, gap=270, row=34) + 24
    s.save()


# ===========================================================================
# 12. Communication diagram
# ===========================================================================

def d_communication():
    s = Svg("communication-relay-command", 1180, 700, "Communication diagram: IN1_ON",
            "The same exchange as the sequence diagram, drawn as the objects and the links between them.")
    W, H = 170, 52
    pos = {
        "host": (40, 140, ":Host", "pc"), "uart": (340, 140, ":UART driver", "hal"),
        "link": (640, 140, ":uart_link", "drv"), "q": (950, 140, ":command queue", "cfg"),
        "task": (950, 350, ":app_fsm task", "app"), "fsm": (640, 350, ":app_fsm", "app"),
        "bank": (640, 560, ":relay_bank", "svc"), "relay": (340, 560, "IN1 : relay_gpio", "drv"),
        "hal": (40, 560, ":pin_hal_esp", "hal"),
    }

    def c(k):
        x, y, _, _ = pos[k]
        return x + W / 2, y + H / 2

    for a, b in [("host", "uart"), ("uart", "link"), ("link", "q"), ("q", "task"), ("task", "fsm"),
                 ("fsm", "bank"), ("bank", "relay"), ("relay", "hal"), ("fsm", "link")]:
        (x1, y1), (x2, y2) = c(a), c(b)
        s.line([(x1, y1), (x2, y2)], head=None, sw=1.4, color="#9ca3af")
    for k, (x, y, t, kind) in pos.items():
        s.box(x, y, W, H, t, [], kind=kind, title_size=13.5)

    def msg(x, y, t, anchor="middle"):
        s.label(x, y, t, size=11.5, color=INK, mono=True, anchor=anchor)

    msg(260, 126, '1: "IN1_ON\\n" ▶')
    msg(260, 214, '◀ 7: "IN1_ON_OK\\r\\n"')
    msg(560, 126, "2: UART_DATA ▶")
    msg(560, 214, "◀ 6: uart_write_bytes()")
    msg(875, 126, "3: xQueueSend(line) ▶")
    msg(1045, 270, "4: xQueueReceive ▼", anchor="start")
    msg(880, 342, "◀ 5: handle_line()")
    msg(737, 270, "▲ 5.3: link_tx_send(IN1_ON_OK)", anchor="start")
    msg(737, 466, "▼ 5.1: set(0, ON)", anchor="start")
    msg(737, 486, "▼ 5.2: wait 15 ms, is_on(0)", anchor="start")
    msg(560, 546, "◀ 5.1.1: relay_set(ON)")
    msg(260, 634, "◀ 5.1.1.1: set_level / get_level")
    s.caption(662, ["Numbers give the order; 5.1 and 5.2 happen inside step 5. Arrows next to each label show the direction.",
                    "Use the sequence diagram to see the timing, and this one to see who talks to whom."])
    s.h = 712
    s.save()


# ===========================================================================
# 13. Interaction overview
# ===========================================================================

def ref_frame(s, cx, cy, w, h, title, detail=None, kind="app"):
    f, st = KIND[kind]
    x, y = cx - w / 2, cy - h / 2
    s.rect(x, y, w, h, fill=f, stroke=st, rx=4)
    s.raw(f'<path d="M{x},{y} h34 v11 l-6,7 h-28 z" fill="#ffffff" stroke="{st}" stroke-width="1.2"/>')
    s.text(x + 6, y + 13, "ref", size=11, weight="bold", color=st)
    s.text(cx, cy + (0 if detail else 5), title, size=13, weight="bold", anchor="middle")
    if detail:
        s.text(cx, cy + 18, detail, size=11.5, anchor="middle", color=MUTED)


def d_interaction_overview():
    s = Svg("interaction-overview", 940, 1000, "Interaction overview: a whole test session",
            "Each 'ref' box is a smaller interaction shown in its own sequence diagram.")
    cx = 420
    s.start(cx, 92)
    ref_frame(s, cx, 160, 440, 62, "Handshake: HeyU → Hello IGNACC v… READY", "retry until it answers (board may be booting)")
    s.line([(cx, 102), (cx, 129)])
    ref_frame(s, cx, 258, 440, 56, "Safe start: ALL_OFF → ALL_OFF_OK", "every relay OFF before the first step", kind="ok")
    s.line([(cx, 191), (cx, 230)])
    s.raw(f'<polygon points="{cx},{318} {cx + 12},{330} {cx},{342} {cx - 12},{330}" fill="#fff" stroke="{LINE}" stroke-width="1.5"/>')
    s.line([(cx, 286), (cx, 318)])
    s.diamond(cx, 410, 190, 66, ["More test steps?"])
    s.line([(cx, 342), (cx, 377)])
    ref_frame(s, cx, 508, 440, 62, "Switch: IN<n>_ON / IN<n>_OFF / ALL_…", "see sequence-relay-command")
    s.line([(cx, 443), (cx, 477)])
    s.label(cx + 24, 466, "[yes]", size=11, anchor="start")
    s.diamond(cx, 606, 150, 60, ["Reply?"])
    s.line([(cx, 539), (cx, 576)])
    ref_frame(s, cx, 700, 440, 56, "Send the matching CAN signal (CAPL)", "only after _OK", kind="pc")
    s.line([(cx, 636), (cx, 672)])
    s.label(cx + 24, 660, "[_OK]", size=11, anchor="start")
    s.line([(cx - 220, 700), (150, 700), (150, 330), (cx - 12, 330)])
    ref_frame(s, 690, 606, 230, 70, "Stop and log", "board already safe on _FAIL", kind="bad")
    s.line([(cx + 75, 606), (575, 606)], color="red")
    s.label(520, 628, "[_FAIL / NACK / timeout]", size=10.5, color="#b91c1c")
    s.line([(690, 641), (690, 690)], color="red")
    s.final(690, 703)
    ref_frame(s, cx, 830, 440, 56, "Safe end: ALL_OFF → ALL_OFF_OK", "bench left with every relay OFF", kind="ok")
    s.line([(cx + 95, 410), (870, 410), (870, 830), (cx + 220, 830)])
    s.label(780, 402, "[no]", size=11)
    s.final(cx, 910)
    s.line([(cx, 858), (cx, 897)])
    s.caption(952, ["Plain words: say hello, switch everything off, then repeat 'switch a relay, wait for OK, send the CAN "
                    "signal'; stop at the first problem; finish by switching everything off."])
    s.save()


# ===========================================================================
# 14-20. Module diagrams
# ===========================================================================

def d_mod_protocol():
    s = Svg("module-protocol", 1110, 650, "Module: protocol",
            "Turns a received text line into a command, and a result into the reply text. Knows the grammar, not the board.")
    p = s.box(40, 90, 520, 252, "protocol", [
        "proto_cmd_t proto_parse(line, len)",
        "int proto_format_result(cmd, ok, buf, size)",
        "int proto_format_hello(version, buf, size)",
        "int proto_format_status(on[], count, buf, size)",
        "int proto_format_nack(buf, size)",
        "",
        "PROTO_MAX_CHANNELS = 16",
        "PROTO_RESPONSE_MAX = 160",
    ], kind="svc", stereo="«module»  protocol.h / protocol.c", mono_body=True, body_size=12.5, badge="host-tested")
    s.box(620, 90, 200, 190, "proto_cmd_kind_t", ["NONE", "RELAY", "ALL", "HELLO", "STATUS"], kind="ifc",
          stereo="«enum»", mono_body=True)
    s.box(870, 90, 200, 140, "proto_cmd_t", ["kind", "channel  1..16", "on       bool"], kind="ifc",
          stereo="«struct»", mono_body=True)
    s.line([(560, 310), (970, 310), (970, 230)], dash="6 4", head="open")
    s.label(765, 304, "proto_parse() returns", size=11)
    s.line([(870, 200), (820, 200)], head="open")
    s.label(845, 194, "kind", size=11)
    s.box(40, 380, 1030, 230, "Grammar  (exact match, case-sensitive; every reply ends with \\r\\n)", [
        "IN<n>_ON | IN<n>_OFF    n = 1..16, no leading zeros     →  IN<n>_ON_OK / IN<n>_OFF_FAIL / …",
        "ALL_ON   | ALL_OFF                                       →  ALL_ON_OK  / ALL_OFF_FAIL  / …",
        "HeyU                                                     →  Hello IGNACC v<version> READY",
        "STATUS                                                   →  STATUS IN1=ON IN2=OFF …  (one per channel)",
        "anything else: IN0_ON, IN01_ON, IN17_ON, in1_on, 'IN1_ON '  →  UNK_NACK",
        "",
        "The protocol accepts IN1..IN16. Whether channel n exists on this board is decided by app_fsm",
        "(n > channel count → UNK_NACK), so the channel count can change without touching this module.",
    ], kind="neutral", mono_body=True, body_size=12)
    s.save()


def d_mod_relay():
    s = Svg("module-relay", 1210, 780, "Module: relay",
            "One interface (relay_t), one implementation per kind of hardware, and a bank that drives many at once.")
    s.box(40, 90, 330, 200, "relay_bank", ["relays : relay_t[count]", "count  : 1..16", "",
                                           "relay_bank_set(bank, i, on)", "relay_bank_is_on(bank, i)",
                                           "relay_bank_set_all(bank, on)", "relay_bank_all_are(bank, on)"],
          kind="svc", stereo="«module»  relay_bank.h", mono_body=True, badge="host-tested")
    s.box(440, 90, 330, 170, "relay_t", ["ops : relay_ops_t*", "ctx : void*  (opaque)", "",
                                         "relay_set(relay, on) → relay_err_t", "relay_is_on(relay) → bool"],
          kind="ifc", stereo="«interface»  relay.h", mono_body=True)
    s.line([(370, 175), (440, 175)], head="open", tail="diamond")
    s.label(405, 166, "1..16", size=11)
    s.box(440, 340, 330, 180, "relay_gpio", ["pin, on_level, off_level", "",
                                             "relay_gpio_create(cfg, hal, out)", "  write OFF → output → verify",
                                             "set(): drive pin, read back", "relay_gpio_destroy(relay)"],
          kind="drv", stereo="«implementation»  relay_gpio.h", mono_body=True, badge="host-tested")
    s.line([(605, 340), (605, 260)], dash="6 4", head="hollow")
    s.label(650, 302, "implements", size=11)
    s.box(40, 370, 330, 110, "relay_gpio_config_t", ["pin          : int", "active_level : 0 low / 1 high"],
          kind="ifc", stereo="«struct»", mono_body=True)
    s.line([(370, 425), (440, 425)], dash="6 4", head="open")
    s.box(850, 340, 320, 180, "pin_hal_t", ["configure_output(pin, initial)", "set_level(pin, level)",
                                            "get_level(pin) → 0 / 1", "", "contract: level is latched",
                                            "before output is enabled"],
          kind="ifc", stereo="«interface»  pin_hal.h", mono_body=True)
    s.line([(770, 430), (850, 430)], dash="6 4", head="open")
    s.label(810, 422, "uses", size=11)
    s.box(850, 590, 320, 110, "pin_hal_esp", ["the only file using driver/gpio.h", "gpio_set_level, gpio_config(IN_OUT)"],
          kind="hal", stereo="«implementation»  pin_hal_esp.h", mono_body=True)
    s.line([(1010, 590), (1010, 520)], dash="6 4", head="hollow")
    s.box(440, 590, 330, 110, "relay_mcp23017, relay_shiftreg, …",
          ["implement relay_t, add to the bank:", "no change to app_fsm or protocol"],
          kind="neutral", stereo="«future implementation»", dash="6 4")
    s.line([(440, 645), (410, 645), (410, 230), (440, 230)], dash="6 4", head="hollow")
    s.box(40, 560, 330, 170, "esp32_relay_pins.h", ["safe relay GPIOs (14):", "4 13 16 17 18 19 21",
                                                    "22 23 25 26 27 32 33", "", "used by main.c static_asserts",
                                                    "and checked by a host test"], kind="note", mono_body=True)
    s.legend(850, 730, [("realize", "implements"), ("agg", "holds 1..16")])
    s.save()


def d_mod_uart():
    s = Svg("module-uart-link", 1210, 720, "Module: uart_link (+ line_assembler, link_if)",
            "Moves text lines between the UART and the state machine. Knows nothing about relays or commands.")
    s.box(40, 110, 250, 130, "UART driver", ["ISR + ring buffer", "event queue: UART_DATA,", "overflow, frame error"],
          kind="hal", stereo="«ESP-IDF»")
    s.box(360, 90, 400, 250, "uart_link", ["uart_link_start(cfg, out_tx)", "", "RX task:",
                                           "  UART_DATA → read → line_asm_push()",
                                           "  complete line → xQueueSend (drop if full)",
                                           "  overflow / UART error → line_asm_discard()", "",
                                           "TX: link_tx_send → uart_write_bytes()"],
          kind="drv", stereo="«module»  uart_link.h", mono_body=True)
    s.line([(290, 175), (360, 175)], head="filled", tail="filled")
    s.label(325, 166, "bytes", size=11)
    s.box(360, 410, 400, 200, "line_assembler", ["line_asm_init(a, max_len)", "line_asm_push(a, c, out) → done?",
                                                 "line_asm_discard(a)", "", "strip one '\\r', skip empty lines,",
                                                 "drop the whole line if > max_len"],
          kind="svc", stereo="«module»  line_assembler.h", mono_body=True, badge="host-tested")
    s.line([(560, 340), (560, 410)], dash="6 4", head="open")
    s.box(840, 90, 330, 170, "link_if.h", ["link_line_t { len, data[128] }", "link_tx_t   { ops, ctx }",
                                           "link_tx_send(tx, buf, len)"],
          kind="ifc", stereo="«interface»", mono_body=True)
    s.line([(760, 175), (840, 175)], dash="6 4", head="hollow")
    s.label(800, 166, "implements", size=11)
    s.box(840, 330, 330, 100, "command queue", ["depth 4 (menuconfig)", "items: link_line_t"], kind="cfg",
          stereo="«FreeRTOS queue»")
    s.line([(760, 300), (800, 300), (800, 380), (840, 380)], head="open")
    s.label(800, 292, "xQueueSend", size=11)
    s.box(840, 500, 330, 110, "app_fsm_task → app_fsm", ["reads lines from the queue,", "replies through link_tx_t"],
          kind="app", stereo="«consumer»")
    s.line([(1005, 430), (1005, 500)], head="open")
    s.label(1060, 470, "xQueueReceive", size=11)
    s.line([(1170, 555), (1192, 555), (1192, 175), (1170, 175)], dash="6 4", head="open")
    s.label(1192, 470, "reply", size=11)
    s.caption(660, ["Why a queue: the UART task never waits for a relay to settle, so bytes keep arriving safely while a "
                    "command is executed."])
    s.save()


def d_mod_fsm():
    s = Svg("module-app-fsm", 1210, 740, "Module: app_fsm (+ app_fsm_task)",
            "The decision maker: sole owner of the relays. Everything it needs is passed in, so it runs on a PC too.")
    s.box(40, 90, 290, 230, "app_fsm_config_t", ["relays     relay_bank_t", "link       link_tx_t",
                                                 "fw_version char*", "settle_ms  uint32",
                                                 "delay      fn(ctx, ms)", "delay_ctx  void*"],
          kind="ifc", stereo="«struct»", mono_body=True)
    s.box(390, 90, 440, 290, "app_fsm", ["app_fsm_create(cfg)  copies relay handles", "app_fsm_start(fsm)   all OFF → READY",
                                         "app_fsm_handle_line(fsm, line, len)", "app_fsm_state(fsm)",
                                         "app_fsm_channel_count(fsm)", "", "private:",
                                         "  handle_relay_command()  handle_all_command()",
                                         "  handle_status()  drive_safe_state()"],
          kind="app", stereo="«module»  app_fsm.h", mono_body=True, body_size=12, badge="host-tested")
    s.line([(330, 200), (390, 200)], dash="6 4", head="open")
    s.label(360, 192, "create", size=11)
    s.box(900, 90, 270, 72, "protocol", ["parse / format"], kind="svc", align="center")
    s.box(900, 186, 270, 72, "relay_bank", ["drive and verify relays"], kind="svc", align="center")
    s.box(900, 282, 270, 72, "link_tx_t", ["send replies"], kind="ifc", align="center")
    for y in (126, 222, 318):
        s.line([(830, y), (900, y)], dash="6 4", head="open")
    s.box(390, 450, 440, 180, "app_fsm_task", ["app_fsm_task_start(fsm, cfg)",
                                               "loop: xQueueReceive → app_fsm_handle_line()",
                                               "app_fsm_rtos_delay_ms(ctx, ms)",
                                               "  vTaskDelay, rounded up: never shorter"],
          kind="app", stereo="«FreeRTOS runner»  app_fsm_task.h", mono_body=True, body_size=12)
    s.line([(610, 450), (610, 380)], dash="6 4", head="open")
    s.label(650, 418, "calls", size=11)
    s.box(900, 480, 270, 90, "command queue", ["lines from uart_link"], kind="cfg", align="center")
    s.line([(900, 525), (830, 525)], head="open")
    s.box(40, 450, 290, 180, "test_app_fsm.c", ["mock relays and mock link,", "fake delay records call order",
                                                 "1..16 channels, failures,", "ALL_ON / ALL_OFF, UNK_NACK"],
          kind="ok", stereo="«host test, 28 tests»")
    s.line([(330, 540), (360, 540), (360, 300), (390, 300)], dash="6 4", head="open")
    s.label(400, 420, "drives it with mocks", size=11, anchor="start")
    s.caption(680, ["The state machine owns its own copy of the relay handles (app_fsm_create copies them), "
                    "so nothing else can switch a relay behind its back."])
    s.save()


def d_mod_main():
    s = Svg("module-main", 1210, 640, "Module: main (composition root)",
            "The only place that knows the board: reads the configuration, checks it, and connects the pieces.")
    s.box(40, 90, 300, 160, "Kconfig.projbuild", ["menuconfig → IGNACC Controller", "relays · UART · tasks",
                                                  "channel count 1–14", "one GPIO per channel"], kind="cfg",
          stereo="«build-time settings»")
    s.box(40, 320, 300, 130, "sdkconfig.h", ["CONFIG_IGNACC_RELAY_COUNT 2", "CONFIG_IGNACC_RELAY1_GPIO 26",
                                             "…", "CONFIG_IGNACC_RELAY14_GPIO 23"], kind="cfg",
          stereo="«generated»", mono_body=True, body_size=11.5)
    s.line([(190, 250), (190, 320)])
    s.label(190, 290, "build", size=11)
    s.box(410, 90, 360, 360, "main.c: compile-time checks", [
        "X-macro list: (channel, gpio) × 14",
        "s_relay_gpios[] = all 14 pins", "",
        "for every channel ≤ count:",
        "  static_assert  safe pin",
        "  static_assert  not a UART pin", "",
        "popcount(pin bit mask) == count",
        "  → no two channels share a GPIO", "",
        "bad config → the build FAILS",
        "with a clear message"], kind="cfg", stereo="«compile time»", mono_body=True, body_size=12)
    s.line([(340, 385), (410, 385)])
    s.box(840, 90, 330, 360, "app_main()", [
        "1  create count × relay_gpio", "   (each latched OFF first)",
        "2  create the command queue",
        "3  start uart_link → link_tx_t",
        "4  app_fsm_create(relays, link,", "     version, settle, delay)",
        "5  app_fsm_start → all OFF",
        "6  app_fsm_task_start", "",
        "any failure → halt()",
        "(relays stay OFF)"], kind="app", stereo="«run time»", mono_body=True, body_size=12)
    s.line([(770, 270), (840, 270)])
    s.label(805, 262, "compiles", size=11)
    s.box(40, 500, 1130, 90, "Why this matters", [
        "Every other module receives plain structs (pins, relays, link, delay) instead of reading sdkconfig, so they "
        "run on a PC with mocks and",
        "can be reused on another board. Changing the board means changing menuconfig and, at most, main.c."],
          kind="note")
    s.save()


def d_kconfig_model():
    s = Svg("kconfig-channel-model", 1210, 720, "Kconfig channel model: from menuconfig to relays",
            "Example: a 2-channel board. Raising the count uses the next slots; nothing else changes.")
    s.box(40, 82, 1130, 96, "menuconfig → IGNACC Controller → Relays", [
        "Number of relay channels: 2",
        "IN1 relay GPIO: 26   ·   IN2 relay GPIO: 27   ·   IN3 … IN14: prompts hidden, defaults still defined"],
          kind="cfg", body_size=12.5)
    pins = [26, 27, 25, 33, 32, 4, 13, 16, 17, 18, 19, 21, 22, 23]
    x0, w, gap = 40, 74, 7.3
    for i, g in enumerate(pins):
        x = x0 + i * (w + gap)
        used = i < 2
        f, st = KIND["ok" if used else "neutral"]
        s.rect(x, 230, w, 72, fill=f, stroke=st, rx=8, dash=None if used else "5 4")
        s.text(x + w / 2, 258, f"IN{i + 1}", size=14, weight="bold", anchor="middle",
               color=INK if used else MUTED)
        s.text(x + w / 2, 280, f"GPIO {g}", size=11.5, anchor="middle", color=INK if used else MUTED)
    s.line([(40, 316), (40, 326), (195, 326), (195, 316)], head=None, color="#15803d", sw=2)
    s.text(118, 344, "used (count = 2)", size=12, weight="bold", anchor="middle", color="#15803d")
    s.line([(203, 316), (203, 326), (1170, 326), (1170, 316)], head=None, color="#6b7280", sw=1.5)
    s.text(686, 344, "defined but unused: the 14 safe pins as defaults, ready when the count grows", size=12,
           anchor="middle", color=MUTED)
    s.text(40, 214, "CONFIG_IGNACC_RELAY1_GPIO … CONFIG_IGNACC_RELAY14_GPIO in sdkconfig.h:", size=12, color=MUTED)

    s.line([(118, 352), (118, 400)])
    cx = 300
    checks = [("safe pin?", "esp32_relay_pins.h"), ("not a UART pin?", "TX 1 / RX 3"),
              ("no duplicates?", "popcount(mask) == count")]
    for i, (t, d) in enumerate(checks):
        x = 40 + i * 250
        s.box(x, 400, 230, 70, t, [d], kind="ifc", align="center", stereo="static_assert")
    s.line([(270, 435), (290, 435)])
    s.line([(520, 435), (540, 435)])
    s.line([(770, 435), (830, 435)])
    s.box(830, 392, 340, 46, "✓ all OK → firmware builds", [], kind="ok")
    s.box(830, 448, 340, 46, "✗ any fails → build stops with a message", [], kind="bad")
    s.line([(600, 470), (600, 540)])
    s.box(410, 540, 380, 110, "relay_bank  (count = 2)", ["IN1 → relay_gpio(26)", "IN2 → relay_gpio(27)"],
          kind="svc", align="center")
    s.line([(790, 595), (870, 595)])
    s.box(870, 540, 300, 110, "app_fsm", ["STATUS IN1=… IN2=…", "ALL_ON drives both", "IN3_ON → UNK_NACK"],
          kind="app", align="center", mono_body=True)
    s.text(40, 600, "app_main creates one relay", size=12, color=MUTED)
    s.text(40, 618, "per used slot and hands them", size=12, color=MUTED)
    s.text(40, 636, "to the state machine as a bank.", size=12, color=MUTED)
    s.save()


def d_mod_tester():
    s = Svg("module-tester", 1210, 720, "Module: IGNACC Tester (Python)",
            "A Windows GUI and command-line tool that drives the board over the COM port and checks every reply.")
    s.box(40, 90, 380, 290, "TesterApp", ["COM port dropdown, Connect", "test list: All / Selected / Range",
                                          "step delay, latency limit", "manual buttons incl. ALL ON / ALL OFF",
                                          "relay indicators", "log, PASS / FAIL per test", "",
                                          "results arrive via an event queue", "from the worker thread"],
          kind="pc", stereo="«Tkinter GUI»  ignacc_tester_gui.py")
    s.box(480, 90, 400, 290, "tester_core", ["Link: serial port, one request at a time",
                                             "TestContext: expect / expect_nack /", "  expect_silent, pacing, latency",
                                             "TESTS: 15 test cases",
                                             "run_tests(link, indices, delay, …)",
                                             "all_off(link): safe clean-up", "",
                                             "CLI: --port --tests --delay",
                                             "     --latency-limit --exclude-settle"],
          kind="svc", stereo="«module»  tester_core.py", mono_body=True, body_size=12)
    s.line([(420, 235), (480, 235)], dash="6 4", head="open")
    s.label(450, 227, "uses", size=11)
    s.box(950, 150, 220, 150, "ESP32 board", ["IGNACC firmware", "COM15", "115200 8N1"], kind="hw", align="center")
    s.line([(880, 225), (950, 225)], head="filled", tail="filled")
    s.label(915, 216, "serial", size=11)
    s.box(40, 450, 380, 180, "build_exe.ps1", ["venv from Python 3.13", "pinned pyserial + pyinstaller",
                                               "PyInstaller --onefile --windowed", "→ dist/IGNACC_Tester.exe"],
          kind="cfg", stereo="«build»")
    s.box(480, 450, 400, 180, "bin/v1.2.0/", ["IGNACC_Tester-v1.2.0-windows-x64.exe", "SHA256SUMS.txt",
                                              "README.md", "", "(v1.0.0 and v1.1.0 kept for older firmware)"],
          kind="ok", stereo="«committed»", mono_body=True, body_size=12)
    s.line([(420, 540), (480, 540)])
    s.label(450, 532, "copy", size=11)
    s.box(950, 380, 220, 250, "Threads", ["GUI thread: Tk only", "worker thread: tests", "results → event queue",
                                          "GUI polls every 50 ms", "", "relays OFF after every", "run, on Stop and on",
                                          "window close"], kind="note")
    s.caption(670, ["The tester checks the real board end to end; the 75 C unit tests check the firmware logic on the PC. "
                    "Both must pass."])
    s.save()


DIAGRAMS = [
    d_system_context, d_use_cases, d_layers, d_components,
    d_state_fsm, d_state_relay, d_state_line,
    d_activity_command, d_activity_boot,
    d_seq_command, d_seq_capl, d_communication, d_interaction_overview,
    d_mod_protocol, d_mod_relay, d_mod_uart, d_mod_fsm, d_mod_main, d_kconfig_model, d_mod_tester,
]

if __name__ == "__main__":
    print(f"Writing diagrams to {OUT}")
    for fn in DIAGRAMS:
        fn()
